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

void lorie_renderer::on_surface_create() {
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

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &toplevel_texture.id);
	glGenTextures(1, &cursor_texture.id);

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

	if (compositor.toplevel) {
		auto data = any_cast<lorie_compositor::client_data*>(compositor.toplevel->client()->user_data());
		compositor.report_mode(data->output);
	}
}

void lorie_renderer::cursor_move(uint32_t x, uint32_t y) {
	if (compositor.cursor == nullptr) return;

	data(compositor.cursor)->x = x;
	data(compositor.cursor)->y = y;
	request_redraw();
}

void lorie_renderer::set_cursor_visibility(bool visibility) {
	if (cursor_visible != visibility)
		cursor_visible = visibility;
}

void lorie_renderer::set_toplevel(surface_t* sfc) {
	LOGV("Setting surface %p as toplevel", sfc);
	compositor.toplevel = sfc;
	request_redraw();
}

void lorie_renderer::set_cursor(surface_t* sfc, uint32_t hs_x, uint32_t hs_y) {
	LOGV("Setting surface %p as cursor", sfc);
	compositor.cursor = sfc;
	this->hotspot_x = hs_x;
	this->hotspot_y = hs_y;

	request_redraw();
}

void lorie_renderer::draw(GLuint id, float x0, float y0, float x1, float y1) const {
    float coords[20] = {
            x0, -y0, 0.f, 0.f, 0.f,
            x1, -y0, 0.f, 1.f, 0.f,
            x0, -y1, 0.f, 0.f, 1.f,
            x1, -y1, 0.f, 1.f, 1.f,
    };
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(g_texture_program);
    glBindTexture(GL_TEXTURE_2D, id);

    glVertexAttribPointer(gv_pos, 3, GL_FLOAT, GL_FALSE, 20, coords);
    glVertexAttribPointer(gv_coords, 2, GL_FLOAT, GL_FALSE, 20, &coords[3]);
    glEnableVertexAttribArray(gv_pos);
    glEnableVertexAttribArray(gv_coords);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void lorie_renderer::draw_cursor() {
	if (compositor.cursor == nullptr) return;

	auto toplevel = compositor.toplevel ? data(compositor.toplevel) : nullptr;
	auto cursor = compositor.cursor ? data(compositor.cursor) : nullptr;
	
	if (cursor && cursor->damaged && cursor->buffer && cursor->buffer->is_shm()) {
		GLsizei w = cursor_texture.width = cursor->buffer->shm_width();
		GLsizei h = cursor_texture.height = cursor->buffer->shm_height();
		void* d = cursor->buffer->shm_data();
		glBindTexture(GL_TEXTURE_2D, cursor_texture.id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
	}
	if (!cursor_texture.valid()) return;
	
	float x, y, w, h;
	x = 2*(((float)(cursor->x-hotspot_x))/(float)toplevel_texture.width) - 1;
	y = 2*(((float)(cursor->y-hotspot_y))/(float)toplevel_texture.height) - 1;
	w = 2*((float)cursor_texture.width)/(float)toplevel_texture.width;
	h = 2*((float)cursor_texture.height)/(float)toplevel_texture.height;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw(cursor_texture.id, x, y, x + w, y + h);
	glDisable(GL_BLEND);
}

void lorie_renderer::redraw() {
	idle = nullptr;
	if (!ready) return;
	
	if (compositor.toplevel && data(compositor.toplevel)) {
		auto d = data(compositor.toplevel);
		if (d->damaged && d->buffer && d->buffer->is_shm()) {
			if (d->buffer->shm_width() != toplevel_texture.width || d->buffer->shm_width() != toplevel_texture.width) {
				GLsizei w = toplevel_texture.width = d->buffer->shm_width();
				GLsizei h = toplevel_texture.height = d->buffer->shm_height();
				void* dd = d->buffer->shm_data();
				glBindTexture(GL_TEXTURE_2D, toplevel_texture.id);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dd);
			} else {
				glBindTexture(GL_TEXTURE_2D, toplevel_texture.id);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
								toplevel_texture.width, toplevel_texture.height, GL_RGBA, GL_UNSIGNED_BYTE, d->buffer->shm_data());
				if (glGetError() == GL_INVALID_OPERATION)
					/* For some reason if our activity goes to background it loses access
					 * to this texture. In this case it should be regenerated. */
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, toplevel_texture.width, toplevel_texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, d->buffer->shm_data());
			}
		}

		draw(toplevel_texture.id, -1.0, -1.0, 1.0, 1.0);

		if (cursor_visible)
			draw_cursor();
	} else {
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	compositor.swap_buffers();
}
