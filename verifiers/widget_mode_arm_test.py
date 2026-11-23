#!/usr/bin/env python3
"""Unicorn ARM32 verifier for `MenuSelectMode` / `BarCtrlDial` widget mode transitions.

Proves that `widget_mode == 3` acts as a sticky touch-veto lock in Sony's stock
`viewUnified2.so` + `libObj.so` wrappers (`0x54b178`, `0x530790`).
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
        UC_ARM_REG_SP,
    )

    lib_dir = resolve_app_lib_dir(args.fw_root)
    v2_raw = (lib_dir / "viewUnified2.so").read_bytes()
    obj_raw = (lib_dir / "libObj.so").read_bytes()

    VB, OB = 0x10000000, 0x20000000
    OBJECT = 0x60000000
    VT = OBJECT + 0x1000
    STACK = 0x70000000
    RET = 0x71000000
    VIS = RET + 0x100

    checks = 0
    rows = []

    def ck(cond: bool) -> None:
        nonlocal checks
        checks += 1
        assert cond

    for name, offset in (("MenuSelectMode", 0x54B178), ("BarCtrlDial", 0x530790)):
        for mode in (0, 1, 2, 3):
            for boolean in (0, 1, 2, 255):
                u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
                map_elf_pt_load(u, v2_raw, VB)
                map_elf_pt_load(u, obj_raw, OB)
                u.mem_map(OBJECT, 0x2000)
                u.mem_map(STACK, 0x10000)
                u.mem_map(RET, 0x1000)
                u.mem_write(OBJECT, struct.pack("<I", VT))
                u.mem_write(OBJECT + 0x21, bytes([mode]))
                u.mem_write(OBJECT + 0x180, struct.pack("<I", 99))
                u.mem_write(VT + 0x108, struct.pack("<I", OB + 0x400693))
                u.mem_write(VT + 0x48, struct.pack("<I", VIS | 1))
                ports = []

                def hook(uc, pc, _size, _arg):
                    if pc == RET:
                        uc.emu_stop()
                        return
                    if pc == OB + 0xFEE54:
                        uc.reg_write(UC_ARM_REG_PC, OB + 0x5C9499)
                        return
                    if pc == OB + 0x1035C8:
                        uc.reg_write(UC_ARM_REG_PC, OB + 0x5C9055)
                        return
                    if pc in (OB + 0xFE7F4, VIS):
                        ports.append(
                            {
                                "port": "MouseArea::setEnable fixture"
                                if pc == OB + 0xFE7F4
                                else "Widget::reset(bool) fixture",
                                "this": uc.reg_read(UC_ARM_REG_R0),
                                "value": uc.reg_read(UC_ARM_REG_R1),
                            }
                        )
                        uc.reg_write(UC_ARM_REG_R0, 0)
                        uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))

                u.hook_add(UC_HOOK_CODE, hook)
                u.reg_write(UC_ARM_REG_SP, STACK + 0xFF00)
                u.reg_write(UC_ARM_REG_R0, OBJECT)
                u.reg_write(UC_ARM_REG_R1, boolean)
                u.reg_write(UC_ARM_REG_LR, RET | 1)
                u.emu_start((VB + offset) | 1, 0, count=300)
                ck(u.reg_read(UC_ARM_REG_PC) == RET)
                default = struct.unpack("<I", u.mem_read(OBJECT + 0x180, 4))[0]
                actual = u.mem_read(OBJECT + 0x21, 1)[0]
                want = 0 if boolean else 2
                ck(default == want)
                ck(actual == (3 if mode == 3 else want))
                if mode == 3:
                    ck(not ports)
                rows.append(
                    {
                        "wrapper": name,
                        "input_bool": boolean,
                        "prior_widget_mode": mode,
                        "default_mode": default,
                        "actual_widget_mode": actual,
                        "external_ports": ports,
                    }
                )

    result = {
        "passed": True,
        "cases": len(rows),
        "checks": checks,
        "artifacts": {
            "viewUnified2.so": hashlib.sha256(v2_raw).hexdigest(),
            "libObj.so": hashlib.sha256(obj_raw).hexdigest(),
        },
        "rows": rows,
    }
    if args.json_out:
        args.json_out.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"passed": True, "cases": len(rows), "checks": checks}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
