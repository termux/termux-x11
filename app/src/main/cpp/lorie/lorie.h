#pragma once
#define unused __attribute__((unused))

#ifdef __cplusplus
extern "C" {
#endif

void tx11_protocol_init(void);

Bool lorieChangeWindow(ClientPtr pClient, void *closure);
void lorieConfigureNotify(int width, int height, int framerate);

#ifdef __cplusplus
}
#endif
