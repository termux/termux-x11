#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
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
#include <android/sharedmem.h>
#include "list.h"
#include "buffer.h"

struct LorieBuffer {
    int16_t refcount;
    LorieBuffer_Desc desc;

    int8_t locked;
    void* lockedData;

    // file descriptor of shared memory fragment for shared memory backed buffer
    int fd;
    size_t size;
    off_t offset;

    GLuint id;
    EGLImage image;
    struct xorg_list link;
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
    int fd = ASharedMemory_create(name, size);
    if (fd)
        return fd;

    fd = memfd_create(name, MFD_CLOEXEC|MFD_ALLOW_SEALING);
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

static LorieBuffer* allocate(int32_t width, int32_t stride, int32_t height, int8_t format, int8_t type, AHardwareBuffer *buf, int fd, size_t size, off_t offset, bool takeFd) {
    AHardwareBuffer_Desc desc = {0};
    static uint64_t id = 0;
    bool acceptable = (format == AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM || format == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM) && width > 0 && height > 0;
    LorieBuffer b = { .desc = { .width = width, .stride = stride, .height = height, .format = format, .type = type, .buffer = buf, .id = id++ }, .fd = takeFd ? fd : dup(fd), .size = size, .offset = offset };

    if (type != LORIEBUFFER_AHARDWAREBUFFER && !acceptable)
        return NULL;

    __sync_fetch_and_add(&b.refcount, 1);

    switch (type) {
        case LORIEBUFFER_REGULAR:
            b.desc.data = calloc(1, stride * height * sizeof(uint32_t));
            if (!b.desc.data)
                return NULL;
            break;
        case LORIEBUFFER_FD:
            if (b.fd < 0)
                return NULL;

            b.desc.data = mmap(NULL, b.size, PROT_READ|PROT_WRITE, MAP_SHARED, b.fd, b.offset);
            if (b.desc.data == NULL || b.desc.data == MAP_FAILED) {
                close(b.fd);
                return NULL;
            }
            break;
        case LORIEBUFFER_AHARDWAREBUFFER: {
            if (!b.desc.buffer)
                return NULL;

            AHardwareBuffer_describe(b.desc.buffer, &desc);
            b.desc.width = desc.width;
            b.desc.height = desc.height;
            b.desc.stride = desc.stride;
            b.desc.format = desc.format;
            break;
        }
        default: return NULL;
    }

    LorieBuffer* buffer = calloc(1, sizeof(*buffer));
    if (!buffer) {
        switch (type) {
            case LORIEBUFFER_REGULAR:
                free(b.desc.data);
                break;
            case LORIEBUFFER_FD:
                munmap(b.desc.data, b.size);
                close(b.fd);
                break;
            case LORIEBUFFER_AHARDWAREBUFFER:
                AHardwareBuffer_release(b.desc.buffer);
                break;
            default: break;
        }

        return NULL;
    }

    *buffer = b;
    xorg_list_init(&buffer->link);
    return buffer;
}

__LIBC_HIDDEN__ LorieBuffer* LorieBuffer_allocate(int32_t width, int32_t height, int8_t format, int8_t type) {
    int fd = -1;
    size_t size = 0;
    AHardwareBuffer *ahardwarebuffer = NULL;

    if (type == LORIEBUFFER_FD) {
        size = alignToPage(width * height * sizeof(uint32_t));
        fd = LorieBuffer_createRegion("LorieBuffer", size);
        if (fd < 0)
            return NULL;
    } else if (type == LORIEBUFFER_AHARDWAREBUFFER) {
        AHardwareBuffer_Desc desc = { .width = width, .height = height, .format = format, .layers = 1,
                .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE };
        int err = AHardwareBuffer_allocate(&desc, &ahardwarebuffer);
        if (err != 0)
            dprintf(2, "FATAL: failed to allocate AHardwareBuffer (width %d height %d format %d): error %d\n", width, height, format, err);
    }

    return allocate(width, width, height, format, type, ahardwarebuffer, fd, size, 0, true);
}

__LIBC_HIDDEN__ LorieBuffer* LorieBuffer_wrapFileDescriptor(int32_t width, int32_t stride, int32_t height, int8_t format, int fd, off_t offset) {
    return allocate(width, stride, height, format, LORIEBUFFER_FD, NULL, fd, stride * height * sizeof(uint32_t), offset, false);
}

__LIBC_HIDDEN__ LorieBuffer* LorieBuffer_wrapAHardwareBuffer(AHardwareBuffer* buffer) {
    return allocate(0, 0, 0, 0, LORIEBUFFER_AHARDWAREBUFFER, buffer, -1, 0, 0, false);
}

__LIBC_HIDDEN__ void LorieBuffer_convert(LorieBuffer* buffer, int8_t type, int8_t format) {
    void *data;
    if (!buffer || buffer->desc.type != LORIEBUFFER_REGULAR
        || (type != LORIEBUFFER_FD && type != LORIEBUFFER_AHARDWAREBUFFER)
        || (format != AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM && format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM))
        return;

    if (type == LORIEBUFFER_FD) {
        size_t size = alignToPage(buffer->desc.stride * buffer->desc.height * sizeof(uint32_t));
        int fd = LorieBuffer_createRegion("LorieBuffer", size);
        if (fd < 0)
            return;

        data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (!data || data == MAP_FAILED) {
            close(fd);
            return;
        }

        pixman_blt(buffer->desc.data, data, buffer->desc.stride, buffer->desc.stride, 32, 32, 0, 0, 0, 0, buffer->desc.width, buffer->desc.height);

        buffer->desc.type = type;
        buffer->desc.format = format;
        buffer->fd = fd;
        buffer->size = size;
        buffer->offset = 0;
        free(buffer->desc.data);
        buffer->desc.data = data;

        buffer->lockedData = NULL;
        buffer->locked = 0;
    } else {
        AHardwareBuffer *b = NULL;
        AHardwareBuffer_Desc desc = { .width = buffer->desc.width, .height = buffer->desc.height, .format = format, .layers = 1,
                .usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE };
        int err = AHardwareBuffer_allocate(&desc, &b);
        if (err != 0)
            return;

        AHardwareBuffer_describe(b, &desc);

        if (AHardwareBuffer_lock(b, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, NULL, &data) == 0) {
            pixman_blt(buffer->desc.data, data, buffer->desc.stride, desc.stride, 32, 32, 0, 0, 0, 0, buffer->desc.width, buffer->desc.height);
            AHardwareBuffer_unlock(b, NULL);
        }

        buffer->desc.type = type;
        buffer->desc.format = format;
        buffer->desc.stride = desc.stride;
        buffer->desc.buffer = b;
        free(buffer->desc.data);
        buffer->desc.data = NULL;
        buffer->lockedData = NULL;
        buffer->locked = 0;
    }
}

__LIBC_HIDDEN__ void __LorieBuffer_free(LorieBuffer* buffer) {
    if (!buffer)
        return;

    xorg_list_del(&buffer->link);

    if (eglGetCurrentContext())
        glDeleteTextures(1, &buffer->id);

    if (eglGetCurrentDisplay() && buffer->image)
        eglDestroyImageKHR(eglGetCurrentDisplay(), buffer->image);

    switch (buffer->desc.type) {
        case LORIEBUFFER_REGULAR:
            free(buffer->desc.data);
            break;
        case LORIEBUFFER_FD:
            munmap(buffer->desc.data, buffer->size);
            close(buffer->fd);
            break;
        case LORIEBUFFER_AHARDWAREBUFFER:
            AHardwareBuffer_release(buffer->desc.buffer);
            break;
        default: break;
    }

    free(buffer);
}

__LIBC_HIDDEN__ const LorieBuffer_Desc* LorieBuffer_description(LorieBuffer* buffer) {
    static const LorieBuffer_Desc none = {0};
    return buffer ? &buffer->desc : &none;
}

__LIBC_HIDDEN__ int LorieBuffer_lock(LorieBuffer* buffer, void** out) {
    int ret = 0;
    if (!buffer)
        return ENODEV;

    if (buffer->locked) {
        dprintf(2, "tried to lock already locked buffer\n");
        if (out)
            *out = buffer->lockedData;
        return EEXIST;
    }

    if (buffer->desc.type == LORIEBUFFER_REGULAR || buffer->desc.type == LORIEBUFFER_FD)
        buffer->lockedData = buffer->desc.data;
    else if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        ret = AHardwareBuffer_lock(buffer->desc.buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, NULL, &buffer->lockedData);

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
    if (buffer->desc.type == LORIEBUFFER_FD)
        ancil_send_fd(socketFd, buffer->fd);
    else if (buffer->desc.type == LORIEBUFFER_AHARDWAREBUFFER)
        AHardwareBuffer_sendHandleToUnixSocket(buffer->desc.buffer, socketFd);
}

__LIBC_HIDDEN__ void LorieBuffer_recvHandleFromUnixSocket(int socketFd, LorieBuffer** outBuffer) {
    LorieBuffer buffer = {0}, *ret = NULL;
    // We should read buffer from socket despite outbuffer is NULL, otherwise we will get protocol error
    if (socketFd < 0)
        return;

    // Reset process-specific data;
    buffer.refcount = 0;
    buffer.locked = false;
    buffer.fd = -1;
    buffer.lockedData = NULL;
    __sync_fetch_and_add(&buffer.refcount, 1); // refcount is the first object in the struct

    read(socketFd, &buffer, sizeof(buffer));
    buffer.image = NULL; // Only for process-local use
    if (buffer.desc.type == LORIEBUFFER_FD) {
        size_t size = buffer.desc.stride * buffer.desc.height * sizeof(uint32_t);
        buffer.fd = ancil_recv_fd(socketFd);
        if (buffer.fd == -1) {
            if (outBuffer)
                *outBuffer = NULL;
            return;
        }

        buffer.desc.data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, buffer.fd, 0);
        if (buffer.desc.data == NULL || buffer.desc.data == MAP_FAILED) {
            close(buffer.fd);
            if (outBuffer)
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
    xorg_list_init(&ret->link);
    *outBuffer = ret;
}

__LIBC_HIDDEN__ void LorieBuffer_attachToGL(LorieBuffer* buffer) {
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
        glTexImage2D(GL_TEXTURE_2D, 0, format, buffer->desc.stride, buffer->desc.height, 0, format, GL_UNSIGNED_BYTE, NULL);
    }
}

__LIBC_HIDDEN__ void LorieBuffer_bindTexture(LorieBuffer *buffer) {
    if (!buffer)
        return;

    glBindTexture(GL_TEXTURE_2D, buffer->id);
    if (buffer->desc.type == LORIEBUFFER_FD)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer->desc.stride, buffer->desc.height, buffer->desc.format == AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM ? GL_BGRA_EXT : GL_RGBA, GL_UNSIGNED_BYTE, buffer->desc.data);
}

__LIBC_HIDDEN__ int LorieBuffer_getWidth(LorieBuffer *buffer) {
    return LorieBuffer_description(buffer)->width;
}

__LIBC_HIDDEN__ int LorieBuffer_getHeight(LorieBuffer *buffer) {
    return LorieBuffer_description(buffer)->height;
}

__LIBC_HIDDEN__ bool LorieBuffer_isRgba(LorieBuffer *buffer) {
    return LorieBuffer_description(buffer)->format != AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM;
}

__LIBC_HIDDEN__ void LorieBuffer_addToList(LorieBuffer* _Nullable buffer, struct xorg_list* _Nullable list) {
    if (buffer && list) {
        xorg_list_del(&buffer->link);
        xorg_list_add(&buffer->link, list);
    }
}

__LIBC_HIDDEN__ void LorieBuffer_removeFromList(LorieBuffer* _Nullable buffer) {
    if (buffer)
        xorg_list_del(&buffer->link);
}

__LIBC_HIDDEN__ LorieBuffer* _Nullable LorieBufferList_first(struct xorg_list* _Nullable list) {
    return xorg_list_is_empty(list) ? NULL : xorg_list_first_entry(list, LorieBuffer, link);
}

__LIBC_HIDDEN__ LorieBuffer* _Nullable LorieBufferList_findById(struct xorg_list* _Nullable list, uint64_t id) {
    LorieBuffer *buffer;
    xorg_list_for_each_entry(buffer, list, link)
        if (buffer->desc.id == id)
            return buffer;
    return NULL;
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message_header);
    cmsg->cmsg_len = message_header.msg_controllen; // sizeof(int);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    ((int*) CMSG_DATA(cmsg))[0] = fd;
#pragma clang diagnostic pop

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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message_header);
    cmsg->cmsg_len = message_header.msg_controllen;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    ((int*) CMSG_DATA(cmsg))[0] = -1;
#pragma clang diagnostic pop

    if (recvmsg(sock, &message_header, 0) < 0) return -1;

    return ((int*) CMSG_DATA(cmsg))[0];
}
