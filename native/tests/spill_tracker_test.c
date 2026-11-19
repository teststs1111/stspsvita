#include "stsv_spill_tracker.h"

#include <assert.h>
#include <stddef.h>

static void test_reupload_replaces_old_bytes(void) {
    stsv_spill_entry entries[2] = {{7, 40}, {0, 0}};
    size_t used = 70;
    assert(stsv_spill_tracker_can_admit(entries, 2, 7, 70, 100, used));
    assert(stsv_spill_tracker_admit(entries, 2, 7, 70, 100, &used));
    assert(used == 100);
    assert(entries[0].id == 7 && entries[0].bytes == 70);
}

static void test_reupload_budget_uses_projected_usage(void) {
    stsv_spill_entry entries[1] = {{7, 40}};
    size_t used = 90;
    assert(!stsv_spill_tracker_can_admit(entries, 1, 7, 60, 100, used));
    assert(!stsv_spill_tracker_admit(entries, 1, 7, 60, 100, &used));
    assert(used == 90 && entries[0].bytes == 40);
}

static void test_zero_id_is_never_admitted(void) {
    stsv_spill_entry entries[1] = {{0, 0}};
    size_t used = 0;
    assert(!stsv_spill_tracker_can_admit(entries, 1, 0, 16, 100, used));
    assert(!stsv_spill_tracker_admit(entries, 1, 0, 16, 100, &used));
    assert(used == 0 && entries[0].id == 0);
}

static void test_full_table_rejects_untracked_texture(void) {
    stsv_spill_entry entries[2] = {{1, 20}, {2, 30}};
    size_t used = 50;
    assert(!stsv_spill_tracker_can_admit(entries, 2, 3, 10, 100, used));
    assert(!stsv_spill_tracker_admit(entries, 2, 3, 10, 100, &used));
    assert(used == 50);
    assert(entries[0].id == 1 && entries[1].id == 2);
}

static void test_full_table_allows_existing_id_update(void) {
    stsv_spill_entry entries[2] = {{1, 20}, {2, 30}};
    size_t used = 50;
    assert(stsv_spill_tracker_admit(entries, 2, 2, 40, 100, &used));
    assert(used == 60);
    assert(entries[1].id == 2 && entries[1].bytes == 40);
}

static void test_refund_frees_budget_and_slot(void) {
    stsv_spill_entry entries[1] = {{9, 80}};
    size_t used = 80;
    assert(stsv_spill_tracker_refund(entries, 1, 9, &used));
    assert(used == 0 && entries[0].id == 0 && entries[0].bytes == 0);
    assert(stsv_spill_tracker_admit(entries, 1, 10, 100, 100, &used));
    assert(used == 100 && entries[0].id == 10);
}

static void test_refund_unknown_id_is_noop(void) {
    stsv_spill_entry entries[1] = {{9, 80}};
    size_t used = 80;
    assert(!stsv_spill_tracker_refund(entries, 1, 10, &used));
    assert(used == 80 && entries[0].id == 9);
}

int main(void) {
    test_reupload_replaces_old_bytes();
    test_reupload_budget_uses_projected_usage();
    test_zero_id_is_never_admitted();
    test_full_table_rejects_untracked_texture();
    test_full_table_allows_existing_id_update();
    test_refund_frees_budget_and_slot();
    test_refund_unknown_id_is_noop();
    return 0;
}
