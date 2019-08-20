#include <lorie-compositor.hpp>
#include <lorie-renderer.hpp>
#include <GLES2/gl2ext.h>

#include <sys/types.h>
#include <unistd.h>

#ifndef __ANDROID__
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#define RANDF ((float)rand()/((float)RAND_MAX))

static const char gSimpleVS[] =
    "attribute vec4 position;\n"
    "attribute vec2 texCoords;\n"
    "varying vec2 outTexCoords;\n"
    "\nvoid main(void) {\n"
    "   outTexCoords = texCoords;\n"
    "   gl_Position = position;\n"
    "}\n\n";
static const char gSimpleFS[] =
    "precision mediump float;\n\n"
    "varying vec2 outTexCoords;\n"
    "uniform sampler2D texture;\n"
    "\nvoid main(void) {\n"
    //"   gl_FragColor = texture2D(texture, outTexCoords);\n"
    "   gl_FragColor = texture2D(texture, outTexCoords).bgra;\n"
    //"   gl_FragColor = vec4(outTexCoords.x/outTexCoords.y,outTexCoords.y/outTexCoords.x, 0.0, 0.0);\n"
    "}\n\n";

static void checkGlError(const char* op, int line) {
    GLint error;
    char *desc = NULL;
    for (error = glGetError(); error; error = glGetError()) {
		switch (error) {
			#define E(code) case code: desc = (char*)#code; break;
			E(GL_INVALID_ENUM);
			E(GL_INVALID_VALUE);
			E(GL_INVALID_OPERATION);
			E(GL_STACK_OVERFLOW_KHR);
			E(GL_STACK_UNDERFLOW_KHR);
			E(GL_OUT_OF_MEMORY);
			E(GL_INVALID_FRAMEBUFFER_OPERATION);
			E(GL_CONTEXT_LOST_KHR);
			#undef E
		}
        LOGE("GL: %s after %s() (line %d)", desc, op, line);
        return;
    }
    //LOGE("Last op on line %d was successfull", line);
}

#define checkGlError(op) checkGlError(op, __LINE__)
static GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL); checkGlError("glShaderSource");
        glCompileShader(shader); checkGlError("glCompileShader");
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled); checkGlError("glGetShaderiv");
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen); checkGlError("glGetShaderiv");
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf); checkGlError("glGetShaderInfoLog");
                    LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader); checkGlError("glDeleteShader");
                shader = 0;
            }
        }
    }
    return shader;
}

static GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram(); checkGlError("glCreateProgram");
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program); checkGlError("glLinkProgram");
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus); checkGlError("glGetProgramiv");
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength); checkGlError("glGetProgramiv");
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf); checkGlError("glGetProgramInfoLog");
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program); checkGlError("glDeleteProgram");
            program = 0;
        }
    }
    return program;
}

LorieRenderer::LorieRenderer(LorieCompositor& compositor) : compositor(compositor) {
}

void LorieRenderer::init() {
	LOGV("Initializing renderer (tid %d)", ::gettid());
	gTextureProgram = createProgram(gSimpleVS, gSimpleFS); checkGlError("glCreateProgram");
	if (!gTextureProgram) {
		LOGE("GLESv2: Unable to create shader program");
		return;
	}
	gvPos = (GLuint) glGetAttribLocation(gTextureProgram, "position"); checkGlError("glGetAttribLocation");
	gvCoords = (GLuint) glGetAttribLocation(gTextureProgram, "texCoords"); checkGlError("glGetAttribLocation");
	gvTextureSamplerHandle = (GLuint) glGetUniformLocation(gTextureProgram, "texture"); checkGlError("glGetAttribLocation");
	ready = true;	
	
	redraw();
}

void LorieRenderer::requestRedraw() {
	//LOGV("Requesting redraw");
	if (idle) return; 
	
	idle = wl_event_loop_add_idle(
		wl_display_get_event_loop(compositor.display),
		&idleDraw,
		this
	);
}

void LorieRenderer::resize(uint32_t w, uint32_t h, uint32_t pw, uint32_t ph) {
	LOGV("Resizing renderer to %dx%d (%dmm x %dmm)", w, h, pw, ph);
	if (w == width &&
		h == height &&
		pw == physical_width &&
		ph == physical_height) return;
	width = w;
	height = h;
	physical_width = pw;
	physical_height = ph;
	
	glViewport(0, 0, w, h); checkGlError("glViewport");

	LorieClient *cl = nullptr;
	if (compositor.toplevel != nullptr) cl = LorieClient::get(compositor.toplevel->client);
	if (cl == nullptr) return;
	cl->output->report_mode();
}

void LorieRenderer::cursorMove(uint32_t x, uint32_t y) {
	if (compositor.cursor == NULL) return;
	
	compositor.cursor->x = x;
	compositor.cursor->y = y;
	requestRedraw();
}

void LorieRenderer::setCursorVisibility(bool visibility) {
	if (cursorVisible != visibility)
		cursorVisible = visibility;
}

void LorieRenderer::set_toplevel(LorieSurface *surface) {
	LOGV("Setting surface %p as toplevel", surface);
	if (toplevel_surface) toplevel_surface->texture.uninit();
	toplevel_surface = surface;
	requestRedraw();
}

void LorieRenderer::set_cursor(LorieSurface *surface, uint32_t hotspot_x, uint32_t hotspot_y) {
	LOGV("Setting surface %p as cursor", surface);
	if (cursor_surface) cursor_surface->texture.uninit();
	cursor_surface = surface;
	this->hotspot_x = hotspot_x;
	this->hotspot_y = hotspot_y;

	requestRedraw();
}

void LorieRenderer::idleDraw(void *data) {
	LorieRenderer* r = static_cast<LorieRenderer*> (data);
	
	r->redraw();
}

void LorieRenderer::drawCursor() {
	if (toplevel_surface == NULL || cursor_surface == NULL) return;

	LorieTexture& toplevel = compositor.toplevel->texture;
	LorieTexture& cursor = compositor.cursor->texture;
	
	if (!cursor.valid()) cursor.reinit();
	if (!cursor.valid()) return;
	
	float x, y, width, height, hs_x, hs_y;
	hs_x = ((float)hotspot_x)/toplevel.width*2;
	hs_y = ((float)hotspot_y)/toplevel.height*2;
	x = (((float)compositor.cursor->x)/toplevel.width)*2 - 1.0 - hs_x;
	y = (((float)compositor.cursor->y)/toplevel.height)*2 - 1.0 - hs_y;
	width = 2*((float)cursor.width)/toplevel.width;
	height = 2*((float)cursor.height)/toplevel.height;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	cursor.draw(x, y, x + width, y + height);
	glDisable(GL_BLEND);
}

void LorieRenderer::redraw() {
	//LOGV("Redrawing screen");
	idle = NULL;
	if (!ready) return;
	//#define RAND ((float)(rand()%256)/256)
	//glClearColor(RAND, RAND, RAND, 1.0); checkGlError("glClearColor");
	glClearColor(0.f, 0.f, 0.f, 0.f); checkGlError("glClearColor");
	glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear");
	
	if (toplevel_surface) {
		if (!toplevel_surface->texture.valid())
			toplevel_surface->texture.reinit();
		toplevel_surface->texture.draw(-1.0, -1.0, 1.0, 1.0);
	}

	if (cursorVisible)
		drawCursor();

	compositor.swap_buffers();
}

void LorieRenderer::uninit() {
	ready = false;
	LOGV("Destroying renderer (tid %d)", ::gettid());
	glUseProgram(0);
	glDeleteProgram(gTextureProgram);
	gTextureProgram = gvPos = gvCoords = gvTextureSamplerHandle = 0;

	if (toplevel_surface) toplevel_surface->texture.uninit();
	if (cursor_surface) cursor_surface->texture.uninit();
}

LorieRenderer::~LorieRenderer() {
	uninit();
}

LorieTexture::LorieTexture(){
	uninit();
}

void LorieTexture::uninit() {
	if (valid()) {
		glDeleteTextures(1, &id);
		checkGlError("glDeleteTextures");
	}
	id = UINT_MAX;
	width = height = 0;
	r = nullptr;
}

void LorieTexture::set_data(LorieRenderer* renderer, uint32_t width, uint32_t height, void *data) {
	uninit();
	LOGV("Reinitializing texture to %dx%d", width, height);
	this->r = renderer;
	this->width = width;
	this->height = height;
	this->data = data;
	reinit();
}

void LorieTexture::reinit() {

	glClearColor(0.f, 0.f, 0.f, 0.f); checkGlError("glClearColor");
	glClear(GL_COLOR_BUFFER_BIT); checkGlError("glClear");
	
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture");
	glGenTextures(1, &id); checkGlError("glGenTextures");
    glBindTexture(GL_TEXTURE_2D, id); checkGlError("glBindTexture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); checkGlError("glTexImage2D");
}

void LorieTexture::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
	damaged = true;
	r->requestRedraw();
}

bool LorieTexture::valid() {
	return (width != 0 && height != 0 && id != UINT_MAX && r != nullptr);
}

void LorieTexture::draw(float x0, float y0, float x1, float y1) {
	if (!valid()) return;
	float coords[20] = {
			x0, -y0, 0.f, 0.f, 0.f,
			x1, -y0, 0.f, 1.f, 0.f,
			x0, -y1, 0.f, 0.f, 1.f,
			x1, -y1, 0.f, 1.f, 1.f,
	};
	//LOGV("Drawing texture %p", this);
    glActiveTexture(GL_TEXTURE0); checkGlError("glActiveTexture");
    glUseProgram(r->gTextureProgram); checkGlError("glUseProgram");
    glBindTexture(GL_TEXTURE_2D, id); checkGlError("glBindTexture");
    
    if (damaged) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		checkGlError("glTexSubImage2D");
		damaged = false;
	}
    
    glVertexAttribPointer(r->gvPos, 3, GL_FLOAT, GL_FALSE, 20, coords); checkGlError("glVertexAttribPointer");
	glVertexAttribPointer(r->gvCoords, 2, GL_FLOAT, GL_FALSE, 20, &coords[3]); checkGlError("glVertexAttribPointer");
	glEnableVertexAttribArray(r->gvPos); checkGlError("glEnableVertexAttribArray");
	glEnableVertexAttribArray(r->gvCoords); checkGlError("glEnableVertexAttribArray");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError("glDrawArrays");
    
    glBindTexture(GL_TEXTURE_2D, 0); checkGlError("glBindTexture");
    glUseProgram(0); checkGlError("glUseProgram");
}
