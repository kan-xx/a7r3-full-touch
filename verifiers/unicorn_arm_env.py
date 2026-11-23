"""Self-contained Unicorn ARM32 execution harness for Sony A7R III FW 3.10 DSOs.

Uses `touch-core/elf_tools.py` for pure-stdlib ELF32 PT_LOAD mapping, SHT_REL
relocation resolution, and SysV ARM32 `.plt` stub decoding — requiring only
`unicorn` (and user-supplied `--fw-root` / `$A7R3_FW_ROOT`) at runtime.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parent
CORE_DIR = REPO_ROOT / "touch-core"

if str(CORE_DIR) not in sys.path:
    sys.path.insert(0, str(CORE_DIR))

from elf_tools import PT_LOAD, Elf32Image  # noqa: E402


def parse_verifier_args(description: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "--fw-root",
        type=Path,
        default=Path(os.environ["A7R3_FW_ROOT"]) if "A7R3_FW_ROOT" in os.environ else None,
        help="Path to extracted Sony A7R III FW 3.10 root (or set A7R3_FW_ROOT)",
    )
    parser.add_argument(
        "--json-out",
        type=Path,
        default=None,
        help="Optional path to write full JSON verification report",
    )
    args = parser.parse_args()
    if args.fw_root is None:
        parser.error("--fw-root or A7R3_FW_ROOT environment variable is required")
    return args


def resolve_app_lib_dir(fw_root: Path) -> Path:
    """Locate `nflasha15/export/lib` containing `libObj.so` and `viewUnified*.so`."""
    candidates = [
        fw_root / "filesystems/nflasha15/export/lib",
        fw_root / "nflasha15/export/lib",
        fw_root / "lib",
        fw_root,
    ]
    for cand in candidates:
        if (cand / "libObj.so").is_file() and (cand / "viewUnified2.so").is_file():
            return cand
    raise FileNotFoundError(
        f"Could not find nflasha15/export/lib (libObj.so, viewUnified2.so) under {fw_root}"
    )


def map_elf_pt_load(uc, raw: bytes, base: int) -> Elf32Image:
    elf = Elf32Image(raw)
    for seg in elf.segments:
        if seg.p_type != PT_LOAD:
            continue
        lo = (base + seg.p_vaddr) & ~0xFFF
        hi = (base + seg.p_vaddr + seg.p_memsz + 0xFFF) & ~0xFFF
        uc.mem_map(lo, hi - lo)
        uc.mem_write(base + seg.p_vaddr, raw[seg.p_offset : seg.p_offset + seg.p_filesz])
    return elf


class ArmMachine:
    """Isolated ARM32 Unicorn machine with explicit PLT / address fixture ports."""

    def __init__(self, raw: bytes, base: int = 0x10000000, reloc: bool = False) -> None:
        from unicorn import UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_ARM, Uc

        self.u = u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.base = base
        self.elf = map_elf_pt_load(u, raw, base)
        self.ports = self.elf.plt_map()
        self.callbacks: dict[int | str, object] = {}
        self.sy = {
            s.name: base + s.st_value
            for s in self.elf.dynsym_symbols()
            if s.name and s.st_value
        }
        self.calls: list[tuple[str, list[int]]] = []

        if reloc:
            for r_off, r_type, name, _sym in self.elf.relocations():
                addr = base + r_off
                val = self.rd(addr)
                if r_type == 23:  # R_ARM_RELATIVE
                    val = (val + base) & 0xFFFFFFFF
                elif r_type in (2, 21, 22):  # R_ARM_ABS32 / GLOB_DAT / JUMP_SLOT
                    val = (self.sy[name] + (val if r_type == 2 else 0)) & 0xFFFFFFFF
                else:
                    raise AssertionError(f"Unsupported ARM relocation type {r_type}")
                self.wr(addr, val)

        for addr, size in ((0x60000000, 0x10000), (0x70000000, 0x10000), (0x71000000, 0x1000)):
            u.mem_map(addr, size)
        self.OBJ = 0x60000000
        self.RET = 0x71000000
        u.hook_add(UC_HOOK_CODE, self._hook)

    def rd(self, addr: int) -> int:
        return struct.unpack("<I", self.u.mem_read(addr, 4))[0]

    def wr(self, addr: int, val: int) -> None:
        self.u.mem_write(addr, struct.pack("<I", val & 0xFFFFFFFF))

    def ret(self, val: int = 0) -> None:
        from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0

        self.u.reg_write(UC_ARM_REG_R0, val & 0xFFFFFFFF)
        self.u.reg_write(UC_ARM_REG_PC, self.u.reg_read(UC_ARM_REG_LR))

    def _hook(self, u, pc: int, _size: int, _ctx) -> None:
        from unicorn.arm_const import (
            UC_ARM_REG_PC,
            UC_ARM_REG_R0,
            UC_ARM_REG_R1,
            UC_ARM_REG_R2,
            UC_ARM_REG_R3,
        )

        if pc == self.RET:
            u.emu_stop()
            return
        args = [u.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
        rel = pc - self.base
        if rel in self.callbacks:
            self.callbacks[rel](self, args)  # type: ignore[operator]
            return
        if rel in self.ports:
            name = self.ports[rel]
            self.calls.append((name, args))
            if name in self.callbacks:
                self.callbacks[name](self, args)  # type: ignore[operator]
                return
            if name in self.sy:
                u.reg_write(UC_ARM_REG_PC, self.sy[name])
                return
            raise AssertionError(("unexpected unmocked import", name, [hex(x) for x in args]))

    def invoke(self, offset: int, *args: int) -> int:
        from unicorn.arm_const import (
            UC_ARM_REG_LR,
            UC_ARM_REG_PC,
            UC_ARM_REG_R0,
            UC_ARM_REG_R1,
            UC_ARM_REG_R2,
            UC_ARM_REG_R3,
            UC_ARM_REG_SP,
        )

        self.u.reg_write(UC_ARM_REG_SP, 0x7000F000)
        self.u.reg_write(UC_ARM_REG_LR, self.RET | 1)
        for reg, val in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), args):
            self.u.reg_write(reg, val & 0xFFFFFFFF)
        self.u.emu_start(self.base + offset, 0, count=3000000)
        assert self.u.reg_read(UC_ARM_REG_PC) == self.RET, hex(self.u.reg_read(UC_ARM_REG_PC))
        return self.u.reg_read(UC_ARM_REG_R0)
