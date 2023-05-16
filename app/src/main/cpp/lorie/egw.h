#pragma once
#define unused __attribute__((unused))

#define DE_STOP 8

#ifdef __cplusplus
extern "C" {
#endif

extern const char *LogInit(const char *fname, const char *backup);
extern int SetNotifyFd(int fd, void (*notify)(int,int,void*), int mask, void *data);
extern int dix_main(int argc, char *argv[], char *envp[]);

void egw_init(void);
void tx11_protocol_init(void);

void vfbChangeWindow(struct ANativeWindow* win);
void vfbConfigureNotify(int width, int height, int dpi);
void vfbMotionNotify(int x, int y);
void vfbButtonNotify(int button, int state);
void vfbKeyNotify(int key, int state);

#ifdef __cplusplus
}
#endif
