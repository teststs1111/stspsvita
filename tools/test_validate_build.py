import tempfile
import unittest
from pathlib import Path

from validate_build import ValidationError, requested_font_inventory


class RequestedFontInventoryTest(unittest.TestCase):
    def test_counts_non_empty_japanese_font_files(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            regular = root / "NotoSansCJKjp-Regular.otf"
            bold = root / "NotoSansCJKjp-Bold.otf"
            regular.write_bytes(b"regular-font")
            bold.write_bytes(b"bold-font")
            staged = {
                "font/jpn/NotoSansCJKjp-Regular.otf": regular,
                "font/jpn/NotoSansCJKjp-Bold.otf": bold,
                "font/04b03.ttf": root / "latin.ttf",
            }

            self.assertEqual(
                requested_font_inventory(staged, ["eng", "jpn"]),
                {"jpn": {"files": 2, "bytes": len(b"regular-font") + len(b"bold-font")}},
            )

    def test_rejects_requested_japanese_font_directory_without_files(self):
        with self.assertRaisesRegex(ValidationError, "jpn.*no font files"):
            requested_font_inventory({}, ["eng", "jpn"])

    def test_rejects_empty_japanese_font_file(self):
        with tempfile.TemporaryDirectory() as td:
            empty = Path(td) / "empty.otf"
            empty.write_bytes(b"")
            with self.assertRaisesRegex(ValidationError, "jpn.*empty font files"):
                requested_font_inventory({"font/jpn/empty.otf": empty}, ["jpn"])

    def test_english_only_has_no_cjk_font_requirement(self):
        self.assertEqual(requested_font_inventory({}, ["eng"]), {})


if __name__ == "__main__":
    unittest.main()
