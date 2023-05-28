#pragma once
#define unused __attribute__((unused))

#ifdef __cplusplus
extern "C" {
#endif

void tx11_protocol_init(void);

void lorieChangeWindow(struct ANativeWindow* win);
void lorieConfigureNotify(int width, int height);

void init_module(void);

#ifdef __cplusplus
}
#endif
