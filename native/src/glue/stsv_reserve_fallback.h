#ifndef STSV_RESERVE_FALLBACK_H
#define STSV_RESERVE_FALLBACK_H

#include <stdint.h>

typedef void *(*stsv_reserve_alloc_fn)(uint32_t size);
typedef void (*stsv_reserve_defer_free_fn)(void *ptr);

/* Match vitaGL's pool-end check without unsigned underflow. */
static inline int stsv_reserve_pool_overflow(uintptr_t ptr, uintptr_t limit,
                                            uint32_t size) {
    return ptr > limit || (uintptr_t) size > limit - ptr;
}

/* Once a request does not fit, consume the remainder of this frame's pool.
 * Saturating the cursor avoids 32-bit pointer wrap after repeated failed
 * fallback requests while preserving all successful in-pool reservations. */
static inline uintptr_t stsv_reserve_pool_advance(uintptr_t ptr, uintptr_t limit,
                                                  uint32_t size) {
    if (ptr > limit || (uintptr_t) size > limit - ptr) {
        return limit;
    }
    return ptr + (uintptr_t) size;
}

/* Preserve the overflow allocation's deferred-free lifetime, but never enqueue
 * a NULL sentinel: vitaGL's GC stops at the first NULL list entry. */
static inline void *stsv_reserve_overflow_alloc(
        uint32_t size,
        stsv_reserve_alloc_fn alloc,
        stsv_reserve_defer_free_fn defer_free) {
    if (alloc == 0 || defer_free == 0) return 0;
    void *ptr = alloc(size);
    if (ptr != 0) defer_free(ptr);
    return ptr;
}

#endif
