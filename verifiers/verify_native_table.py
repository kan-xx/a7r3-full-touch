#!/usr/bin/env python3
"""Unicorn ARM32 verifier for `ZoomParameter` 18-step playback zoom ratio table (`libObj.so`).

Executes `libObj.so` constructors/accessors at `0x141521`, `0x14108b`, `0x14bfb9`,
and `0x1435a1` (with VFP enabled) to verify the 18-detent zoom curve `[100..800]`
for stock `AvbbcBackup` properties `0x35000a=17` and `0x35000b=800`.
"""
from __future__ import annotations

import hashlib
import json
import struct

from unicorn_arm_env import ArmMachine, parse_verifier_args, resolve_app_lib_dir


def main() -> int:
    args = parse_verifier_args(__doc__)
    from unicorn.arm_const import UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC

    lib_dir = resolve_app_lib_dir(args.fw_root)
    raw = (lib_dir / "libObj.so").read_bytes()
    m = ArmMachine(raw)
    u = m.u
    u.reg_write(UC_ARM_REG_C1_C0_2, 0x00F00000)
    u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
    m.callbacks[0x10452C] = lambda machine, _a: machine.ret(0)  # disable native log

    obj = m.OBJ
    vec = obj + 0x100
    checks = 0
    cases = []

    def ck(cond: bool) -> None:
        nonlocal checks
        checks += 1
        assert cond

    def configure(backup_max: int | None, backup_steps: int | None):
        seen = []

        def make_reader(expected: int, width: int, value: int | None):
            def port(machine, call_args):
                ck(call_args[0] == expected)
                seen.append([call_args[0], width, value])
                if value is not None:
                    u.mem_write(call_args[1], value.to_bytes(width, "little"))
                machine.ret(0 if value is not None else 1)

            return port

        m.callbacks["_ZN11AvbbcBackup9Read2ByteEmPt"] = make_reader(0x35000B, 2, backup_max)
        m.callbacks["_ZN11AvbbcBackup9Read1ByteEmPh"] = make_reader(0x35000A, 1, backup_steps)
        return seen

    for backup_max, backup_steps in ((800, 17), (800, 16), (None, None), (400, 8), (1000, 20)):
        seen = configure(backup_max, backup_steps)
        u.mem_write(obj, bytes(0x400))
        for off, val in ((0x24, vec), (0x28, vec), (0x2C, vec + 0x200)):
            m.wr(obj + off, val)
        status = m.invoke(0x141521, obj)
        ck(status == int(backup_max is None or backup_steps is None))
        ck(len(seen) == 2)
        n = 16 if backup_steps is None else backup_steps
        used = (m.rd(obj + 0x28) - vec) // 2
        ck(used == n + 1)
        ratios = [m.invoke(0x14108B, obj, i) for i in range(used)]
        ck(ratios == list(struct.unpack("<" + ("H" * used), u.mem_read(vec, used * 2))))
        if backup_steps == 17:
            ck(
                ratios
                == [100, 110, 120, 130, 150, 170, 200, 230, 270, 310, 350, 400, 460, 510, 580, 640, 720, 800]
            )
        if backup_steps == 20:
            ck(ratios[0] == ratios[1] == 130 and ratios[-1] == 1010)
        cases.append({"backups": seen, "native_status": status, "table_u16_by_index": ratios})

        mag = obj + 0x80
        owner = obj + 0x500
        m.wr(owner + 0xF8, obj)
        for before, mode, after in (
            (0, 2, 0),
            (0, 1, 1),
            (n // 2, 1, n // 2 + 1),
            (n // 2, 2, n // 2 - 1),
            (n, 1, n),
            (n, 2, n - 1),
        ):
            u.mem_write(mag, bytes(0x20))
            u.mem_write(mag + 0xA, struct.pack("<H", ratios[before]))
            u.mem_write(mag + 0xC, bytes([n, before]))
            ck(m.invoke(0x14BFB9, owner, mag, mode, 0) == 0)
            ck(u.mem_read(mag + 0xD, 1)[0] == after)
            ck(int.from_bytes(u.mem_read(mag + 0xA, 2), "little") == ratios[after])

    initial = []
    seq = obj + 0x500
    for mode, ratio in ((0, 110), (1, 0), (2, 0)):
        seen = configure(800, 17)
        u.mem_write(obj, bytes(0x600))
        for off, val in ((0x24, vec), (0x28, vec), (0x2C, vec + 0x200)):
            m.wr(obj + off, val)
        ck(m.invoke(0x141521, obj) == 0)
        u.mem_write(obj + 0x48, bytes([0x5A]) * 18)
        for off, val in ((0x34, 4100), (0x38, 4800), (0x3C, 5100), (0x40, 7000)):
            m.wr(obj + off, val)
        u.mem_write(seq + 0x50, bytes([mode]))
        m.wr(seq + 0x54, ratio)
        m.wr(seq + 0x58, obj)
        ck(m.invoke(0x1435A1, seq) == 0)
        info = bytes(u.mem_read(obj + 0x48, 18))
        ck(info[0] == (1 if mode == 2 else mode))
        ck(int.from_bytes(info[2:4], "little") == (110 if mode == 0 else 100))
        ck(int.from_bytes(info[0xA:0xC], "little") == 0)
        ck(u.mem_read(obj + 0x65, 1)[0] == (mode == 2))
        ck([m.rd(obj + i) for i in (0x34, 0x38, 0x3C, 0x40)] == [5000] * 4)
        initial.append({"mode": mode, "input": ratio, "mag_info_hex": info.hex()})

    result = {
        "passed": True,
        "checks": checks,
        "original_sha256": hashlib.sha256(raw).hexdigest(),
        "cases": cases,
        "init_mode_cases": initial,
        "anchors": {
            hex(a): m.elf.read_vaddr(a, 16).hex()
            for a in (0x141520, 0x14108A, 0x14BFB8, 0x14C298, 0x14C2C2, 0x1435A0)
        },
    }
    if args.json_out:
        args.json_out.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"passed": True, "checks": checks, "cases": len(cases)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
