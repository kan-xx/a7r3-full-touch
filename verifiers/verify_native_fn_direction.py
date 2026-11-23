#!/usr/bin/env python3
"""Unicorn ARM32 verifier for `viewUnified4.so` `Fn` 12-cell focus navigation (`0x19432f`).

Executes original FW 3.10 Thumb dispatcher actions `0`/`1`, verifying wrap-around
and empty-slot (`-0xe1`) skipping across 12-cell `Fn` configurations.
"""
from __future__ import annotations

import hashlib
import json
import struct

from unicorn_arm_env import map_elf_pt_load, parse_verifier_args, resolve_app_lib_dir


def main() -> int:
    args = parse_verifier_args(__doc__)
    from unicorn import UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_ARM, Uc
    from unicorn.arm_const import (
        UC_ARM_REG_LR,
        UC_ARM_REG_PC,
        UC_ARM_REG_R0,
        UC_ARM_REG_R1,
        UC_ARM_REG_R2,
        UC_ARM_REG_R3,
        UC_ARM_REG_SP,
    )

    lib_dir = resolve_app_lib_dir(args.fw_root)
    raw = (lib_dir / "viewUnified4.so").read_bytes()

    BASE = 0x10000000
    OBJ = 0x50000000
    STACK = 0x60000000
    RET = 0x70000000
    PORT = RET + 0x1000

    u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    elf = map_elf_pt_load(u, raw, BASE)
    for addr, size in ((OBJ, 0x10000), (STACK, 0x10000), (RET, 0x20000)):
        u.mem_map(addr, size)

    ports: dict[int, str] = {}
    nextp = PORT
    for r_off, r_type, name, sym in elf.relocations():
        addr = BASE + r_off
        val = struct.unpack("<I", u.mem_read(addr, 4))[0]
        if r_type == 23:
            u.mem_write(addr, struct.pack("<I", (BASE + val) & 0xFFFFFFFF))
        elif r_type in (2, 21, 22):
            if sym.st_value:
                val = BASE + sym.st_value + (val if r_type == 2 else 0)
            else:
                existing = [k for k, n in ports.items() if n == name]
                val = existing[0] if existing else nextp | 1
                if not existing:
                    ports[val] = name
                    nextp += 4
            u.mem_write(addr, struct.pack("<I", val & 0xFFFFFFFF))

    fixtures = {
        0x18EB18: 0,
        0x19110C: 0,
        0x18BE08: 0,
        0x18BE3C: 0,
        0x18C518: 1,
        0x18C600: 1,
        0x191880: 1,
        0x18C680: 0,
        0x18BD7C: 0,
        0x18BE54: 0,
        0x190C7C: 0,
        0x18E22C: 0,
        0x19326C: 0,
        0x18E9EC: 0,
        0x18CC08: 0,
    }
    called: list[str] = []

    def ret(val: int = 0) -> None:
        u.reg_write(UC_ARM_REG_R0, val & 0xFFFFFFFF)
        u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))

    def code_hook(uc, pc, _n, _ctx):
        if pc == RET:
            uc.emu_stop()
            return
        if pc - BASE in fixtures:
            called.append(hex(pc - BASE))
            ret(fixtures[pc - BASE])
            return
        if pc | 1 in ports:
            called.append(ports[pc | 1])
            ret()

    u.hook_add(UC_HOOK_CODE, code_hook)

    def wr(addr: int, val: int) -> None:
        u.mem_write(addr, struct.pack("<I", val & 0xFFFFFFFF))

    def rd(addr: int) -> int:
        return struct.unpack("<I", u.mem_read(addr, 4))[0]

    def run(current: int, valid: list[int], action: int, flags: dict[int, int] | None = None):
        called.clear()
        u.mem_write(OBJ, bytes(0x398))
        wr(OBJ + 0x14C, current)
        wr(OBJ + 0x1C4, 2)
        wr(OBJ + 0x360, 1)
        for idx in range(12):
            wr(OBJ + 0x298 + idx * 8, idx + 10 if idx in valid else -0xE1)
        for off, val in (flags or {}).items():
            u.mem_write(OBJ + off, bytes([val]))
        u.reg_write(UC_ARM_REG_SP, STACK + 0xF000)
        u.reg_write(UC_ARM_REG_LR, RET | 1)
        u.reg_write(UC_ARM_REG_R0, OBJ)
        u.reg_write(UC_ARM_REG_R1, action)
        u.reg_write(UC_ARM_REG_R2, 0)
        u.reg_write(UC_ARM_REG_R3, 0)
        u.emu_start(BASE + 0x19432F, 0, count=30000)
        assert u.reg_read(UC_ARM_REG_PC) == RET, hex(u.reg_read(UC_ARM_REG_PC))
        return rd(OBJ + 0x14C), rd(OBJ + 0x360), list(called)

    cases = []
    assertions = 0
    for valid in (list(range(12)), [0, 2, 4, 6, 8, 10], [0], [11], [1, 4, 8, 9]):
        for current in range(12):
            for action in (0, 1):
                got, state, _calls = run(current, valid, action)
                step = -1 if action == 0 else 1
                want = (current + step) % 12
                while want not in valid:
                    want = (want + step) % 12
                assert got == want, (current, valid, action, got, want)
                assert state == 1
                assertions += 2
                cases.append({"current": current, "valid": valid, "action": action, "result": got})

    for action in (0, 1):
        for flags in ({0x37C: 1}, {0x200 if action == 0 else 0x201: 1}):
            got, _st, _c = run(5, list(range(12)), action, flags)
            assert got == 5
            assertions += 1
            cases.append({"action": action, "flags": {hex(k): v for k, v in flags.items()}, "result": got})
        for helper in (0x18C518, 0x18C600, 0x191880):
            fixtures[helper] = 0
            got, _st, _c = run(5, list(range(12)), action)
            fixtures[helper] = 1
            assert got == 5
            assertions += 1
            cases.append({"action": action, "rejecting_fixture": hex(helper), "result": got})

    res = {
        "passed": True,
        "sha256": hashlib.sha256(raw).hexdigest(),
        "native_dispatcher_thumb": "0x19432f",
        "cases": len(cases),
        "assertions": assertions,
        "results": cases,
    }
    if args.json_out:
        args.json_out.write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps({k: v for k, v in res.items() if k != "results"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
