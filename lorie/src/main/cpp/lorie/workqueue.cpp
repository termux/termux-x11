#include <cstdlib>
#include <cstddef>
#include <pthread.h>

extern "C" {
#include <dix-config.h>
#include <misc.h>
#include <dix.h>
#include <dixstruct.h>

extern WorkQueuePtr workQueue;
}

/*
 * Fast path: a bounded lock-free MPMC ring buffer (Vyukov's algorithm,
 * single-word atomic builtins only, so it stays lock-free on armeabi-v7a
 * too) holding items by value. Producers are any thread; the ring is only
 * ever drained by the server's own thread.
 *
 * Overflow path: when the ring is full (a producer burst past kRingSize
 * in-flight items), items spill onto a spinlock-protected malloc'd list,
 * freed again as soon as they're drained. This keeps steady state
 * allocation-free while still tolerating unbounded bursts.
 */

namespace {

WorkQueueRec workQueueSentinel;

struct WorkItem {
    Bool (*function)(ClientPtr, void *);
    ClientPtr client;
    void *closure;
};

struct OverflowNode {
    WorkItem item;
    OverflowNode *next;
};

constexpr size_t kRingSize = 128; /* must be a power of two */

struct Cell {
    size_t sequence;
    WorkItem item;
};

Cell ring[kRingSize];
size_t enqueuePos;
size_t dequeuePos;

OverflowNode *overflowHead;
OverflowNode **overflowLast = &overflowHead;
pthread_spinlock_t overflowLock;

__attribute__((constructor)) void initWorkQueue() {
    for (size_t i = 0; i < kRingSize; i++)
        __atomic_store_n(&ring[i].sequence, i, __ATOMIC_RELAXED);
    pthread_spin_init(&overflowLock, PTHREAD_PROCESS_PRIVATE);
    /* os/WaitFor.c guards ProcessWorkQueue() with `if (workQueue)`; keep it
     * non-NULL so that still fires (the pointer itself is never dereferenced). */
    workQueue = &workQueueSentinel;
}

bool ringPush(Bool (*function)(ClientPtr, void *), ClientPtr client, void *closure) {
    size_t pos = __atomic_load_n(&enqueuePos, __ATOMIC_RELAXED);
    Cell *cell;

    for (;;) {
        cell = &ring[pos & (kRingSize - 1)];
        size_t seq = __atomic_load_n(&cell->sequence, __ATOMIC_ACQUIRE);
        auto diff = (ptrdiff_t) seq - (ptrdiff_t) pos;

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&enqueuePos, &pos, pos + 1, true,
                                             __ATOMIC_RELAXED, __ATOMIC_RELAXED))
                break;
        } else if (diff < 0) {
            return false; /* ring is full */
        } else {
            pos = __atomic_load_n(&enqueuePos, __ATOMIC_RELAXED);
        }
    }

    cell->item.function = function;
    cell->item.client = client;
    cell->item.closure = closure;
    __atomic_store_n(&cell->sequence, pos + 1, __ATOMIC_RELEASE);
    return true;
}

bool ringPop(WorkItem *out) {
    size_t pos = __atomic_load_n(&dequeuePos, __ATOMIC_RELAXED);
    Cell *cell;

    for (;;) {
        cell = &ring[pos & (kRingSize - 1)];
        size_t seq = __atomic_load_n(&cell->sequence, __ATOMIC_ACQUIRE);
        auto diff = (ptrdiff_t) seq - (ptrdiff_t) (pos + 1);

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&dequeuePos, &pos, pos + 1, true,
                                             __ATOMIC_RELAXED, __ATOMIC_RELAXED))
                break;
        } else if (diff < 0) {
            return false; /* ring is empty */
        } else {
            pos = __atomic_load_n(&dequeuePos, __ATOMIC_RELAXED);
        }
    }

    *out = cell->item;
    __atomic_store_n(&cell->sequence, pos + kRingSize, __ATOMIC_RELEASE);
    return true;
}

inline __always_inline size_t ringCount() {
    size_t tail = __atomic_load_n(&dequeuePos, __ATOMIC_RELAXED);
    size_t head = __atomic_load_n(&enqueuePos, __ATOMIC_RELAXED);
    return head - tail;
}

bool overflowPush(Bool (*function)(ClientPtr, void *), ClientPtr client, void *closure) {
    auto *node = (OverflowNode *) malloc(sizeof(OverflowNode));

    if (!node)
        return false;
    node->item.function = function;
    node->item.client = client;
    node->item.closure = closure;
    node->next = nullptr;

    pthread_spin_lock(&overflowLock);
    *overflowLast = node;
    overflowLast = &node->next;
    pthread_spin_unlock(&overflowLock);
    return true;
}

/* Best effort: put an already-owned item back for a later retry. On OOM
 * (only reachable if the ring is also full) the item is dropped. */
inline __always_inline void requeue(Bool (*function)(ClientPtr, void *), ClientPtr client, void *closure) {
    if (!ringPush(function, client, closure))
        overflowPush(function, client, closure);
}

} // namespace

extern "C" {

Bool QueueWorkProc(Bool (*function)(ClientPtr, void *), ClientPtr client, void *closure) {
    return ringPush(function, client, closure) || overflowPush(function, client, closure) ? TRUE : FALSE;
}

void ProcessWorkQueue(void) {
    WorkItem item{};
    size_t remaining = ringCount();

    /*
     * Scan the ring once, calling each function. Those which return TRUE
     * are done; the rest are requeued to be retried on a later call. Items
     * pushed while this runs are left for the next call. This must be
     * reentrant with QueueWorkProc.
     */
    while (remaining-- && ringPop(&item)) {
        if (!item.function(item.client, item.closure))
            requeue(item.function, item.client, item.closure);
    }

    pthread_spin_lock(&overflowLock);
    OverflowNode *q, **p = &overflowHead;
    while ((q = *p)) {
        pthread_spin_unlock(&overflowLock);
        if (q->item.function(q->item.client, q->item.closure)) {
            pthread_spin_lock(&overflowLock);
            *p = q->next;
            free(q);
        } else {
            pthread_spin_lock(&overflowLock);
            p = &q->next;
        }
    }
    overflowLast = p;
    pthread_spin_unlock(&overflowLock);
}

void ProcessWorkQueueZombies(void) {
    WorkItem item{};
    size_t remaining = ringCount();

    while (remaining-- && ringPop(&item)) {
        if (item.client && item.client->clientGone)
            item.function(item.client, item.closure);
        else
            requeue(item.function, item.client, item.closure);
    }

    pthread_spin_lock(&overflowLock);
    OverflowNode *q, **p = &overflowHead;
    while ((q = *p)) {
        if (q->item.client && q->item.client->clientGone) {
            q->item.function(q->item.client, q->item.closure);
            *p = q->next;
            free(q);
        } else {
            p = &q->next;
        }
    }
    overflowLast = p;
    pthread_spin_unlock(&overflowLock);
}

void ClearWorkQueue(void) {
    WorkItem item{};

    while (ringPop(&item))
        ;

    pthread_spin_lock(&overflowLock);
    OverflowNode *q, **p = &overflowHead;
    while ((q = *p)) {
        *p = q->next;
        free(q);
    }
    overflowLast = p;
    pthread_spin_unlock(&overflowLock);
}

} // extern "C"
