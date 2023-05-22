#pragma once
#define unused __attribute__((unused))

#ifdef __cplusplus
extern "C" {
#endif

void tx11_protocol_init(void);

void lorieChangeWindow(struct ANativeWindow* win);
void lorieConfigureNotify(int width, int height, int dpi);

#ifdef __cplusplus
}
#endif
