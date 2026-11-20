#!/usr/bin/env python3
"""Map VitaGL reserve wrapper callsites to the raw ARM Thumb LR value."""

from __future__ import annotations

import argparse
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

_SYMBOL_RE = re.compile(r"^\\s*([0-9a-fA-F]+) <(.+)>:\\s*$")
_CALL_RE = re.compile(
    r"^\\s*([0-9a-fA-F]+):\\s+.*\\bblx?\\s+.*<(__wrap_vgl_reserve_data_pool|vgl_reserve_data_pool)>"
)


@dataclass(frozen=True)
class ReserveCall:
    call_address: int
    function: str
    function_offset: int

    @property
    def instruction_return_address(self) -> int:
        """Address of the next Thumb instruction, with the state bit clear."""
        return self.call_address + 4

    @property
    def thumb_lr(self) -> int:
        """Thumb BL return LR: next instruction address with bit 0 set."""
        return self.instruction_return_address | 1


def parse_reserve_calls(disassembly: str) -> tuple[list[ReserveCall], list[ReserveCall]]:
    current_function = "?"
    current_start = 0
    wrapped: list[ReserveCall] = []
    bypass: list[ReserveCall] = []

    for line in disassembly.splitlines():
        symbol = _SYMBOL_RE.match(line)
        if symbol:
            current_start = int(symbol.group(1), 16)
            current_function = symbol.group(2).removesuffix("()")
            continue

        call = _CALL_RE.match(line)
        if not call:
            continue

        address = int(call.group(1), 16)
        target = call.group(2)
        entry = ReserveCall(address, current_function, address - current_start)
        if target == "__wrap_vgl_reserve_data_pool":
            wrapped.append(entry)
        elif current_function != "__wrap_vgl_reserve_data_pool":
            bypass.append(entry)

    return wrapped, bypass


def format_report(version_text: str, wrapped: list[ReserveCall], bypass: list[ReserveCall]) -> str:
    lines = ["VitaGL package:"]
    version_lines = version_text.strip().splitlines()
    lines.extend(version_lines if version_lines else ["version metadata unavailable"])
    lines.append("")
    lines.append(f"wrapped vgl_reserve_data_pool callsites: {len(wrapped)}")
    for entry in wrapped:
        lines.append(
            f"call=0x{entry.call_address:08x} "
            f"return=0x{entry.instruction_return_address:08x} "
            f"thumb_lr=0x{entry.thumb_lr:08x} "
            f"caller=0x{entry.thumb_lr:08x} "
            f"function={entry.function} function_offset=0x{entry.function_offset:x}"
        )
    lines.append("")
    lines.append(f"direct calls to real vgl_reserve_data_pool outside wrapper: {len(bypass)}")
    for entry in bypass:
        lines.append(
            f"UNWRAPPED call=0x{entry.call_address:08x} "
            f"return=0x{entry.instruction_return_address:08x} "
            f"thumb_lr=0x{entry.thumb_lr:08x} "
            f"caller=0x{entry.thumb_lr:08x} "
            f"function={entry.function} function_offset=0x{entry.function_offset:x}"
        )
    return "\\n".join(lines) + "\\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--version-file", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    result = subprocess.run(
        ["arm-vita-eabi-objdump", "-d", "-C", str(args.binary)],
        check=True, capture_output=True, text=True,
    )
    wrapped, bypass = parse_reserve_calls(result.stdout)
    report = format_report(args.version_file.read_text(), wrapped, bypass)
    args.output.write_text(report)
    print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
