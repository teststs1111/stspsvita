#ifndef STSV_BUFFER_DIAG_H
#define STSV_BUFFER_DIAG_H

#include <stdint.h>

typedef enum {
    STSV_BUFFER_DIAG_NONE = 0,
    STSV_BUFFER_DIAG_DATA = 1,
    STSV_BUFFER_DIAG_SUBDATA = 2
} stsv_buffer_diag_op;

typedef struct {
    stsv_buffer_diag_op op;
    uint32_t target;
    uint32_t buffer_id;
    int buffer_id_known;
    int64_t buffer_size;
    int64_t offset;
    int64_t update_size;
    uint32_t usage;
    uintptr_t data;
} stsv_buffer_diag_context;

static inline stsv_buffer_diag_context stsv_buffer_diag_make_data(
        uint32_t target, uint32_t buffer_id, int buffer_id_known,
        int64_t size, uint32_t usage, uintptr_t data) {
    stsv_buffer_diag_context context = {
        STSV_BUFFER_DIAG_DATA, target, buffer_id, buffer_id_known,
        size, 0, size, usage, data
    };
    return context;
}

static inline stsv_buffer_diag_context stsv_buffer_diag_make_subdata(
        uint32_t target, uint32_t buffer_id, int buffer_id_known,
        int64_t offset, int64_t size, uintptr_t data) {
    stsv_buffer_diag_context context = {
        STSV_BUFFER_DIAG_SUBDATA, target, buffer_id, buffer_id_known,
        -1, offset, size, 0, data
    };
    return context;
}

static inline const char *stsv_buffer_diag_op_name(stsv_buffer_diag_op op) {
    switch (op) {
    case STSV_BUFFER_DIAG_DATA:
        return "buffer-data";
    case STSV_BUFFER_DIAG_SUBDATA:
        return "buffer-subdata";
    default:
        return "none";
    }
}

/* vitaGL r1488 calls vgl_reserve_data_pool(gpu_buf->size) from the scratch
 * branch of glNamedBufferSubData. The reserve request is therefore the exact
 * existing buffer capacity for that operation. */
static inline int64_t stsv_buffer_diag_reserve_capacity(
        const stsv_buffer_diag_context *context, uint32_t reserve_size) {
    if (context == 0) return -1;
    if (context->op == STSV_BUFFER_DIAG_DATA) return context->buffer_size;
    if (context->op == STSV_BUFFER_DIAG_SUBDATA) return (int64_t) reserve_size;
    return -1;
}

#endif
