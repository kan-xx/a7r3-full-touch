#!/usr/bin/env python3
"""Unicorn ARM32 verifier for `libObj.so` hit traversal, final-mode max, and bubbling.

Executes shipped ARM32 instructions at `0x5ccc31`, `0x5e239d`, `0x5cda2a`,
`0x5cd91a`, and `0x5ccca1` to prove `final_mode = max(ancestor.mode)` and
why `mode != 0` blocks press/release delivery in stock firmware.
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
        UC_ARM_REG_R6,
        UC_ARM_REG_R10,
        UC_ARM_REG_SP,
    )

    lib_dir = resolve_app_lib_dir(args.fw_root)
    obj_raw = (lib_dir / "libObj.so").read_bytes()

    BASE = 0x20000000
    NODE = 0x60000000
    ROOT = NODE
    CHILD = NODE + 0x200
    VT = NODE + 0x1000
    STACK = 0x70000000
    RET = 0x71000000
    LAST, HIT, EVENT, HOOK = RET + 0x100, RET + 0x110, RET + 0x120, RET + 0x130

    checks = 0

    def ck(ok: bool) -> None:
        nonlocal checks
        checks += 1
        assert ok

    def setup(root_mode: int = 0, child_mode: int = 0):
        u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        map_elf_pt_load(u, obj_raw, BASE)
        for addr, size in ((NODE, 0x3000), (STACK, 0x10000), (RET, 0x1000)):
            u.mem_map(addr, size)
        for addr, m in ((ROOT, root_mode), (CHILD, child_mode)):
            u.mem_write(addr, struct.pack("<I", VT))
            u.mem_write(addr + 0x21, bytes([m]))
        u.mem_write(VT + 0x14, struct.pack("<I", LAST | 1))
        u.mem_write(VT + 0x90, struct.pack("<I", HIT | 1))
        u.mem_write(VT + 0x68, struct.pack("<I", EVENT | 1))
        u.mem_write(VT + 0x6C, struct.pack("<I", BASE + 0x5E2157))
        u.reg_write(UC_ARM_REG_SP, STACK + 0xFF00)
        u.reg_write(UC_ARM_REG_LR, RET | 1)
        return u

    rows = []
    for root_mode in range(4):
        for child_mode in range(4):
            u = setup(root_mode, child_mode)
            observed = []

            def hook(uc, pc, _size, _arg):
                if pc == RET:
                    uc.emu_stop()
                    return
                if pc in (LAST, HIT, BASE + 0x5E2340, BASE + 0x5E235E):
                    obj = uc.reg_read(UC_ARM_REG_R0)
                    if pc == LAST:
                        val = CHILD if obj == ROOT else 0
                    elif pc == HIT:
                        val = int(obj == CHILD)
                    elif pc == BASE + 0x5E2340:
                        val = ROOT if obj == CHILD else 0
                    else:
                        val = 0
                    observed.append(
                        {
                            "pc": hex(pc - BASE) if BASE <= pc < BASE + 0x1000000 else hex(pc),
                            "this": hex(obj),
                            "returns": val,
                        }
                    )
                    uc.reg_write(UC_ARM_REG_R0, val)
                    uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))

            u.hook_add(UC_HOOK_CODE, hook)
            u.reg_write(UC_ARM_REG_R0, NODE + 0x800)
            u.reg_write(UC_ARM_REG_R1, ROOT)
            u.emu_start(BASE + 0x5CCC31, 0, count=500)
            hit = u.reg_read(UC_ARM_REG_R0)
            ck(hit == (CHILD if root_mode <= 1 and child_mode <= 1 else 0))
            ck(u.reg_read(UC_ARM_REG_PC) == RET)

            u.reg_write(UC_ARM_REG_R0, CHILD)
            u.reg_write(UC_ARM_REG_LR, RET | 1)
            u.emu_start(BASE + 0x5E239D, 0, count=200)
            final = u.reg_read(UC_ARM_REG_R0)
            ck(final == max(root_mode, child_mode))

            guard = {}
            for key, entry, admit, reject, reg in (
                ("press", 0x5CDA2A, 0x5CDA32, 0x5CDA86, UC_ARM_REG_R10),
                ("release", 0x5CD91A, 0x5CD924, 0x5CD9CA, UC_ARM_REG_R6),
            ):

                def stop(uc, pc, _size, _arg, a=admit, rj=reject):
                    if pc in (BASE + a, BASE + rj):
                        uc.emu_stop()

                g = u.hook_add(UC_HOOK_CODE, stop)
                u.reg_write(reg, CHILD)
                u.reg_write(UC_ARM_REG_LR, RET | 1)
                u.emu_start(BASE + entry + 1, 0, count=200)
                pc = u.reg_read(UC_ARM_REG_PC)
                ck(pc == BASE + (admit if final == 0 else reject))
                guard[key] = "admit" if pc == BASE + admit else "reject"
                u.hook_del(g)
            rows.append(
                {
                    "root_mode": root_mode,
                    "child_mode": child_mode,
                    "hit": hex(hit),
                    "final_mode": final,
                    "guards": guard,
                    "fixture_calls": observed,
                }
            )

    dispatch = []
    for root_hooked in (False, True):
        for consumes in (False, True):
            u = setup()
            u.mem_write(ROOT + 0x20, bytes([int(root_hooked)]))
            u.mem_write(VT + 0x6C, struct.pack("<I", HOOK | 1 if consumes else BASE + 0x5E2157))
            seen = []

            def dhook(uc, pc, _size, _arg):
                if pc == RET:
                    uc.emu_stop()
                    return
                if pc == BASE + 0x5E2340:
                    uc.reg_write(UC_ARM_REG_R0, ROOT if uc.reg_read(UC_ARM_REG_R0) == CHILD else 0)
                    uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))
                    return
                if pc == BASE + 0x5E2156:
                    seen.append("native_default_hook_returns1")
                if pc in (EVENT, HOOK):
                    seen.append("target_handler_fixture" if pc == EVENT else "consuming_hook_fixture")
                    uc.reg_write(UC_ARM_REG_R0, 0)
                    uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))

            u.hook_add(UC_HOOK_CODE, dhook)
            u.reg_write(UC_ARM_REG_R0, NODE + 0x800)
            u.reg_write(UC_ARM_REG_R1, CHILD)
            u.reg_write(UC_ARM_REG_R2, NODE + 0x900)
            u.emu_start(BASE + 0x5CCCA1, 0, count=300)
            ck(u.reg_read(UC_ARM_REG_PC) == RET)
            ck(("target_handler_fixture" in seen) == (not root_hooked or not consumes))
            dispatch.append(
                {"root_hooked": root_hooked, "custom_consumes": consumes, "observed": seen}
            )

    result = {
        "passed": True,
        "checks": checks,
        "filter_cases": len(rows),
        "dispatch_cases": len(dispatch),
        "artifact_sha256": hashlib.sha256(obj_raw).hexdigest(),
        "filters": rows,
        "dispatch": dispatch,
    }
    if args.json_out:
        args.json_out.write_text(json.dumps(result, indent=2) + "\n")
    print(
        json.dumps(
            {
                "passed": True,
                "checks": checks,
                "filter_cases": len(rows),
                "dispatch_cases": len(dispatch),
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
