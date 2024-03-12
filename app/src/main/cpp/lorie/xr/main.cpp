#include <cstring>
#include <vector>

#include "base.h"
#include "input.h"
#include "math.h"
#include "renderer.h"

Base* s_module_base = NULL;
Input* s_module_input = NULL;
Renderer* s_module_renderer = NULL;

#if defined(_DEBUG)
#include <GLES3/gl3.h>
void GLCheckErrors(const char* file, int line) {
	for (int i = 0; i < 10; i++) {
		const GLenum error = glGetError();
		if (error == GL_NO_ERROR) {
			break;
		}
		ALOGE("OpenGL error on line %s:%d %d", file, line, error);
	}
}

void OXRCheckErrors(XrResult result, const char* file, int line) {
	if (XR_FAILED(result)) {
		char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(s_module_base->GetInstance(), result, errorBuffer);
        ALOGE("OpenXR error on line %s:%d %s", file, line, errorBuffer);
	}
}
#endif
