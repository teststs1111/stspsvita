#include "stsv_legacy_pool_diag.h"

#include <assert.h>
#include <stdint.h>

static void test_thumb_caller_range(void) {
    assert(stsv_legacy_pool_is_scene_reset_caller(
            0x1105u, 0x1001u, 0x2001u, 8192u, 8192u));
    assert(!stsv_legacy_pool_is_scene_reset_caller(
            0x2001u, 0x1001u, 0x2001u, 8192u, 8192u));
    assert(!stsv_legacy_pool_is_scene_reset_caller(
            0x0fffu, 0x1001u, 0x2001u, 8192u, 8192u));
    assert(!stsv_legacy_pool_is_scene_reset_caller(
            0x1105u, 0x1001u, 0x2001u, 4096u, 8192u));
    assert(!stsv_legacy_pool_is_scene_reset_caller(
            0x1105u, 0x1001u, 0x2001u, 8192u, 0u));
}

static void test_usage_uses_furthest_pointer(void) {
    const uintptr_t base = 0x100000u;
    stsv_legacy_pool_usage usage = stsv_legacy_pool_measure(
            base, 8192u, base + 2048u, base + 4096u, 8192u, 7u, 7u);
    assert(usage.valid);
    assert(!usage.overrun);
    assert(usage.used_bytes == 4096u);
    assert(usage.remaining_bytes == 4096u);
}

static void test_exact_capacity_is_not_overrun(void) {
    const uintptr_t base = 0x200000u;
    stsv_legacy_pool_usage usage = stsv_legacy_pool_measure(
            base, 8192u, base + 8192u, base + 4096u, 8192u, 8u, 8u);
    assert(usage.valid);
    assert(!usage.overrun);
    assert(usage.used_bytes == 8192u);
    assert(usage.remaining_bytes == 0u);
}

static void test_overrun_is_reported_without_pointer_dereference(void) {
    const uintptr_t base = 0x300000u;
    stsv_legacy_pool_usage usage = stsv_legacy_pool_measure(
            base, 8192u, base + 8193u, base + 8192u, 8192u, 9u, 9u);
    assert(usage.valid);
    assert(usage.overrun);
    assert(usage.used_bytes == 8193u);
    assert(usage.remaining_bytes == 0u);
}

static void test_invalid_or_mismatched_snapshot(void) {
    stsv_legacy_pool_usage usage = stsv_legacy_pool_measure(
            0u, 8192u, 0x1000u, 0x1000u, 8192u, 10u, 10u);
    assert(!usage.valid);
    usage = stsv_legacy_pool_measure(
            0x1000u, 8192u, 0x0fffu, 0x1000u, 8192u, 11u, 11u);
    assert(!usage.valid);
    usage = stsv_legacy_pool_measure(
            0x1000u, 8192u, 0x2000u, 0x2000u, 4096u, 12u, 12u);
    assert(!usage.valid);
    usage = stsv_legacy_pool_measure(
            0x1000u, 8192u, 0x2000u, 0x2000u, 8192u, 12u, 13u);
    assert(!usage.valid);
}

static void test_history_tracks_last_peak_and_overruns(void) {
    stsv_legacy_pool_history history = {0, 0, 0, 0, 0};
    stsv_legacy_pool_usage usage = stsv_legacy_pool_measure(
            0x100000u, 8192u, 0x101000u, 0x100800u,
            8192u, 20u, 20u);
    stsv_legacy_pool_history_record(&history, usage, 20u);
    assert(history.valid_frames == 1u);
    assert(history.last_frame == 20u);
    assert(history.last_used_bytes == 4096u);
    assert(history.peak_used_bytes == 4096u);
    assert(history.overrun_frames == 0u);

    usage = stsv_legacy_pool_measure(
            0x200000u, 8192u, 0x202000u, 0x201000u,
            8192u, 21u, 21u);
    stsv_legacy_pool_history_record(&history, usage, 21u);
    assert(history.valid_frames == 2u);
    assert(history.last_frame == 21u);
    assert(history.last_used_bytes == 8192u);
    assert(history.peak_used_bytes == 8192u);

    /* A stale frame must not replace the last valid sample or peak. */
    usage = stsv_legacy_pool_measure(
            0x300000u, 8192u, 0x301000u, 0x301000u,
            8192u, 22u, 23u);
    stsv_legacy_pool_history_record(&history, usage, 23u);
    assert(history.valid_frames == 2u);
    assert(history.last_frame == 21u);
    assert(history.last_used_bytes == 8192u);

    usage = stsv_legacy_pool_measure(
            0x400000u, 8192u, 0x402001u, 0x402000u,
            8192u, 24u, 24u);
    stsv_legacy_pool_history_record(&history, usage, 24u);
    assert(history.valid_frames == 3u);
    assert(history.last_frame == 24u);
    assert(history.peak_used_bytes == 8193u);
    assert(history.overrun_frames == 1u);
}

int main(void) {
    test_thumb_caller_range();
    test_usage_uses_furthest_pointer();
    test_exact_capacity_is_not_overrun();
    test_overrun_is_reported_without_pointer_dereference();
    test_invalid_or_mismatched_snapshot();
    test_history_tracks_last_peak_and_overruns();
    return 0;
}
