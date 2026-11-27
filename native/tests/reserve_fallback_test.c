#include "stsv_reserve_fallback.h"
#include "stsv_buffer_diag.h"

#include <assert.h>
#include <stddef.h>

static unsigned char storage[64];
static int allocation_calls;
static int deferred_calls;
static int fail_allocation;
static uint32_t requested_size;

static void *fake_alloc(uint32_t size) {
    allocation_calls++;
    requested_size = size;
    return fail_allocation ? NULL : storage;
}

static void fake_defer_free(void *ptr) {
    assert(ptr == storage);
    deferred_calls++;
}

static void reset(void) {
    allocation_calls = 0;
    deferred_calls = 0;
    fail_allocation = 0;
    requested_size = 0;
}

static void test_pool_boundary_matches_vitagl(void) {
    assert(!stsv_reserve_pool_overflow(0x1000u, 0x2000u, 0x1000u));
    assert(stsv_reserve_pool_overflow(0x1000u, 0x2000u, 0x1001u));
    assert(stsv_reserve_pool_overflow(0x2001u, 0x2000u, 1u));
    assert(!stsv_reserve_pool_overflow(0x1000u, 0x2000u, 0u));
}

static void test_pool_cursor_saturates_after_overflow(void) {
    assert(stsv_reserve_pool_advance(0x1000u, 0x2000u, 0x400u) == 0x1400u);
    assert(stsv_reserve_pool_advance(0x1000u, 0x2000u, 0x1000u) == 0x2000u);
    assert(stsv_reserve_pool_advance(0x1000u, 0x2000u, 0x1001u) == 0x2000u);
    assert(stsv_reserve_pool_advance(0x2001u, 0x2000u, 1u) == 0x2000u);
    assert(stsv_reserve_pool_advance(UINTPTR_MAX - 0x10u, 0x2000u, 0x100u)
            == 0x2000u);
}

static void test_success_is_deferred_once(void) {
    reset();
    void *ptr = stsv_reserve_overflow_alloc(8192u, fake_alloc, fake_defer_free);
    assert(ptr == storage);
    assert(allocation_calls == 1);
    assert(deferred_calls == 1);
    assert(requested_size == 8192u);
}

static void test_failed_alloc_is_not_enqueued(void) {
    reset();
    fail_allocation = 1;
    void *ptr = stsv_reserve_overflow_alloc(8192u, fake_alloc, fake_defer_free);
    assert(ptr == NULL);
    assert(allocation_calls == 1);
    assert(deferred_calls == 0);
}

static void test_missing_defer_callback_does_not_allocate(void) {
    reset();
    void *ptr = stsv_reserve_overflow_alloc(8192u, fake_alloc, NULL);
    assert(ptr == NULL);
    assert(allocation_calls == 0);
    assert(deferred_calls == 0);
}


static void test_buffer_data_context_keeps_requested_capacity(void) {
    stsv_buffer_diag_context context = stsv_buffer_diag_make_data(
            0x8892u, UINT32_C(0x81234567), 1,
            INT64_C(8388608), 0x88E0u, (uintptr_t) 0x1234u);
    assert(context.op == STSV_BUFFER_DIAG_DATA);
    assert(context.target == 0x8892u);
    assert(context.buffer_id == UINT32_C(0x81234567));
    assert(context.buffer_id_known);
    assert(context.buffer_size == INT64_C(8388608));
    assert(context.offset == 0);
    assert(context.update_size == INT64_C(8388608));
    assert(context.usage == 0x88E0u);
    assert(context.data == (uintptr_t) 0x1234u);
    assert(stsv_buffer_diag_reserve_capacity(&context, 4096u)
            == INT64_C(8388608));
    assert(stsv_buffer_diag_op_name(context.op) != NULL);
}

static void test_buffer_subdata_context_uses_full_reserve_size(void) {
    stsv_buffer_diag_context context = stsv_buffer_diag_make_subdata(
            0x8892u, UINT32_C(0x81234567), 1,
            INT64_C(256), INT64_C(64), (uintptr_t) 0x5678u);
    assert(context.op == STSV_BUFFER_DIAG_SUBDATA);
    assert(context.target == 0x8892u);
    assert(context.buffer_id == UINT32_C(0x81234567));
    assert(context.buffer_id_known);
    assert(context.buffer_size == -1);
    assert(context.offset == INT64_C(256));
    assert(context.update_size == INT64_C(64));
    assert(context.usage == 0);
    assert(context.data == (uintptr_t) 0x5678u);
    /* vitaGL r1488 reserves gpu_buf->size, not the small update size. */
    assert(stsv_buffer_diag_reserve_capacity(&context, 8388608u)
            == INT64_C(8388608));
}

static void test_unknown_buffer_context_does_not_invent_capacity(void) {
    stsv_buffer_diag_context context = stsv_buffer_diag_make_subdata(
            0x8A11u, 0, 0, 0, 16, 0);
    assert(!context.buffer_id_known);
    assert(context.buffer_id == 0);
    assert(stsv_buffer_diag_reserve_capacity(&context, 16u) == 16);
    stsv_buffer_diag_context empty = {0};
    assert(stsv_buffer_diag_reserve_capacity(&empty, 16u) == -1);
    assert(stsv_buffer_diag_op_name(STSV_BUFFER_DIAG_NONE) != NULL);
}

int main(void) {
    test_pool_boundary_matches_vitagl();
    test_pool_cursor_saturates_after_overflow();
    test_success_is_deferred_once();
    test_failed_alloc_is_not_enqueued();
    test_missing_defer_callback_does_not_allocate();
    test_buffer_data_context_keeps_requested_capacity();
    test_buffer_subdata_context_uses_full_reserve_size();
    test_unknown_buffer_context_does_not_invent_capacity();
    return 0;
}
