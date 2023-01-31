#include <lorie_compositor.hpp>
#include <GLES2/gl2ext.h>

#include <sys/types.h>
#include <unistd.h>

using namespace wayland;

static const char vertex_shader[] = R"(
    attribute vec4 position;
    attribute vec2 texCoords;
    varying vec2 outTexCoords;
    void main(void) {
       outTexCoords = texCoords;
       gl_Position = position;
    }
)";
static const char fragment_shader[] = R"(
    precision mediump float;
    varying vec2 outTexCoords;
    uniform sampler2D texture;
    void main(void) {
       gl_FragColor = texture2D(texture, outTexCoords).bgra;
    }
)";

static GLuint load_shader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, nullptr);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                    LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

static GLuint create_program(const char* p_vertex_source, const char* p_fragment_source) {
    GLuint vertexShader = load_shader(GL_VERTEX_SHADER, p_vertex_source);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = load_shader(GL_FRAGMENT_SHADER, p_fragment_source);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, nullptr, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

static inline lorie_compositor::surface_data* data(surface_t* sfc) {
	return any_cast<lorie_compositor::surface_data*>(sfc->user_data());
}

lorie_renderer::lorie_renderer(lorie_compositor& compositor) : compositor(compositor) {}

void lorie_renderer::init() {
	LOGV("Initializing renderer (tid %d)", ::gettid());
	g_texture_program = create_program(vertex_shader, fragment_shader);
	if (!g_texture_program) {
		LOGE("GLESv2: Unable to create shader program");
		return;
	}
	gv_pos = (GLuint) glGetAttribLocation(g_texture_program, "position");
	gv_coords = (GLuint) glGetAttribLocation(g_texture_program, "texCoords");
	gv_texture_sampler_handle = (GLuint) glGetUniformLocation(g_texture_program, "texture");
	ready = true;	
	
	redraw();
}

void lorie_renderer::request_redraw() {
	//LOGV("Requesting redraw");
	if (idle) return; 
	
	idle = wl_event_loop_add_idle(
		wl_display_get_event_loop(compositor.display),
		[](void* data) { reinterpret_cast<lorie_renderer*>(data)->redraw(); },
		this
	);
}

void lorie_renderer::resize(int w, int h, uint32_t pw, uint32_t ph) {
	LOGV("Resizing renderer to %dx%d (%dmm x %dmm)", w, h, pw, ph);
	if (w == width &&
		h == height &&
		pw == physical_width &&
		ph == physical_height) return;
	width = w;
	height = h;
	physical_width = pw;
	physical_height = ph;
	
	glViewport(0, 0, w, h);

	if (toplevel_surface) {
		auto data = any_cast<lorie_compositor::client_data*>(toplevel_surface->client()->user_data());
		compositor.report_mode(data->output);
	}
}

void lorie_renderer::cursor_move(uint32_t x, uint32_t y) {
	if (compositor.cursor == nullptr) return;

	data(cursor_surface)->x = x;
	data(cursor_surface)->y = y;
	request_redraw();
}

void lorie_renderer::set_cursor_visibility(bool visibility) {
	if (cursor_visible != visibility)
		cursor_visible = visibility;
}

void lorie_renderer::set_toplevel(surface_t* surface) {
	LOGV("Setting surface %p as toplevel", surface);
	if (toplevel_surface) data(toplevel_surface)->texture.uninit();
	toplevel_surface = surface;
	request_redraw();
}

void lorie_renderer::set_cursor(surface_t* surface, uint32_t hotspot_x, uint32_t hotspot_y) {
	LOGV("Setting surface %p as cursor", surface);
	if (cursor_surface)
		data(cursor_surface)->texture.uninit();
	cursor_surface = surface;
	this->hotspot_x = hotspot_x;
	this->hotspot_y = hotspot_y;

	request_redraw();
}

void lorie_renderer::draw_cursor() {
	if (cursor_surface == nullptr) return;

	auto toplevel = data(toplevel_surface);
	auto cursor = data(cursor_surface);
	
	if (!cursor->texture.valid()) cursor->texture.reinit();
	if (!cursor->texture.valid()) return;
	
	float x, y, width, height, hs_x, hs_y;
	hs_x = ((float)hotspot_x)/toplevel->texture.width*2;
	hs_y = ((float)hotspot_y)/toplevel->texture.height*2;
	x = (((float)cursor->x)/toplevel->texture.width)*2 - 1.0 - hs_x;
	y = (((float)cursor->y)/toplevel->texture.height)*2 - 1.0 - hs_y;
	width = 2*((float)cursor->texture.width)/toplevel->texture.width;
	height = 2*((float)cursor->texture.height)/toplevel->texture.height;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	cursor->texture.draw(x, y, x + width, y + height);
	glDisable(GL_BLEND);
}

void lorie_renderer::redraw() {
	//LOGV("Redrawing screen");
	idle = NULL;
	if (!ready) return;
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (toplevel_surface && data(toplevel_surface)) {
		if (!data(toplevel_surface)->texture.valid())
			data(toplevel_surface)->texture.reinit();
		data(toplevel_surface)->texture.draw(-1.0, -1.0, 1.0, 1.0);

		if (cursor_visible)
			draw_cursor();
	}

	compositor.swap_buffers();
}

void lorie_renderer::uninit() {
	ready = false;
	LOGV("Destroying renderer (tid %d)", ::gettid());
	glUseProgram(0);
	glDeleteProgram(g_texture_program);
	g_texture_program = gv_pos = gv_coords = gv_texture_sampler_handle = 0;

	if (toplevel_surface) data(toplevel_surface)->texture.uninit();
	if (cursor_surface) data(cursor_surface)->texture.uninit();
}

lorie_renderer::~lorie_renderer() {
	uninit();
}

lorie_texture::lorie_texture(){
	uninit();
}

void lorie_texture::uninit() {
	if (valid()) {
		glDeleteTextures(1, &id);
	}
	id = UINT_MAX;
	width = height = 0;
	r = nullptr;
}

void lorie_texture::set_data(lorie_renderer* renderer, uint32_t width, uint32_t height, void *data) {
	uninit();
	LOGV("Reinitializing texture to %dx%d", width, height);
	this->r = renderer;
	this->width = width;
	this->height = height;
	this->data = data;
	reinit();
}

void lorie_texture::reinit() {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	
    glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void lorie_texture::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
	damaged = true;
	r->request_redraw();
}

bool lorie_texture::valid() {
	return (width != 0 && height != 0 && id != UINT_MAX && r != nullptr);
}

void lorie_texture::draw(float x0, float y0, float x1, float y1) {
	if (!valid()) return;
	float coords[20] = {
			x0, -y0, 0.f, 0.f, 0.f,
			x1, -y0, 0.f, 1.f, 0.f,
			x0, -y1, 0.f, 0.f, 1.f,
			x1, -y1, 0.f, 1.f, 1.f,
	};
	//LOGV("Drawing texture %p", this);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(r->g_texture_program);
    glBindTexture(GL_TEXTURE_2D, id);
    
    if (damaged) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		damaged = false;
	}
    
    glVertexAttribPointer(r->gv_pos, 3, GL_FLOAT, GL_FALSE, 20, coords);
	glVertexAttribPointer(r->gv_coords, 2, GL_FLOAT, GL_FALSE, 20, &coords[3]);
	glEnableVertexAttribArray(r->gv_pos);
	glEnableVertexAttribArray(r->gv_coords);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
