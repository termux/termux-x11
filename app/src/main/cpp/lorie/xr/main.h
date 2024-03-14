
#include <stdbool.h>

void bindXrFramebuffer(void);
bool beginXrFrame(void);
void endXrFrame(void);
void enterXr(void);
void getXrResolution(int *width, int *height);
bool isXrEnabled(void);
