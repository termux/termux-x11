#pragma once
#include <fcntl.h>
#include <linux/ashmem.h>
#include <android/hardware_buffer.h>

#define STATIC_INLINE static inline __always_inline

#define AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM 5 // Stands to HAL_PIXEL_FORMAT_BGRA_8888

enum {
    LORIEBUFFER_UNKNOWN __unused,
    LORIEBUFFER_REGULAR,
    LORIEBUFFER_FD,
    LORIEBUFFER_AHARDWAREBUFFER,
};

typedef struct {
    int32_t width, height, stride;
    uint8_t format, type;
    uint64_t id;
    AHardwareBuffer* _Nullable buffer;
    void* _Nullable data;
} LorieBuffer_Desc;

typedef struct LorieBuffer LorieBuffer;

/**
 * Creates anonymous shared memory fragment in anonymous namespace
 *
 * @param name name of fragment passed to `memfd_create` or `ashmem`.
 * @param size size of fragment in bytes.
 * @return negative value on error, positive or zero  in the case of file descriptor.
 */
int LorieBuffer_createRegion(char const* _Nonnull name, size_t size);

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
LorieBuffer* _Nullable LorieBuffer_allocate(int32_t width, int32_t height, int8_t format, int8_t type);

/**
 * Wraps given memory fragment file descriptor into LorieBuffer.
 * Takes ownership on the given file descriptor.
 *
 * @param width width of buffer.
 * @param height height of buffer.
 * @param format format of buffer. Accepts AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM or AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM
 * @param fd file descriptor of buffer.
 * @param size size of memory fragment.
 * @param offset offset inside memory fragment.
 * @return
 */
LorieBuffer* _Nullable LorieBuffer_wrapFileDescriptor(int32_t width, int32_t stride, int32_t height, int8_t format, int fd, off_t offset);

/**
 * Wraps given AHardwareBuffer into LorieBuffer.
 * Takes ownership on the given AHardwareBuffer.
 *
 * @param buffer buffer to be wrapped
 * @return
 */
LorieBuffer* _Nullable LorieBuffer_wrapAHardwareBuffer(AHardwareBuffer* _Nullable buffer);

/**
 * Convert regular buffer to given type.
 *
 * @param buffer
 * @param type
 * @param format
 */
void LorieBuffer_convert(LorieBuffer* _Nullable buffer, int8_t type, int8_t format);

/**
 * Acquire a reference on the given LorieBuffer object.
 *
 * This prevents the object from being deleted until the last reference
 * is removed.
 *
 * @param buffer the buffer
 */
STATIC_INLINE void LorieBuffer_acquire(LorieBuffer* _Nullable buffer) {
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
STATIC_INLINE void LorieBuffer_release(LorieBuffer* _Nullable buffer) {
    void __LorieBuffer_free(LorieBuffer* buffer);
    if (buffer && __sync_fetch_and_sub((int16_t*) buffer, 1) == 1) // refcount is the first object in the struct
        __LorieBuffer_free(buffer);
}

/**
 * Return a description of the LorieBuffer.
 *
 * @param buffer the buffer to be described
 * @return reference to description
 */
const LorieBuffer_Desc* _Nonnull LorieBuffer_description(LorieBuffer* _Nullable buffer);

/**
 * Lock the AHardwareBuffer for direct CPU access.
 * See AHardwareBuffer_lock() description for details
 *
 * @param buffer the buffer to be locked
 * @param outDesc description of the buffer to be locked
 * @param out description of the buffer
 * @return 0 on success, not 0 on failure
 */
int LorieBuffer_lock(LorieBuffer* _Nullable buffer, void* _Nullable * _Nonnull out);

/**
 * Unlock the AHardwareBuffer from direct CPU access.
 * See AHardwareBuffer_unlock() description for details
 *
 * @param buffer
 * @return
 */
int LorieBuffer_unlock(LorieBuffer* _Nullable buffer);

/**
 * Send the AHardwareBuffer to an AF_UNIX socket.
 *
 * @param buffer
 * @param socketFd
 */
void LorieBuffer_sendHandleToUnixSocket(LorieBuffer* _Nonnull buffer, int socketFd);

/**
 * Receive an AHardwareBuffer from an AF_UNIX socket.
 *
 * @param socketFd
 * @param outBuffer
 */
void LorieBuffer_recvHandleFromUnixSocket(int socketFd, LorieBuffer* _Nullable * _Nullable outBuffer);

/**
 * Attach buffer to GL. Must be done on GL thread.
 * After attaching subsequent call to LorieBuffer_release must be done only from GL thread.
 *
 * @param buffer the buffer to be attached.
 */
void LorieBuffer_attachToGL(LorieBuffer* _Nullable buffer);

/**
 * Call glBindTexture for the buffer.
 *
 * @param buffer the buffer to be bound.
 */
void LorieBuffer_bindTexture(LorieBuffer* _Nullable buffer);

/**
 * Get width of the buffer.
 *
 * @param buffer
 * @return
 */

int LorieBuffer_getWidth(LorieBuffer* _Nullable buffer);
/**
 * Get height of the buffer.
 *
 * @param buffer
 * @return
 */
int LorieBuffer_getHeight(LorieBuffer* _Nullable buffer);

/**
 * Check if the buffer is RGBA.
 *
 * @param buffer
 * @return
 */
bool LorieBuffer_isRgba(LorieBuffer* _Nullable buffer);

struct xorg_list;

/**
 * Add the buffer to xorg_list.
 *
 * @param buffer
 * @param list
 */
void LorieBuffer_addToList(LorieBuffer* _Nullable buffer, struct xorg_list* _Nullable list);

/**
 * Remove the buffer from xorg_list.
 *
 * @param buffer
 */
void LorieBuffer_removeFromList(LorieBuffer* _Nullable buffer);

/**
 * Get the first buffer in the list.
 *
 * @param list
 * @return buffer if it is present, NULL otherwise.
 */
LorieBuffer* _Nullable LorieBufferList_first(struct xorg_list* _Nullable list);

/**
 * Find the buffer with given ID in the list
 *
 * @param list
 * @param id
 * @return buffer if it is present, NULL otherwise.
 */
LorieBuffer* _Nullable LorieBufferList_findById(struct xorg_list* _Nullable list, uint64_t id);

#undef STATIC_INLINE

int ancil_send_fd(int sock, int fd);
int ancil_recv_fd(int sock);
