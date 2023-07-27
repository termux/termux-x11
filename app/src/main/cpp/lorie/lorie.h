#pragma once
#define unused __attribute__((unused))

#ifdef __cplusplus
extern "C" {
#endif

Bool lorieChangeWindow(ClientPtr pClient, void *closure);
void lorieConfigureNotify(int width, int height, int framerate);
void lorieEnableClipboardSync(Bool enable);
void lorieSendClipboardData(const char* data);

#ifdef __cplusplus
}
#endif
