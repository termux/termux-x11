#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jni.h>
#include <fcntl.h>

#define unused __attribute__((__unused__))

void DumpHex(const void* data, size_t size);

typedef struct {
    int fds[2];
    int position;
    uint8_t data[0];
} nativePipe;

static long toLong(nativePipe* p) {
    union {
        jlong l;
        nativePipe* p;
    } u = {0};
    u.p = p;
    return u.l;
}

static nativePipe* fromLong(jlong v) {
    union {
        jlong l;
        nativePipe* p;
    } u = {0};
    u.l = v;
    return u.p;
}

JNIEXPORT jlong JNICALL
Java_com_termux_x11_starter_ExecHelper_createPipe(unused JNIEnv *env, unused jclass clazz,
                                                  jint capacity) {
    size_t size = sizeof(nativePipe) + sizeof(uint8_t) * capacity + 1;
    nativePipe* p = malloc(size);
    memset(p, 0, size);
    if (p != NULL) pipe(p->fds);
    int flags = fcntl(p->fds[0], F_GETFL, 0);
    fcntl(p->fds[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(p->fds[1], F_GETFL, 0);
    fcntl(p->fds[1], F_SETFL, flags | O_NONBLOCK);
    return toLong(p);
}

JNIEXPORT jint JNICALL
Java_com_termux_x11_starter_ExecHelper_pipeWriteFd(unused JNIEnv *env, unused jclass clazz,
                                                  jlong jp) {
    nativePipe* p = fromLong(jp);
    if (!p)
        return -1;
    return p->fds[1];
}

JNIEXPORT void JNICALL
Java_com_termux_x11_starter_ExecHelper_flushPipe(unused JNIEnv *env, unused jclass clazz, jlong jp) {
    nativePipe* p = fromLong(jp);
    if (!p)
        return;

    int bytesRead = read(p->fds[0], p->data + p->position, 1024);
    p->position += bytesRead;
}

JNIEXPORT void JNICALL
Java_com_termux_x11_starter_ExecHelper_performExec(unused JNIEnv *env, unused jclass clazz, jlong jp) {
    nativePipe* p = fromLong(jp);
    if (!p)
        return;
    //DumpHex(p->data, p->position);
    char *argv[1024] = {0};
    argv[0] = (char*) p->data;
    for (int i = 0, j = 1; i < p->position; i++) {
        if (p->data[i] == 0 && i+1 < p->position) {;
            argv[j++] = (char*) p->data + i + 1;
        }
    }

    for (int i=0; i<1024; i++) {
        if (argv[i]) {
            printf("argv[%d] = %s \n", i, argv[i]);
            DumpHex(argv[i], strlen(argv[i]));
        }
    }
    execv(argv[0], argv);
    perror("execv");
    exit (1);
}

void DumpHex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}
#pragma clang diagnostic pop