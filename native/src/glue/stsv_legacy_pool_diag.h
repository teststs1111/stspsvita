#ifndef STSV_LEGACY_POOL_DIAG_H
#define STSV_LEGACY_POOL_DIAG_H

#include <stdint.h>

typedef struct {
    int valid;
    int overrun;
    uintptr_t used_bytes;
    uintptr_t remaining_bytes;
} stsv_legacy_pool_usage;

/* Thumb function pointers and LR return addresses carry bit 0. Compare
 * canonical code addresses and keep the end symbol exclusive. */
static inline int stsv_legacy_pool_is_scene_reset_caller(
        uintptr_t caller, uintptr_t scene_reset_start,
        uintptr_t next_function_start, uint32_t request_size,
        uint32_t configured_size) {
    caller &= ~(uintptr_t) 1;
    scene_reset_start &= ~(uintptr_t) 1;
    next_function_start &= ~(uintptr_t) 1;
    return configured_size != 0 && request_size == configured_size
            && scene_reset_start != 0
            && next_function_start > scene_reset_start
            && caller >= scene_reset_start
            && caller < next_function_start;
}

/* legacy_pool and legacy_pool_ptr both advance through the same per-frame
 * scratch allocation: the former after glEnd/draw, the latter while vertices
 * are emitted. Use the furthest pointer to include an in-progress batch. */
static inline stsv_legacy_pool_usage stsv_legacy_pool_measure(
        uintptr_t base, uint32_t reserved_size,
        uintptr_t draw_ptr, uintptr_t write_ptr,
        uint32_t configured_size, uint32_t reserve_frame,
        uint32_t current_frame) {
    stsv_legacy_pool_usage usage = {0, 0, 0, 0};
    if (base == 0 || reserved_size == 0
            || reserved_size != configured_size
            || reserve_frame != current_frame
            || draw_ptr == 0 || write_ptr == 0
            || draw_ptr < base || write_ptr < base) {
        return usage;
    }

    uintptr_t high = draw_ptr > write_ptr ? draw_ptr : write_ptr;
    usage.valid = 1;
    usage.used_bytes = high - base;
    usage.overrun = usage.used_bytes > reserved_size;
    usage.remaining_bytes = usage.overrun
            ? 0 : (uintptr_t) reserved_size - usage.used_bytes;
    return usage;
}

#endif
