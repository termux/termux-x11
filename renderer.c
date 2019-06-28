#include <stdlib.h>
#include "renderer.h"
#include "log.h"

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
			#define E(code) case code: desc = #code; break;
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
    }
}

#define checkGlError(op) checkGlError(op, __LINE__)
static GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
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

static GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
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

struct renderer *renderer_create(void) {
	struct renderer* r = calloc(1, sizeof(struct renderer));
	if (r == NULL) {
		LOGE("Failed to allocate renderer\n");
		return NULL;
	}
	r->gTextureProgram = createProgram(gSimpleVS, gSimpleFS); checkGlError("glCreateProgram");
	if (!r->gTextureProgram) {
		LOGE("GLESv2: Unable to create shader program");
		renderer_destroy(&r);
		return NULL;
	}
	r->gvPos = (GLuint) glGetAttribLocation(r->gTextureProgram, "position"); checkGlError("glGetAttribLocation");
	r->gvCoords = (GLuint) glGetAttribLocation(r->gTextureProgram, "texCoords"); checkGlError("glGetAttribLocation");
	r->gvTextureSamplerHandle = (GLuint) glGetUniformLocation(r->gTextureProgram, "texture"); checkGlError("glGetAttribLocation");	
	return r;
}

void renderer_destroy(struct renderer** renderer) {
	if (!renderer || !(*renderer)) return;
	
	glUseProgram(0);
	glDeleteProgram((*renderer)->gTextureProgram);
	free(*renderer);
	*renderer = NULL;
}

struct texture *texture_create(int width, int height) {
	if (width <= 0 || height <= 0) return NULL;
	struct texture* texture = calloc(1, sizeof(struct texture));
	if (texture == NULL) {
		LOGE("Failed to allocate renderer\n");
		return NULL;
	}
	
	//LOGD("Creating %dx%d texture", width, height);
	
    glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	
	texture->width = width;
	texture->height = height;
	return texture;
}

void texture_upload(struct texture *texture, void *data) {
	if (!texture || !data) return;
	
	//LOGD("Texture uploading");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);
	
	#if 1
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	#else
	int h;
	for (h=texture->damage.y; h<(texture->damage.y+texture->damage.height); h++) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, texture->damage.y, texture->width, texture->damage.height, GL_RGBA, GL_UNSIGNED_BYTE, &data[texture->damage.y * texture->width * 4]);
		texture->damage.x = 
		texture->damage.y = 
		texture->damage.width = 
		texture->damage.height = 0;
	}
	#endif
}

void texture_draw(struct renderer* renderer, struct texture *texture, float x0, float y0, float x1, float y1) {
	if (!renderer || !texture) return;
	//LOGD("Texture drawing");
	float coords[20] = {
			x0, -y0, 0.f, 0.f, 0.f,
			x1, -y0, 0.f, 1.f, 0.f,
			x0, -y1, 0.f, 0.f, 1.f,
			x1, -y1, 0.f, 1.f, 1.f,
	};
	
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(renderer->gTextureProgram); checkGlError("glUseProgram");
    glBindTexture(GL_TEXTURE_2D, texture->id);
    
    glVertexAttribPointer(renderer->gvPos, 3, GL_FLOAT, GL_FALSE, 20, coords); checkGlError("glVertexAttribPointer");
	glVertexAttribPointer(renderer->gvCoords, 2, GL_FLOAT, GL_FALSE, 20, &coords[3]); checkGlError("glVertexAttribPointer");
	glEnableVertexAttribArray(renderer->gvPos); checkGlError("glEnableVertexAttribArray");
	glEnableVertexAttribArray(renderer->gvCoords); checkGlError("glEnableVertexAttribArray");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError("glDrawArrays");
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); checkGlError("glUseProgram");
}

void texture_destroy(struct texture** texture) {
	if (!texture || !(*texture)) return;
	//LOGD("Texture destroying");
	
	glDeleteTextures(1, &(*texture)->id);
	
	free(*texture);
	*texture = NULL;
}
