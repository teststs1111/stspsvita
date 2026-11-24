#include "stsv_compressed_mip_diag.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static void test_round_up_power_of_two_matches_vitagl(void) {
    assert(stsv_mip_diag_round_up_po2(1) == 1);
    assert(stsv_mip_diag_round_up_po2(256) == 256);
    assert(stsv_mip_diag_round_up_po2(257) == 512);
    assert(stsv_mip_diag_round_up_po2(300) == 512);
    assert(stsv_mip_diag_round_up_po2(0) == 0);
    assert(stsv_mip_diag_round_up_po2(UINT32_C(0x80000001)) == 0);
}

static void test_dxt1_mip_chain_is_cumulative(void) {
    assert(stsv_compressed_mip_chain_bytes(256, 128, 0, 8) == 16384);
    assert(stsv_compressed_mip_chain_bytes(256, 128, 1, 8) == 20480);
    assert(stsv_compressed_mip_chain_bytes(256, 128, 2, 8) == 21504);
}

static void test_dxt5_uses_sixteen_byte_blocks(void) {
    assert(stsv_compressed_mip_chain_bytes(256, 128, 2, 16) == 43008);
}

static void test_non_power_of_two_base_matches_vitagl_alignment(void) {
    assert(stsv_compressed_mip_chain_bytes(300, 200, 0, 8) == 65536);
}

static void test_one_pixel_levels_repeat_like_vitagl(void) {
    assert(stsv_compressed_mip_chain_bytes(1, 1, 2, 8) == 24);
}

static void test_invalid_chain_inputs_return_zero(void) {
    assert(stsv_compressed_mip_chain_bytes(0, 128, 1, 8) == 0);
    assert(stsv_compressed_mip_chain_bytes(128, 128, -1, 8) == 0);
    assert(stsv_compressed_mip_chain_bytes(128, 128, 1, 0) == 0);
    assert(stsv_compressed_mip_chain_bytes(UINT32_C(0x80000001), 8, 1, 8) == 0);
}

static void test_base_records_update_and_clear_by_texture_id(void) {
    stsv_compressed_mip_base entries[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
    uint32_t width = 0, height = 0;
    int format = 0;
    assert(stsv_mip_diag_base_set(entries, 2, 17, 256, 128, 0x83F0));
    assert(stsv_mip_diag_base_get(entries, 2, 17, &width, &height, &format));
    assert(width == 256 && height == 128 && format == 0x83F0);
    assert(stsv_mip_diag_base_set(entries, 2, 17, 512, 256, 0x83F3));
    assert(stsv_mip_diag_base_get(entries, 2, 17, &width, &height, &format));
    assert(width == 512 && height == 256 && format == 0x83F3);
    assert(stsv_mip_diag_base_clear(entries, 2, 17));
    assert(!stsv_mip_diag_base_get(entries, 2, 17, &width, &height, &format));
}

static void test_base_table_rejects_zero_id_and_full_table(void) {
    stsv_compressed_mip_base entries[1] = {{1, 64, 64, 0x83F0}};
    assert(!stsv_mip_diag_base_set(entries, 1, 0, 64, 64, 0x83F0));
    assert(!stsv_mip_diag_base_set(entries, 1, 2, 64, 64, 0x83F0));
    assert(entries[0].id == 1 && entries[0].width == 64);
    assert(stsv_mip_diag_base_clear(entries, 1, 1));
    assert(stsv_mip_diag_base_set(entries, 1, 2, 32, 32, 0x83F3));
}

int main(void) {
    test_round_up_power_of_two_matches_vitagl();
    test_dxt1_mip_chain_is_cumulative();
    test_dxt5_uses_sixteen_byte_blocks();
    test_non_power_of_two_base_matches_vitagl_alignment();
    test_one_pixel_levels_repeat_like_vitagl();
    test_invalid_chain_inputs_return_zero();
    test_base_records_update_and_clear_by_texture_id();
    test_base_table_rejects_zero_id_and_full_table();
    return 0;
}
