import unittest

from vitagl_reserve_map import format_report, parse_reserve_calls


class ReserveCallsiteMapTest(unittest.TestCase):
    def test_maps_thumb_lr_with_state_bit(self):
        disassembly = """
8150db00 <vglVertexPointer>:
 8150db12: f7f2 fd5f bl 815005d4 <__wrap_vgl_reserve_data_pool>
815005d4 <__wrap_vgl_reserve_data_pool>:
 8150062a: f036 fe21 bl 81537270 <vgl_reserve_data_pool>
"""
        wrapped, bypass = parse_reserve_calls(disassembly)
        self.assertEqual(len(wrapped), 1)
        self.assertEqual(wrapped[0].call_address, 0x8150DB12)
        self.assertEqual(wrapped[0].instruction_return_address, 0x8150DB16)
        self.assertEqual(wrapped[0].thumb_lr, 0x8150DB17)
        self.assertEqual(wrapped[0].function, "vglVertexPointer")
        self.assertEqual(bypass, [])

    def test_detects_direct_reserve_call_outside_wrapper(self):
        disassembly = """
81501000 <unexpected_caller>:
 81501010: f036 fe21 bl 81537270 <vgl_reserve_data_pool>
"""
        wrapped, bypass = parse_reserve_calls(disassembly)
        self.assertEqual(wrapped, [])
        self.assertEqual(len(bypass), 1)
        self.assertEqual(bypass[0].thumb_lr, 0x81501015)

    def test_report_exposes_runtime_caller_value(self):
        wrapped, bypass = parse_reserve_calls("""
8150db00 <vglVertexPointer>:
 8150db12: f7f2 fd5f bl 815005d4 <__wrap_vgl_reserve_data_pool>
""")
        report = format_report(
            "Name : vitaGL\\nVersion : 0.0.0.r1488.g2bdbe89-1",
            wrapped,
            bypass,
        )
        self.assertIn("thumb_lr=0x8150db17 caller=0x8150db17", report)
        self.assertIn("Version : 0.0.0.r1488.g2bdbe89-1", report)
        self.assertIn("direct calls to real vgl_reserve_data_pool outside wrapper: 0", report)


if __name__ == "__main__":
    unittest.main()
