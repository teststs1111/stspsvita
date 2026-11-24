#ifndef STSV_COMPRESSED_MIP_DIAG_H
#define STSV_COMPRESSED_MIP_DIAG_H

#include <stddef.h>
#include <stdint.h>

/* Read-only mirror of the pinned vitaGL compressed mip-chain sizing rules.
 * VitaGL first rounds the base dimensions up to powers of two, then sums
 * ceil(width/4) * ceil(height/4) * block_bytes for levels 0..level. */
typedef struct {
    int id;
    uint32_t width;
    uint32_t height;
    int internal_format;
} stsv_compressed_mip_base;

static inline uint32_t stsv_mip_diag_round_up_po2(uint32_t value) {
    if (value == 0 || value > UINT32_C(0x80000000)) return 0;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

static inline size_t stsv_compressed_mip_chain_bytes(uint32_t base_width,
                                                      uint32_t base_height,
                                                      int level,
                                                      size_t block_bytes) {
    if (base_width == 0 || base_height == 0 || level < 0 || level > 31
            || block_bytes == 0) {
        return 0;
    }
    uint32_t width = stsv_mip_diag_round_up_po2(base_width);
    uint32_t height = stsv_mip_diag_round_up_po2(base_height);
    if (width == 0 || height == 0) return 0;

    size_t total = 0;
    for (int current = 0; current <= level; current++) {
        size_t blocks_w = ((size_t) width + 3u) / 4u;
        size_t blocks_h = ((size_t) height + 3u) / 4u;
        if (blocks_w != 0 && blocks_h > SIZE_MAX / blocks_w) return 0;
        size_t blocks = blocks_w * blocks_h;
        if (blocks != 0 && block_bytes > SIZE_MAX / blocks) return 0;
        size_t bytes = blocks * block_bytes;
        if (bytes > SIZE_MAX - total) return 0;
        total += bytes;
        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }
    return total;
}

/* Small bounded table keyed by live GL texture id. Level-zero definitions
 * replace an id's base dimensions; delete/redefinition clears the old entry. */
static inline int stsv_mip_diag_base_slot(const stsv_compressed_mip_base *entries,
                                           size_t capacity, int id) {
    if (entries == NULL || id == 0) return -1;
    int free_slot = -1;
    for (size_t i = 0; i < capacity; i++) {
        if (entries[i].id == id) return (int) i;
        if (entries[i].id == 0 && free_slot < 0) free_slot = (int) i;
    }
    return free_slot;
}

static inline int stsv_mip_diag_base_set(stsv_compressed_mip_base *entries,
                                          size_t capacity, int id,
                                          uint32_t width, uint32_t height,
                                          int internal_format) {
    if (entries == NULL || id == 0 || width == 0 || height == 0) return 0;
    int slot = stsv_mip_diag_base_slot(entries, capacity, id);
    if (slot < 0) return 0;
    entries[slot].id = id;
    entries[slot].width = width;
    entries[slot].height = height;
    entries[slot].internal_format = internal_format;
    return 1;
}

static inline int stsv_mip_diag_base_get(const stsv_compressed_mip_base *entries,
                                          size_t capacity, int id,
                                          uint32_t *width, uint32_t *height,
                                          int *internal_format) {
    if (entries == NULL || id == 0) return 0;
    for (size_t i = 0; i < capacity; i++) {
        if (entries[i].id == id) {
            if (width != NULL) *width = entries[i].width;
            if (height != NULL) *height = entries[i].height;
            if (internal_format != NULL) *internal_format = entries[i].internal_format;
            return 1;
        }
    }
    return 0;
}

static inline int stsv_mip_diag_base_clear(stsv_compressed_mip_base *entries,
                                            size_t capacity, int id) {
    if (entries == NULL || id == 0) return 0;
    int cleared = 0;
    for (size_t i = 0; i < capacity; i++) {
        if (entries[i].id == id) {
            entries[i].id = 0;
            entries[i].width = 0;
            entries[i].height = 0;
            entries[i].internal_format = 0;
            cleared = 1;
        }
    }
    return cleared;
}

#endif
