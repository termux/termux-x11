#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define LOGI(...) (printf("(I): " __VA_ARGS__))
#define LOGW(...) (printf("(W): " __VA_ARGS__))
#define LOGE(...) (printf("(E): " __VA_ARGS__))

static GLuint gTextureProgram;
static GLuint gvTexturePositionHandle;
static GLuint gvTextureTexCoordsHandle;
static GLuint gvTextureSamplerHandle;

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
    "   gl_FragColor = texture2D(texture, outTexCoords);\n"
    "}\n\n";

static void checkGlError(const char* op) {
    GLint error;
    for (error = glGetError(); error; error = glGetError()) {
        LOGE("after %s() glError (0x%x)\n", op, error);
    }
}
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
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
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

bool setupGraphics(void) {

    gTextureProgram = createProgram(gSimpleVS, gSimpleFS);
    if (!gTextureProgram) {
        return false;
    }
    gvTexturePositionHandle = glGetAttribLocation(gTextureProgram, "position"); checkGlError("glGetAttribLocation");
    gvTextureTexCoordsHandle = glGetAttribLocation(gTextureProgram, "texCoords"); checkGlError("glGetAttribLocation");
    gvTextureSamplerHandle = glGetUniformLocation(gTextureProgram, "texture"); checkGlError("glGetAttribLocation");

    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

#define FLOAT_SIZE_BYTES 4;
const GLint TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
const GLfloat gTriangleVerticesData[] = {
    // X, Y, Z, U, V
    -1.0f, 1.0f, 0, 0.f, 0.f,
    1.0f, 1.0f, 0, 1.f, 0.f,
    -1.0f,  -1.0f, 0, 0.f, 1.f,
    1.0f,   -1.0f, 0, 1.f, 1.f,
};

void glDrawTex(void) {
    if (!gTextureProgram) {
        if(!setupGraphics()) {
            LOGE("Could not set up graphics.\n");
            return;
        }
    }
    glUseProgram(gTextureProgram); checkGlError("glUseProgram");

    glVertexAttribPointer(gvTexturePositionHandle, 3, GL_FLOAT, GL_FALSE,
            TRIANGLE_VERTICES_DATA_STRIDE_BYTES, gTriangleVerticesData);
    checkGlError("glVertexAttribPointer");
    glVertexAttribPointer(gvTextureTexCoordsHandle, 2, GL_FLOAT, GL_FALSE,
            TRIANGLE_VERTICES_DATA_STRIDE_BYTES, &gTriangleVerticesData[3]);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvTexturePositionHandle);
    glEnableVertexAttribArray(gvTextureTexCoordsHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");

    glUseProgram(0); checkGlError("glUseProgram");
}

static void glDrawTexCoords(float x0, float y0, float x1, float y1) {
	float coords[20] = {
			x0, -y0, 0.f, 0.f, 0.f,
			x1, -y0, 0.f, 1.f, 0.f,
			x0, -y1, 0.f, 0.f, 1.f,
			x1, -y1, 0.f, 1.f, 1.f
	};

    glUseProgram(gTextureProgram); checkGlError("glUseProgram");

	glVertexAttribPointer(gvTexturePositionHandle, 3, GL_FLOAT, GL_FALSE, TRIANGLE_VERTICES_DATA_STRIDE_BYTES, coords); checkGlError("glVertexAttribPointer");
	glVertexAttribPointer(gvTextureTexCoordsHandle, 2, GL_FLOAT, GL_FALSE, TRIANGLE_VERTICES_DATA_STRIDE_BYTES, &coords[3]); checkGlError("glVertexAttribPointer");
	glEnableVertexAttribArray(gvTexturePositionHandle); checkGlError("glEnableVertexAttribArray");
	glEnableVertexAttribArray(gvTextureTexCoordsHandle); checkGlError("glEnableVertexAttribArray");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError("glDrawArrays");

	glUseProgram(0); checkGlError("glUseProgram");
}

static GLuint gPixelsTexture;

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * data){
    if(!gPixelsTexture) glGenTextures(1, &gPixelsTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPixelsTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, type, data);
    glDrawTex();
    //glDrawTexCoords(-1.0, -1.0, 1.0, 1.0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
