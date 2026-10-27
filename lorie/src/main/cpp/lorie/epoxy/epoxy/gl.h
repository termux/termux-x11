#pragma once
#include <stdbool.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

struct lorieGles3Functions {
    PFNGLBINDVERTEXARRAYPROC bindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays;
    PFNGLGENVERTEXARRAYSPROC genVertexArrays;
    PFNGLDRAWARRAYSINSTANCEDPROC drawArraysInstanced;
    PFNGLMAPBUFFERRANGEPROC mapBufferRange;
    PFNGLUNMAPBUFFERPROC unmapBuffer;
    PFNGLUNIFORM4UIPROC uniform4ui;
    PFNGLVERTEXATTRIBDIVISORPROC vertexAttribDivisor;
    PFNGLDEBUGMESSAGECALLBACKPROC debugMessageCallback;
    PFNGLDEBUGMESSAGECONTROLPROC debugMessageControl;
    PFNGLOBJECTLABELPROC objectLabel;
};
extern struct lorieGles3Functions lorieGles3;

#define glBindVertexArray (lorieGles3.bindVertexArray)
#define glDeleteVertexArrays (lorieGles3.deleteVertexArrays)
#define glGenVertexArrays (lorieGles3.genVertexArrays)
#define glDrawArraysInstanced (lorieGles3.drawArraysInstanced)
#define glMapBufferRange (lorieGles3.mapBufferRange)
#define glUnmapBuffer (lorieGles3.unmapBuffer)
#define glUniform4ui (lorieGles3.uniform4ui)
#define glVertexAttribDivisor (lorieGles3.vertexAttribDivisor)
#define glDebugMessageCallback (lorieGles3.debugMessageCallback)
#define glDebugMessageControl (lorieGles3.debugMessageControl)
#define glObjectLabel (lorieGles3.objectLabel)

int epoxy_has_gl_extension(const char *extension);
int epoxy_gl_version(void);
int epoxy_is_desktop_gl(void);

/* Desktop GL tokens and entry points that glamor names but only uses when the context is desktop GL. */
#define GL_BGRA 0x80E1
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_CLEAR 0x1500
#define GL_AND 0x1501
#define GL_AND_REVERSE 0x1502
#define GL_AND_INVERTED 0x1504
#define GL_NOOP 0x1505
#define GL_XOR 0x1506
#define GL_OR 0x1507
#define GL_NOR 0x1508
#define GL_EQUIV 0x1509
#define GL_OR_REVERSE 0x150B
#define GL_COPY_INVERTED 0x150C
#define GL_OR_INVERTED 0x150D
#define GL_NAND 0x150E
#define GL_SET 0x150F
#define GL_SRC1_COLOR 0x88F9
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_MAP_PERSISTENT_BIT 0x0040
#define GL_MAP_COHERENT_BIT 0x0080

void glTextureBarrierNV(void);
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
void glLogicOp(GLenum op);
void *glMapBuffer(GLenum target, GLenum access);
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const char *name);
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
