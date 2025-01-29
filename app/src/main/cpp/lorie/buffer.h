#pragma once
#include <fcntl.h>
#include <linux/ashmem.h>
#include <android/hardware_buffer.h>

#define STATIC_INLINE static inline __always_inline

#define AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM 5 // Stands to HAL_PIXEL_FORMAT_BGRA_8888

enum {
    LORIEBUFFER_UNKNOWN,
    LORIEBUFFER_REGULAR,
    LORIEBUFFER_AHARDWAREBUFFER,
};

typedef struct {
    int32_t width, height, stride;
    uint8_t format, type;
    AHardwareBuffer* buffer;
    void* data;
} LorieBuffer_Desc;

typedef struct LorieBuffer LorieBuffer;

/**
 * Creates anonymous shared memory fragment in anonymous namespace
 *
 * @param name name of fragment passed to `memfd_create` or `ashmem`.
 * @param size size of fragment in bytes.
 * @return negative value on error, positive or zero  in the case of file descriptor.
 */
int LorieBuffer_createRegion(char const* name, size_t size);

/**
 * Allocates new shareable buffer.
 * It is a reference counted object. The returned buffer has a reference count of 1.
 *
 * @param width width of buffer.
 * @param height height of buffer.
 * @param format format of buffer. Accepts AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM or AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM
 * @param type type of buffer. Accepts LORIEBUFFER_REGULAR or LORIEBUFFER_AHARDWAREBUFFER.
 * @return returns the buffer itself or NULL on failure.
 */
LorieBuffer* LorieBuffer_allocate(int32_t width, int32_t height, int8_t format, int8_t type);

/**
 * Acquire a reference on the given LorieBuffer object.
 *
 * This prevents the object from being deleted until the last reference
 * is removed.
 *
 * @param buffer the buffer
 */
STATIC_INLINE void LorieBuffer_acquire(LorieBuffer* buffer) {
    if (!buffer)
        return;

    __sync_fetch_and_add((int*) buffer, 1); // refcount is the first object in the struct
}

/**
 * Remove a reference that was previously acquired with
 * AHardwareBuffer_acquire() or AHardwareBuffer_allocate().
 *
 * @param buffer the buffer
 */
STATIC_INLINE void LorieBuffer_release(LorieBuffer* buffer) {
    void __LorieBuffer_free(LorieBuffer* buffer);
    if (buffer && __sync_fetch_and_sub((int*) buffer, 1) == 1) // refcount is the first object in the struct
        __LorieBuffer_free(buffer);
}

/**
 * Return a description of the LorieBuffer in the passed LorieBuffer_Desc struct.
 *
 * @param buffer the buffer to be described
 * @param desc reference to description
 */
void LorieBuffer_describe(LorieBuffer* buffer, LorieBuffer_Desc* desc);

/**
 * Lock the AHardwareBuffer for direct CPU access.
 * See AHardwareBuffer_lock() description for details
 *
 * @param buffer the buffer to be locked
 * @param outDesc description of the buffer to be locked
 * @param out description of the buffer
 * @return 0 on success, not 0 on failure
 */
int LorieBuffer_lock(LorieBuffer* buffer, AHardwareBuffer_Desc* outDesc, void** out);

/**
 * Unlock the AHardwareBuffer from direct CPU access.
 * See AHardwareBuffer_unlock() description for details
 *
 * @param buffer
 * @return
 */
int LorieBuffer_unlock(LorieBuffer* buffer);

/**
 * Send the AHardwareBuffer to an AF_UNIX socket.
 *
 * @param buffer
 * @param socketFd
 */
void LorieBuffer_sendHandleToUnixSocket(LorieBuffer* buffer, int socketFd);

/**
 * Receive an AHardwareBuffer from an AF_UNIX socket.
 *
 * @param socketFd
 * @param outBuffer
 */
void LorieBuffer_recvHandleFromUnixSocket(int socketFd, LorieBuffer** outBuffer);

void LorieBuffer_attachToGL(LorieBuffer* buffer);
void LorieBuffer_bindTexture(LorieBuffer *buffer);

int LorieBuffer_getWidth(LorieBuffer *buffer);
int LorieBuffer_getHeight(LorieBuffer *buffer);
bool LorieBuffer_isRgba(LorieBuffer *buffer);

#undef STATIC_INLINE

int ancil_send_fd(int sock, int fd);
int ancil_recv_fd(int sock);
