#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define unused __attribute__((__unused__))

#define DEFAULT_PREFIX "/data/data/com.termux/files/usr"
#define DEFAULT_XDG_RUNTIME_DIR DEFAULT_PREFIX "/tmp"
#define DEFAULT_SOCKET_NAME "termux-x11"

static int socket_action(int* fd, char* path,
                         int (*action)(int, const struct sockaddr *, socklen_t)) {
    if (!fd || !action) {
        errno = -EINVAL;
        return -1;
    }

    struct sockaddr_un local;
    size_t len;

    if((*fd = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return *fd;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    return action(*fd, (struct sockaddr *)&local, len);
}

JNIEXPORT void JNICALL
Java_com_termux_x11_starter_Starter_checkXdgRuntimeDir(unused JNIEnv *env, unused jobject thiz) {
    char* XDG_RUNTIME_DIR = getenv("XDG_RUNTIME_DIR");
    if (!XDG_RUNTIME_DIR || strlen(XDG_RUNTIME_DIR) == 0) {
        printf("$XDG_RUNTIME_DIR is unset.\n");
        printf("Exporting default value (%s).\n", DEFAULT_XDG_RUNTIME_DIR);
        setenv("XDG_RUNTIME_DIR", DEFAULT_XDG_RUNTIME_DIR, true);
    }
}

static const char *getWaylandSocketPath() {
    static char* path = NULL;
    if (path != NULL)
        return path;

    path = malloc(256 * sizeof(char));
    memset(path, 0, 256 * sizeof(char));

    char* XDG_RUNTIME_DIR = getenv("XDG_RUNTIME_DIR");
    if (!XDG_RUNTIME_DIR || strlen(XDG_RUNTIME_DIR) == 0) {
        printf("$XDG_RUNTIME_DIR is unset");
        exit(1);
    }

    sprintf(path, "%s/%s", XDG_RUNTIME_DIR, DEFAULT_SOCKET_NAME);
    return path;
}

JNIEXPORT jboolean JNICALL
Java_com_termux_x11_starter_Starter_checkWaylandSocket(unused JNIEnv *env, unused jobject thiz) {
    int fd;
    errno = 0;

    if (socket_action(&fd, (char *) getWaylandSocketPath(), connect) != -1) {
        close(fd);
        return 1;
    }

    return 0;
}

JNIEXPORT jint JNICALL
Java_com_termux_x11_starter_Starter_createWaylandSocket(unused JNIEnv *env, unused jobject thiz) {
    int fd;
    errno = 0;

    unlink(getWaylandSocketPath());
    if (socket_action(&fd, (char *) getWaylandSocketPath(), bind) < 0) {
        perror("socket");
        return -1;
    }

    return fd;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
JNIEXPORT jint JNICALL
Java_com_termux_x11_starter_Starter_openLogFD(unused JNIEnv *env, unused jobject thiz) {
    const char* TERMUX_X11_LOG_FILE = getenv("TERMUX_X11_LOG_FILE");
    int sv[2]; /* the pair of socket descriptors */
    if (TERMUX_X11_LOG_FILE == NULL || strlen(TERMUX_X11_LOG_FILE) == 0)
        return -1;

    int logfd = open(TERMUX_X11_LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (logfd < 0) {
        perror("open logfile");
        return -1;
    }

    if (pipe(sv) == -1) {
        perror("pipe");
        return -1;
    }

    fchmod (sv[0], 0777);
    fchmod (sv[1], 0777);

    switch(fork()) {
        /*
         * Android do not allow another process to write to tty or pipe fd of our process.
         * We can not force allowing even using chmod.
         * That is a reason we are using pipe and cat here.
         */
        case 0: {
                int new_stderr = dup(2);
                close(sv[1]);
                dup2(sv[0], 0);
                dup2(logfd, 1);
                dup2(logfd, 2);
                close(sv[0]);
                close(logfd);

                printf("cat started (%d)\n", getpid());

                struct pollfd pfd = {0};
                pfd.fd = 0;
                pfd.events = POLLIN | POLLHUP;

                poll(&pfd, 1, 10000);

                execl("/data/data/com.termux/files/usr/bin/cat", "cat", NULL);
                dprintf(new_stderr, "execl cat: %s\n", strerror(errno));
                return -1;
            }
        case -1:
            return -1;
        default:
            close(sv[0]);
            close(logfd);
            return sv[1];
    }
}

JNIEXPORT void JNICALL
Java_com_termux_x11_starter_Starter_exec(JNIEnv *env, jclass clazz, jstring jpath, jobjectArray jargv) {
    // execv's argv array is a bit incompatible with Java's String[], so we do some converting here...
    int argc = (*env)->GetArrayLength(env, jargv) + 2; // Leading executable path and terminating NULL
    char *argv[argc];
    memset(argv, 0, sizeof(char*) * argc);

    for(int i=0; i<argc-1; i++)
    {
        jstring js = i ? (*env)->GetObjectArrayElement(env, jargv, i - 1) : jpath;
        const char *pjc = (*env)->GetStringUTFChars(env, js, false);
        argv[i] = calloc(strlen(pjc)+1, sizeof(char)); //Extra char for the terminating NULL
        strcpy((char *) argv[i], pjc);
        (*env)->ReleaseStringUTFChars(env, js, pjc);
    }
    argv[argc] = NULL; // Terminating NULL
    setenv("WAYLAND_DISPLAY", DEFAULT_SOCKET_NAME, 1);

    execv(argv[0], argv);
    perror("execv");
    exit(1);
}

#pragma clang diagnostic pop
