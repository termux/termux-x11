#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pixman.h>
#include <stdbool.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <errno.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "misc.h"
#include "buffer.h"

struct LorieBuffer {
    int16_t refcount;
    LorieBuffer_Desc desc;

    int8_t locked;
    void* lockedData;

    // file descriptor of shared memory fragment for shared memory backed buffer
    int fd;

    GLuint id;
    EGLImage image;
};

__attribute__((unused))
static int memfd_create(const char *name, unsigned int flags) {
#ifndef __NR_memfd_create
#if defined __i386__
#define __NR_memfd_create 356
#elif defined __x86_64__
    #define __NR_memfd_create 319
#elif defined __arm__
#define __NR_memfd_create 385
#elif defined __aarch64__
#define __NR_memfd_create 279
#endif
#endif

#ifdef __NR_memfd_create
    return syscall(__NR_memfd_create, name, flags); // NOLINT(cppcoreguidelines-narrowing-conversions)
#else
    errno = ENOSYS;
	return -1;
#endif
}

static inline size_t alignToPage(size_t size) {
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return (size + page_size - 1) & ~(page_size - 1);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCallsOfFunction"
int LorieBuffer_createRegion(char const* name, size_t size) {
    int fd = memfd_create(name, MFD_CLOEXEC|MFD_ALLOW_SEALING);
    if (fd) {
        ftruncate (fd, size);
        return fd;
    }

    fd = open("/dev/ashmem", O_RDWR);
    if (fd < 0)
        return fd;

    char name_buffer[ASHMEM_NAME_LEN] = {0};
    strncpy(name_buffer, name, sizeof(name_buffer));
    name_buffer[sizeof(name_buffer)-1] = 0;

    int ret = ioctl(fd, ASHMEM_SET_NAME, name_buffer);
    if (ret < 0) goto error;

    ret = ioctl(fd, ASHMEM_SET_SIZE, size);
    if (ret < 0) goto error;

    return fd;
    error:
    close(fd);
    return ret;
}
#pragma clang diagnostic pop

__LIBC_HIDDEN__ LorieBuffer* LorieBuffer_allocate(int32_t width, int32_t height, int8_t format, int8_t type) {
    if (width <= 0 || height <= 0
        || (format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM && format != AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM)
        || (type != LORIEBUFFER_REGULAR && type != LORIEBUFFER_AHARDWAREBUFFER))
        return NULL;
    LorieBuffer* buffer = calloc(1, sizeof(*buffer));
    if (!buffer)
        return NULL;

    *buffer = (LorieBuffer) { .desc = { .width = width, .stride = width, .height = height, .format = format, .type = type }, .fd = -1 };
    __sync_fetch_and_add(&buffer->refcount, 1);
    if (type == LORIEBUFFER_REGULAR) {
        size_t size = alignToPage(width * height * sizeof(uint32_t));
        buffer->fd = LorieBuffer_createRegion("LorieBuffer", size);
        if (buffer->fd < 0)
            goto error;

        buffer->desc.data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, buffer->fd, 0);
        if (buffer->desc.data == NULL || buffer->desc.data == MAP_FAILED)
            goto error;
    } else if (type == LORIEBUFFER_AHARDWAREBUFFER) {
        AHardwareBuffer_Desc desc = { .width = width, .height = height, .format = format, .layers = 1,
                .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE };
        int err = AHardwareBuffer_allocate(&desc, &buffer->desc.buffer);
        if (err != 0) {
            dprintf(2, "FATAL: failed to allocate AHardwareBuffer (width %d height %d format %d): error %d\n", width, height, format, err);
            goto error;
        }
        AHardwareBuffer_describe(buffer->desc.buffer, &desc);
        buffer->desc.stride = desc.stride;
    } else
        goto error;

    return buffer;

    error:
    if (buffer->fd >= 0)
        close(buffer->fd);

    free(buffer);
    return NULL;
}

__LIBC_HIDDEN__ void __LorieBuffer_free(LorieBuffer* buffer) {
    if (!buffer)
        return;

    if (eglGetCurrentContext())
        glDeleteTextures(1, &buffer->id);

    if (eglGetCurrentDisplay() && buffer->image)
        eglDestroyImageKHR(eglGetCurrentDisplay(), buffer->image);

    if (buffer->desc.type == LORIEBUFFER_REGULAR) {
        if (buffer->desc.data)
            munmap (buffer->desc.data, alignToPage(buffer->desc.width * buffer->desc.height * sizeof(uint32_t)));
        if (buffer->fd != -1)
            close(buffer->fd);
    } else if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER) {
        if (buffer)
            AHardwareBuffer_release(buffer->desc.buffer);
    }

    free(buffer);
}

__LIBC_HIDDEN__ void LorieBuffer_describe(LorieBuffer* buffer, LorieBuffer_Desc* desc) {
    if (!desc)
        return;

    if (buffer)
        memcpy(desc, &buffer->desc, sizeof(*desc));
    else
        memset(desc, 0, sizeof(*desc));
}

__LIBC_HIDDEN__ int LorieBuffer_lock(LorieBuffer* buffer, AHardwareBuffer_Desc* outDesc, void** out) {
    int ret = 0;
    if (!buffer)
        return ENODEV;

    if (buffer->locked) {
        dprintf(2, "tried to lock already locked buffer\n");
        *out = buffer->lockedData;
        return EEXIST;
    }

    if (buffer->desc.type == LORIEBUFFER_REGULAR)
        buffer->lockedData = buffer->desc.data;
    else if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        ret = AHardwareBuffer_lock(buffer->desc.buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, NULL, &buffer->lockedData);

    if (outDesc) {
        outDesc->width = buffer->desc.width;
        outDesc->height = buffer->desc.height;
        outDesc->stride = buffer->desc.stride;
        outDesc->format = buffer->desc.format;
    }

    if (out)
        *out = buffer->lockedData;

    buffer->locked = 1;

    return ret;
}

__LIBC_HIDDEN__ int LorieBuffer_unlock(LorieBuffer* buffer) {
    int ret = 0;
    if (!buffer)
        return ENODEV;

    if (!buffer->locked) {
        dprintf(2, "tried to unlock non-locked buffer\n");
        return ENOENT;
    }

    if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        ret = AHardwareBuffer_unlock(buffer->desc.buffer, NULL);

    buffer->lockedData = NULL;
    buffer->locked = false;

    return ret;
}

__LIBC_HIDDEN__ void LorieBuffer_sendHandleToUnixSocket(LorieBuffer* _Nonnull buffer, int socketFd) {
    if (socketFd < 0 || !buffer)
        return;

    write(socketFd, buffer, sizeof(*buffer));
    if (buffer->desc.type == LORIEBUFFER_REGULAR)
        ancil_send_fd(socketFd, buffer->fd);
    else if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        AHardwareBuffer_sendHandleToUnixSocket(buffer->desc.buffer, socketFd);
}

__LIBC_HIDDEN__ void LorieBuffer_recvHandleFromUnixSocket(int socketFd, LorieBuffer** outBuffer) {
    LorieBuffer buffer = {0}, *ret = NULL;
    // We should read buffer from socket despite outbuffer is NULL, otherwise we will get protocol error
    if (socketFd < 0 || !outBuffer)
        return;

    // Reset process-specific data;
    buffer.refcount = 0;
    buffer.locked = false;
    buffer.fd = -1;
    buffer.lockedData = NULL;
    __sync_fetch_and_add(&buffer.refcount, 1); // refcount is the first object in the struct

    read(socketFd, &buffer, sizeof(buffer));
    buffer.image = NULL; // Only for process-local use
    if (buffer.desc.type == LORIEBUFFER_REGULAR) {
        size_t size = alignToPage(buffer.desc.width * buffer.desc.height * sizeof(uint32_t));
        buffer.fd = ancil_recv_fd(socketFd);
        if (buffer.fd == -1) {
            *outBuffer = NULL;
            return;
        }

        buffer.desc.data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, buffer.fd, 0);
        if (buffer.desc.data == NULL || buffer.desc.data == MAP_FAILED) {
            close(buffer.fd);
            *outBuffer = NULL;
            return;
        }
    } else if (buffer.desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        AHardwareBuffer_recvHandleFromUnixSocket(socketFd, &buffer.desc.buffer);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "MemoryLeak"
    if (outBuffer)
        ret = calloc(1, sizeof(buffer));
#pragma clang diagnostic pop
    if (!ret) {
        if (buffer.fd >= 0)
            close(buffer.fd);
        if (buffer.desc.buffer)
            AHardwareBuffer_release(buffer.desc.buffer);
        if (outBuffer)
            outBuffer = NULL;
        return;
    }

    *ret = buffer;
    *outBuffer = ret;
}

void LorieBuffer_attachToGL(LorieBuffer* buffer) {
    const EGLint imageAttributes[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    if (!eglGetCurrentDisplay() || !buffer)
        return;

    if (buffer->image == NULL && buffer->desc.buffer)
        buffer->image = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, eglGetNativeClientBufferANDROID(buffer->desc.buffer), imageAttributes);

    glGenTextures(1, &buffer->id);
    glBindTexture(GL_TEXTURE_2D, buffer->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (buffer->image)
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, buffer->image);
    else if (buffer->desc.data && buffer->desc.width > 0 && buffer->desc.height > 0) {
        int format = buffer->desc.format == AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM ? GL_BGRA_EXT : GL_RGBA;
        // The image will be updated in redraw call because of `drawRequested` flag, so we are not uploading pixels
        glTexImage2D(GL_TEXTURE_2D, 0, format, buffer->desc.width, buffer->desc.height, 0, format, GL_UNSIGNED_BYTE, NULL);
    }
}

void LorieBuffer_bindTexture(LorieBuffer *buffer) {
    if (!buffer)
        return;

    glBindTexture(GL_TEXTURE_2D, buffer->id);
    if (buffer->desc.type == LORIEBUFFER_REGULAR)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer->desc.width, buffer->desc.height, buffer->desc.format == AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM ? GL_BGRA_EXT : GL_RGBA, GL_UNSIGNED_BYTE, buffer->desc.data);
}

int LorieBuffer_getWidth(LorieBuffer *buffer) {
    return buffer ? buffer->desc.width : 0;
}

int LorieBuffer_getHeight(LorieBuffer *buffer) {
    return buffer ? buffer->desc.height : 0;
}

bool LorieBuffer_isRgba(LorieBuffer *buffer) {
    return buffer ? buffer->desc.format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM : false;
}

__LIBC_HIDDEN__ int ancil_send_fd(int sock, int fd) {
    char nothing = '!';
    struct iovec nothing_ptr = { .iov_base = &nothing, .iov_len = 1 };

    struct {
        struct cmsghdr align;
        int fd[1];
    } ancillary_data_buffer;

    struct msghdr message_header = {
            .msg_name = NULL,
            .msg_namelen = 0,
            .msg_iov = &nothing_ptr,
            .msg_iovlen = 1,
            .msg_flags = 0,
            .msg_control = &ancillary_data_buffer,
            .msg_controllen = sizeof(struct cmsghdr) + sizeof(int)
    };

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message_header);
    cmsg->cmsg_len = message_header.msg_controllen; // sizeof(int);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    ((int*) CMSG_DATA(cmsg))[0] = fd;

    return sendmsg(sock, &message_header, 0) >= 0 ? 0 : -1;
}

__LIBC_HIDDEN__ int ancil_recv_fd(int sock) {
    char nothing = '!';
    struct iovec nothing_ptr = { .iov_base = &nothing, .iov_len = 1 };

    struct {
        struct cmsghdr align;
        int fd[1];
    } ancillary_data_buffer;

    struct msghdr message_header = {
            .msg_name = NULL,
            .msg_namelen = 0,
            .msg_iov = &nothing_ptr,
            .msg_iovlen = 1,
            .msg_flags = 0,
            .msg_control = &ancillary_data_buffer,
            .msg_controllen = sizeof(struct cmsghdr) + sizeof(int)
    };

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message_header);
    cmsg->cmsg_len = message_header.msg_controllen;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    ((int*) CMSG_DATA(cmsg))[0] = -1;

    if (recvmsg(sock, &message_header, 0) < 0) return -1;

    return ((int*) CMSG_DATA(cmsg))[0];
}
