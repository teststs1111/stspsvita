#include "stsv_reserve_fallback.h"

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

int main(void) {
    test_pool_boundary_matches_vitagl();
    test_pool_cursor_saturates_after_overflow();
    test_success_is_deferred_once();
    test_failed_alloc_is_not_enqueued();
    test_missing_defer_callback_does_not_allocate();
    return 0;
}
