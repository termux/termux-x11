#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define unused __attribute__((unused))

/* Can't get these from <GL/glx.h> since it pulls in client headers */
#define GLX_RGBA_BIT		0x00000001
#define GLX_WINDOW_BIT		0x00000001
#define GLX_PIXMAP_BIT		0x00000002
#define GLX_PBUFFER_BIT		0x00000004
#define GLX_NONE                0x8000
#define GLX_SLOW_CONFIG         0x8001
#define GLX_TRUE_COLOR		0x8002
#define GLX_DIRECT_COLOR	0x8003
#define GLX_NON_CONFORMANT_CONFIG 0x800D
#define GLX_DONT_CARE           0xFFFFFFFF
#define GLX_RGBA_FLOAT_BIT_ARB  0x00000004
#define GLX_SWAP_UNDEFINED_OML  0x8063

/* These functions will never be invoked since indirect GLX is not supported by server... */

void glDeleteLists(unused  GLuint list, unused GLsizei range) {}
GLuint glGenLists(unused GLsizei range) { return 0; }
void glNewList(unused GLuint list, unused GLenum mode) {}
void glEndList(void) {}
void glCallList(unused GLuint list) {}
void glCallLists(unused GLsizei n, unused GLenum type, unused const GLvoid *lists) {}
void glListBase(unused GLuint base) {}
void glBegin(unused GLenum mode) {}
void glEnd(void) {}
void glBitmap(unused GLsizei width, unused GLsizei height, unused GLfloat xorig, unused GLfloat yorig, unused GLfloat xmove, unused GLfloat ymove, unused const GLubyte *bitmap ) {}
void glColor3bv(unused const GLbyte *v) {}
void glColor3dv(unused const GLdouble *v) {}
void glColor3fv(unused const GLfloat *v) {}
void glColor3iv(unused const GLint *v) {}
void glColor3sv(unused const GLshort *v) {}
void glColor3ubv(unused const GLubyte *v) {}
void glColor3uiv(unused const GLuint *v) {}
void glColor3usv(unused const GLushort *v) {}
void glColor4bv(unused const GLbyte *v) {}
void glColor4dv(unused const GLdouble *v) {}
void glColor4fv(unused const GLfloat *v) {}
void glColor4iv(unused const GLint *v) {}
void glColor4sv(unused const GLshort *v) {}
void glColor4ubv(unused const GLubyte *v) {}
void glColor4uiv(unused const GLuint *v) {}
void glColor4usv(unused const GLushort *v) {}
void glEdgeFlagv(unused const GLboolean *flag) {}
void glIndexdv(unused const GLdouble *c) {}
void glIndexfv(unused const GLfloat *c) {}
void glIndexiv(unused const GLint *c) {}
void glIndexsv(unused const GLshort *c) {}
void glNormal3bv(unused const GLbyte *v) {}
void glNormal3dv(unused const GLdouble *v) {}
void glNormal3fv(unused const GLfloat *v) {}
void glNormal3iv(unused const GLint *v) {}
void glNormal3sv(unused const GLshort *v) {}
void glRasterPos2dv(unused const GLdouble *v) {}
void glRasterPos2fv(unused const GLfloat *v) {}
void glRasterPos2iv(unused const GLint *v) {}
void glRasterPos2sv(unused const GLshort *v) {}
void glRasterPos3dv(unused const GLdouble *v) {}
void glRasterPos3fv(unused const GLfloat *v) {}
void glRasterPos3iv(unused const GLint *v) {}
void glRasterPos3sv(unused const GLshort *v) {}
void glRasterPos4dv(unused const GLdouble *v) {}
void glRasterPos4fv(unused const GLfloat *v) {}
void glRasterPos4iv(unused const GLint *v) {}
void glRasterPos4sv(unused const GLshort *v) {}
void glRectdv(unused const GLdouble *v1, unused const GLdouble *v2) {}
void glRectfv(unused const GLfloat *v1, unused const GLfloat *v2) {}
void glRectiv(unused const GLint *v1, unused const GLint *v2) {}
void glRectsv(unused const GLshort *v1, unused const GLshort *v2) {}
void glTexCoord1dv(unused const GLdouble *v) {}
void glTexCoord1fv(unused const GLfloat *v) {}
void glTexCoord1iv(unused const GLint *v) {}
void glTexCoord1sv(unused const GLshort *v) {}
void glTexCoord2dv(unused const GLdouble *v) {}
void glTexCoord2fv(unused const GLfloat *v) {}
void glTexCoord2iv(unused const GLint *v) {}
void glTexCoord2sv(unused const GLshort *v) {}
void glTexCoord3dv(unused const GLdouble *v) {}
void glTexCoord3fv(unused const GLfloat *v) {}
void glTexCoord3iv(unused const GLint *v) {}
void glTexCoord3sv(unused const GLshort *v) {}
void glTexCoord4dv(unused const GLdouble *v) {}
void glTexCoord4fv(unused const GLfloat *v) {}
void glTexCoord4iv(unused const GLint *v) {}
void glTexCoord4sv(unused const GLshort *v) {}
void glVertex2dv(unused const GLdouble *v) {}
void glVertex2fv(unused const GLfloat *v) {}
void glVertex2iv(unused const GLint *v) {}
void glVertex2sv(unused const GLshort *v) {}
void glVertex3dv(unused const GLdouble *v) {}
void glVertex3fv(unused const GLfloat *v) {}
void glVertex3iv(unused const GLint *v) {}
void glVertex3sv(unused const GLshort *v) {}
void glVertex4dv(unused const GLdouble *v) {}
void glVertex4fv(unused const GLfloat *v) {}
void glVertex4iv(unused const GLint *v) {}
void glVertex4sv(unused const GLshort *v) {}
void glClipPlane(unused GLenum plane, unused const GLdouble *equation) {}
void glColorMaterial(unused GLenum face, unused GLenum mode) {}
void glFogf(unused GLenum pname, unused GLfloat param) {}
void glFogi(unused GLenum pname, unused GLint param) {}
void glFogfv(unused GLenum pname, unused const GLfloat *params) {}
void glFogiv(unused GLenum pname, unused const GLint *params) {}
void glLightf(unused GLenum light, unused GLenum pname, unused GLfloat param) {}
void glLighti(unused GLenum light, unused GLenum pname, unused GLint param) {}
void glLightfv(unused GLenum light, unused GLenum pname, unused const GLfloat *params) {}
void glLightiv(unused GLenum light, unused GLenum pname, unused const GLint *params) {}
void glLightModelf(unused GLenum pname, unused GLfloat param) {}
void glLightModeli(unused GLenum pname, unused GLint param) {}
void glLightModelfv(unused GLenum pname, unused const GLfloat *params) {}
void glLightModeliv(unused GLenum pname, unused const GLint *params) {}
void glLineStipple(unused GLint factor, unused GLushort pattern) {}
void glMaterialf(unused GLenum face, unused GLenum pname, unused GLfloat param) {}
void glMateriali(unused GLenum face, unused GLenum pname, unused GLint param) {}
void glMaterialfv(unused GLenum face, unused GLenum pname, unused const GLfloat *params) {}
void glMaterialiv(unused GLenum face, unused GLenum pname, unused const GLint *params) {}
void glPointSize(unused GLfloat size) {}
void glPolygonMode(unused GLenum face, unused GLenum mode) {}
void glPolygonStipple(unused const GLubyte *mask) {}
void glShadeModel(unused GLenum mode) {}
void glTexImage1D(unused GLenum target, unused GLint level, unused GLint internalFormat, unused GLsizei width, unused GLint border, unused GLenum format, unused GLenum type, unused const GLvoid *pixels) {}
void glTexEnvf(unused GLenum target, unused GLenum pname, unused GLfloat param) {}
void glTexEnvfv(unused GLenum target, unused GLenum pname, unused const GLfloat *params) {}
void glTexEnvi(unused GLenum target, unused GLenum pname, unused GLint param) {}
void glTexEnviv(unused GLenum target, unused GLenum pname, unused const GLint *params) {}
void glTexGend(unused GLenum coord, unused GLenum pname, unused GLdouble param) {}
void glTexGendv(unused GLenum coord, unused GLenum pname, unused const GLdouble *params) {}
void glTexGenf(unused GLenum coord, unused GLenum pname, unused GLfloat param) {}
void glTexGenfv(unused GLenum coord, unused GLenum pname, unused const GLfloat *params) {}
void glTexGeni(unused GLenum coord, unused GLenum pname, unused GLint param) {}
void glTexGeniv(unused GLenum coord, unused GLenum pname, unused const GLint *params) {}
void glPassThrough(unused GLfloat token) {}
void glInitNames(unused void) {}
void glLoadName(unused GLuint name) {}
void glPushName(unused GLuint name) {}
void glPopName(unused void) {}
void glDrawBuffer(unused GLenum mode) {}
void glClearAccum(unused GLfloat red, unused GLfloat green, unused GLfloat blue, unused GLfloat alpha) {}
void glAccum(unused GLenum op, unused GLfloat value) {}
void glClearIndex(unused GLfloat c) {}
void glClearDepth(unused GLclampd depth) {}
void glIndexMask(unused GLuint mask) {}
void glPushAttrib(unused GLbitfield mask) {}
void glPopAttrib(void) {}
void glMapGrid1d(unused GLint un, unused GLdouble u1, unused GLdouble u2) {}
void glMapGrid1f(unused GLint un, unused GLfloat u1, unused GLfloat u2) {}
void glMapGrid2d(unused GLint un, unused GLdouble u1, unused GLdouble u2, unused GLint vn, unused GLdouble v1, unused GLdouble v2) {}
void glMapGrid2f(unused GLint un, unused GLfloat u1, unused GLfloat u2, unused GLint vn, unused GLfloat v1, unused GLfloat v2) {}
void glEvalCoord1dv(unused const GLdouble *u) {}
void glEvalCoord1fv(unused const GLfloat *u) {}
void glEvalCoord2dv(unused const GLdouble *u) {}
void glEvalCoord2fv(unused const GLfloat *u) {}
void glEvalPoint1(unused GLint i) {}
void glEvalPoint2(unused GLint i, unused GLint j) {}
void glEvalMesh1(unused GLenum mode, unused GLint i1, unused GLint i2) {}
void glEvalMesh2(unused GLenum mode, unused GLint i1, unused GLint i2, unused GLint j1, unused GLint j2) {}
void glAlphaFunc(unused GLenum func, unused GLclampf ref) {}
void glLogicOp(unused GLenum opcode) {}
void glPixelZoom(unused GLfloat xfactor, unused GLfloat yfactor) {}
void glPixelTransferf(unused GLenum pname, unused GLfloat param) {}
void glPixelTransferi(unused GLenum pname, unused GLint param) {}
void glPixelStoref(unused GLenum pname, unused GLfloat param) {}
void glPixelMapfv(unused GLenum map, unused GLsizei mapsize, unused const GLfloat *values) {}
void glPixelMapuiv(unused GLenum map, unused GLsizei mapsize, unused const GLuint *values) {}
void glPixelMapusv(unused GLenum map, unused GLsizei mapsize, unused const GLushort *values) {}
void glReadBuffer(unused GLenum mode) {}
void glDrawPixels(unused GLsizei width, unused GLsizei height, unused GLenum format, unused GLenum type, unused const GLvoid *pixels) {}
void glCopyPixels(unused GLint x, unused GLint y, unused GLsizei width, unused GLsizei height, unused GLenum type) {}
void glGetClipPlane(unused GLenum plane, unused GLdouble *equation) {}
void glGetDoublev(unused GLenum pname, unused GLdouble *params) {}
void glGetLightfv(unused GLenum light, unused GLenum pname, unused GLfloat *params) {}
void glGetLightiv(unused GLenum light, unused GLenum pname, unused GLint *params) {}
void glGetMapdv(unused GLenum target, unused GLenum query, unused GLdouble *v) {}
void glGetMapfv(unused GLenum target, unused GLenum query, unused GLfloat *v) {}
void glGetMapiv(unused GLenum target, unused GLenum query, unused GLint *v) {}
void glGetMaterialfv(unused GLenum face, unused GLenum pname, unused GLfloat *params) {}
void glGetMaterialiv(unused GLenum face, unused GLenum pname, unused GLint *params) {}
void glGetPixelMapfv(unused GLenum map, unused GLfloat *values) {}
void glGetPixelMapuiv(unused GLenum map, unused GLuint *values) {}
void glGetPixelMapusv(unused GLenum map, unused GLushort *values) {}
void glGetTexEnvfv(unused GLenum target, unused GLenum pname, unused GLfloat *params) {}
void glGetTexEnviv(unused GLenum target, unused GLenum pname, unused GLint *params) {}
void glGetTexGendv(unused GLenum coord, unused GLenum pname, unused GLdouble *params) {}
void glGetTexGenfv(unused GLenum coord, unused GLenum pname, unused GLfloat *params) {}
void glGetTexGeniv(unused GLenum coord, unused GLenum pname, unused GLint *params) {}
void glGetTexLevelParameterfv(unused GLenum target, unused GLint level, unused GLenum pname, unused GLfloat *params) {}
void glGetTexLevelParameteriv(unused GLenum target, unused GLint level, unused GLenum pname, unused GLint *params) {}
GLboolean glIsList(unused GLuint list) { return FALSE; }
void glDepthRange(unused GLclampd near_val, unused GLclampd far_val) {}
void glFrustum(unused GLdouble left, unused GLdouble right, unused GLdouble bottom, unused GLdouble top, unused GLdouble near_val, unused GLdouble far_val) {}
void glLoadIdentity(unused void) {}
void glLoadMatrixf(unused const GLfloat *m) {}
void glLoadMatrixd(unused const GLdouble *m) {}
void glMultMatrixf(unused const GLfloat *m) {}
void glMultMatrixd(unused const GLdouble *m) {}
void glMatrixMode(unused GLenum mode) {}
void glOrtho(unused GLdouble left, unused GLdouble right, unused GLdouble bottom, unused GLdouble top, unused GLdouble near_val, unused GLdouble far_val) {}
void glPushMatrix(unused void) {}
void glPopMatrix(unused void) {}
void glRotated(unused GLdouble angle, unused GLdouble x, unused GLdouble y, unused GLdouble z) {}
void glRotatef(unused GLfloat angle, unused GLfloat x, unused GLfloat y, unused GLfloat z) {}
void glScaled(unused GLdouble x, unused GLdouble y, unused GLdouble z) {}
void glScalef(unused GLfloat x, unused GLfloat y, unused GLfloat z) {}
void glTranslated(unused GLdouble x, unused GLdouble y, unused GLdouble z) {}
void glTranslatef(unused GLfloat x, unused GLfloat y, unused GLfloat z) {}
void glIndexubv(unused const GLubyte *c) {}
GLAPI GLboolean GLAPIENTRY glAreTexturesResident(unused GLsizei n, unused const GLuint *textures, unused GLboolean *residences) { return FALSE; }
void glCopyTexImage1D(unused GLenum target, unused GLint level, unused GLenum internalformat, unused GLint x, unused GLint y, unused GLsizei width, unused GLint border) {}
void glCopyTexSubImage1D(unused GLenum target, unused GLint level, unused GLint xoffset, unused GLint x, unused GLint y, unused GLsizei width) {}
void glPrioritizeTextures(unused GLsizei n, unused const GLuint *textures, unused const GLclampf *priorities) {}
void glTexSubImage1D(unused GLenum target, unused GLint level, unused GLint xoffset, unused GLsizei width, unused GLenum format, unused GLenum type, unused const GLvoid *pixels) {}
void glColorTable(unused GLenum target, unused GLenum internalformat, unused GLsizei width, unused GLenum format, unused GLenum type, unused const GLvoid *table) {}
void glColorTableParameteriv(unused GLenum target, unused GLenum pname, unused const GLint *params) {}
void glColorTableParameterfv(unused GLenum target, unused GLenum pname, unused const GLfloat *params) {}
void glCopyColorTable(unused GLenum target, unused GLenum internalformat, unused GLint x, unused GLint y, unused GLsizei width) {}
void glGetColorTableParameterfv(unused GLenum target, unused GLenum pname, unused GLfloat *params) {}
void glGetColorTableParameteriv(unused GLenum target, unused GLenum pname, unused GLint *params) {}
void glColorSubTable(unused GLenum target, unused GLsizei start, unused GLsizei count, unused GLenum format, unused GLenum type, unused const GLvoid *data) {}
void glCopyColorSubTable(unused GLenum target, unused GLsizei start, unused GLint x, unused GLint y, unused GLsizei width) {}
void glConvolutionFilter1D(unused GLenum target, unused GLenum internalformat, unused GLsizei width, unused GLenum format, unused GLenum type, unused const GLvoid *image) {}
void glConvolutionFilter2D(unused GLenum target, unused GLenum internalformat, unused GLsizei width, unused GLsizei height, unused GLenum format, unused GLenum type, unused const GLvoid *image) {}
void glConvolutionParameterf(unused GLenum target, unused GLenum pname, unused GLfloat params) {}
void glConvolutionParameterfv(unused GLenum target, unused GLenum pname, unused const GLfloat *params) {}
void glConvolutionParameteri(unused GLenum target, unused GLenum pname, unused GLint params) {}
void glConvolutionParameteriv(unused GLenum target, unused GLenum pname, unused const GLint *params) {}
void glCopyConvolutionFilter1D(unused GLenum target, unused GLenum internalformat, unused GLint x, unused GLint y, unused GLsizei width) {}
void glCopyConvolutionFilter2D(unused GLenum target, unused GLenum internalformat, unused GLint x, unused GLint y, unused GLsizei width, unused GLsizei height) {}
void glGetConvolutionParameterfv(unused GLenum target, unused GLenum pname, unused GLfloat *params) {}
void glGetConvolutionParameteriv(unused GLenum target, unused GLenum pname, unused GLint *params) {}
void glGetHistogramParameterfv(unused GLenum target, unused GLenum pname, unused GLfloat *params) {}
void glGetHistogramParameteriv(unused GLenum target, unused GLenum pname, unused GLint *params) {}
void glGetMinmaxParameterfv(unused GLenum target, unused GLenum pname, unused GLfloat *params) {}
void glGetMinmaxParameteriv(unused GLenum target, unused GLenum pname, unused GLint *params) {}
void glHistogram(unused GLenum target, unused GLsizei width, unused GLenum internalformat, unused GLboolean sink) {}
void glResetHistogram(unused GLenum target) {}
void glMinmax(unused GLenum target, unused GLenum internalformat, unused GLboolean sink) {}
void glResetMinmax(unused GLenum target) {}
void glTexImage3D(unused GLenum target, unused GLint level, unused GLint internalFormat, unused GLsizei width, unused GLsizei height, unused GLsizei depth, unused GLint border, unused GLenum format, unused GLenum type, unused const GLvoid *pixels) {}
void glTexSubImage3D(unused GLenum target, unused GLint level, unused GLint xoffset, unused GLint yoffset, unused GLint zoffset, unused GLsizei width, unused GLsizei height, unused GLsizei depth, unused GLenum format, unused GLenum type, unused const GLvoid *pixels) {}
void glCopyTexSubImage3D(unused GLenum target, unused GLint level, unused GLint xoffset, unused GLint yoffset, unused GLint zoffset, unused GLint x, unused GLint y, unused GLsizei width, unused GLsizei height) {}
void glActiveTextureARB(unused GLenum texture) {}
void glMultiTexCoord1dvARB(unused GLenum target, unused const GLdouble *v) {}
void glMultiTexCoord1fvARB(unused GLenum target, unused const GLfloat *v) {}
void glMultiTexCoord1ivARB(unused GLenum target, unused const GLint *v) {}
void glMultiTexCoord1svARB(unused GLenum target, unused const GLshort *v) {}
void glMultiTexCoord2dvARB(unused GLenum target, unused const GLdouble *v) {}
void glMultiTexCoord2fvARB(unused GLenum target, unused const GLfloat *v) {}
void glMultiTexCoord2ivARB(unused GLenum target, unused const GLint *v) {}
void glMultiTexCoord2svARB(unused GLenum target, unused const GLshort *v) {}
void glMultiTexCoord3dvARB(unused GLenum target, unused const GLdouble *v) {}
void glMultiTexCoord3fvARB(unused GLenum target, unused const GLfloat *v) {}
void glMultiTexCoord3ivARB(unused GLenum target, unused const GLint *v) {}
void glMultiTexCoord3svARB(unused GLenum target, unused const GLshort *v) {}
void glMultiTexCoord4dvARB(unused GLenum target, unused const GLdouble *v) {}
void glMultiTexCoord4fvARB(unused GLenum target, unused const GLfloat *v) {}
void glMultiTexCoord4ivARB(unused GLenum target, unused const GLint *v) {}
void glMultiTexCoord4svARB(unused GLenum target, unused const GLshort *v) {}
void glMap1d(unused GLenum target, unused GLdouble u1, unused GLdouble u2, unused GLint stride, unused GLint order, unused const GLdouble *points) {}
void glMap1f(unused GLenum target, unused GLfloat u1, unused GLfloat u2, unused GLint stride, unused GLint order, unused const GLfloat *points) {}
void glMap2d(unused GLenum target, unused GLdouble u1, unused GLdouble u2, unused GLint ustride, unused GLint uorder, unused GLdouble v1, unused GLdouble v2, unused GLint vstride, unused GLint vorder, unused const GLdouble *points) {}
void glMap2f(unused GLenum target, unused GLfloat u1, unused GLfloat u2, unused GLint ustride, unused GLint uorder, unused GLfloat v1, unused GLfloat v2, unused GLint vstride, unused GLint vorder, unused const GLfloat *points) {}
void glEnableClientState(unused GLenum cap) {}
void glVertexPointer(unused GLint size, unused GLenum type, unused GLsizei stride, unused const GLvoid *ptr) {}
void glNormalPointer(unused GLenum type, unused GLsizei stride, unused const GLvoid *ptr) {}
void glColorPointer(unused GLint size, unused GLenum type, unused GLsizei stride, unused const GLvoid *ptr) {}
void glIndexPointer(unused GLenum type, unused GLsizei stride, unused const GLvoid *ptr) {}
void glTexCoordPointer(unused GLint size, unused GLenum type, unused GLsizei stride, unused const GLvoid *ptr) {}
void glEdgeFlagPointer(unused GLsizei stride, unused const GLvoid *ptr) {}
void glDisableClientState(unused GLenum cap) {}
void glSeparableFilter2D(unused GLenum target, unused GLenum internalformat, unused GLsizei width, unused GLsizei height, unused GLenum format, unused GLenum type, unused const GLvoid *row, unused const GLvoid *column) {}
void glFeedbackBuffer(unused GLsizei size, unused GLenum type, unused GLfloat *buffer) {}
void glSelectBuffer(unused GLsizei size, unused GLuint *buffer) {}
GLint glRenderMode(unused GLenum mode) { return 0; }
void glGetTexImage(unused GLenum target, unused GLint level, unused GLenum format, unused GLenum type, unused GLvoid *pixels) {}
void glGetPolygonStipple(unused GLubyte *mask) {}
void glGetSeparableFilter(unused GLenum target, unused GLenum format, unused GLenum type, unused GLvoid *row, unused GLvoid *column, unused GLvoid *span) {}
void glGetConvolutionFilter(unused GLenum target, unused GLenum format, unused GLenum type, unused GLvoid *image) {}
void glGetHistogram(unused GLenum target, unused GLboolean reset, unused GLenum format, unused GLenum type, unused GLvoid *values) {}
void glGetMinmax(unused GLenum target, unused GLboolean reset, unused GLenum format, unused GLenum types, unused GLvoid *values) {}
void glGetColorTable(unused GLenum target, unused GLenum format, unused GLenum type, unused GLvoid *table) {}
void *glXGetProcAddressARB(unused const char *symbol);
void *glXGetProcAddressARB(unused const char *symbol) { return NULL; } 
 
