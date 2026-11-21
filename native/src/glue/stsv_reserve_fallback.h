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
