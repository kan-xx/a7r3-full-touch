#!/usr/bin/env python3
"""Unicorn ARM32 verifier for `GetRAWFileInfo` & `m_contType` `{5, 15}` JPEG/RAW contract.

Executes stock instructions in `libObj.so`, `viewUnified2.so`, and `viewUnified3.so`
to prove that normal JPEG (`rawflag=0 -> MWF 4 -> m_contType 5`) and Sony `.ARW` RAW
(`rawflag=1 -> MWF 14 -> m_contType 15`) are both accepted by `SinglePb` (`0x1f5bcd`,
mask `0xede0`) and `ViewZoomPb` condition 2 (`0x421d1`).
"""
from __future__ import annotations

import hashlib
import json
import struct

from unicorn_arm_env import ArmMachine, parse_verifier_args, resolve_app_lib_dir


def main() -> int:
    args = parse_verifier_args(__doc__)
    from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_R4

    lib_dir = resolve_app_lib_dir(args.fw_root)
    checks = 0
    cases = []

    def ck(cond: bool) -> None:
        nonlocal checks
        checks += 1
        assert cond

    obj_raw = (lib_dir / "libObj.so").read_bytes()
    m = ArmMachine(obj_raw)
    pins = {"libObj.so": hashlib.sha256(obj_raw).hexdigest()}

    lit = struct.unpack("<I", m.elf.read_vaddr(0x939F04, 4))[0]
    addr = 0x939EF8 + lit
    label = m.elf.read_vaddr(addr, 40).split(b"\x00")[0].decode()
    ck(addr == 0xF0C117 and label == "[PbSetupCtrl] GetRAWFileInfo")

    m.callbacks[0x96171C] = lambda mach, _a: mach.ret(0)
    m.callbacks[0x93CACC] = lambda mach, _a: mach.ret(0)
    m.wr(m.base + 0x35CE3C, m.base + 0xDC1F80)
    selected_holder = [0]

    def stop_value(mach, _a):
        selected_holder[0] = mach.u.reg_read(UC_ARM_REG_R4)
        mach.u.reg_write(UC_ARM_REG_PC, mach.RET | 1)

    m.callbacks[0x93E15A] = stop_value

    for rawflag, want_mwf, want_app in ((0, 4, 5), (1, 14, 15)):
        m.u.mem_write(m.OBJ, bytes(0x8000))
        m.u.mem_write(m.OBJ + 0x2316, bytes([rawflag]))
        out = m.OBJ + 0x7000
        m.u.mem_write(out, b"\xff")
        m.invoke(0x939EE1, m.OBJ, out)
        got = m.u.mem_read(out, 1)[0]
        ck(got == rawflag)
        m.invoke(0x93E0C9, m.OBJ, m.OBJ + 0x5000, m.OBJ + 0x6000, m.OBJ + 0x7000)
        ck(selected_holder[0] == want_mwf)
        app = m.invoke(0x35CE25, m.OBJ, selected_holder[0])
        ck(app == want_app)
        cases.append(
            {
                "native_RAWFileInfo_flag": got,
                "native_producer_MWF": selected_holder[0],
                "native_mapped_m_contType": app,
            }
        )

    expected = [0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 4, 16, 18, 0xFFFFFFFF, 19, 20, 21]
    for idx, val in enumerate(expected):
        ck(m.invoke(0x35CE25, m.OBJ, idx) == val)
    for idx in (22, 0xFFFFFFFF):
        ck(m.invoke(0x35CE25, m.OBJ, idx) == 0xFFFFFFFF)

    getter_out: dict[str, int] = {}
    m.callbacks["_Znwj"] = lambda mach, _a: mach.ret(mach.OBJ + 0x9000)

    def get_int(mach, call_args):
        getter_out["value"] = call_args[1]
        mach.ret(call_args[0])

    def add_param(mach, call_args):
        getter_out["key"] = call_args[1]
        mach.ret(0)

    m.callbacks[0x319290] = get_int
    m.callbacks["_ZN9ParamList3addEmP9ParamBase"] = add_param
    for ty in (5, 15):
        m.wr(m.OBJ + 0x68, ty)
        getter_out.clear()
        m.invoke(0x35CCC1, m.OBJ, 0xD01, m.OBJ + 0x8000, 0)
        ck(getter_out == {"value": ty, "key": 0x11FFFE25})
        cases.append({"original_D01_m_contType": ty, "returned_param": dict(getter_out)})

    v2_raw = (lib_dir / "viewUnified2.so").read_bytes()
    m2 = ArmMachine(v2_raw)
    pins["viewUnified2.so"] = hashlib.sha256(v2_raw).hexdigest()
    for name in ("_ZN9ParamListC1Ej", "_ZN9ParamListD1Ev"):
        m2.callbacks[name] = lambda mach, _a: mach.ret(0)
    m2.callbacks["_ZN8ViewBase12getModelInfoEPKciP9ParamListPKS2_"] = lambda mach, _a: mach.ret(1)
    m2.callbacks[0x15CA34] = lambda mach, _a: mach.ret(mach.OBJ + 0x1000)
    accepted = []
    for ty in [*range(24), 0xFFFFFFFF]:
        m2.callbacks[0x15CA2C] = lambda mach, _a, t=ty: mach.ret(t)
        got = m2.invoke(0x1F5BCD, m2.OBJ)
        want = int(ty <= 15 and ((0xEDE0 >> ty) & 1))
        ck(got == want)
        if got:
            accepted.append(ty)

    v3_raw = (lib_dir / "viewUnified3.so").read_bytes()
    m3 = ArmMachine(v3_raw)
    pins["viewUnified3.so"] = hashlib.sha256(v3_raw).hexdigest()
    for name in ("_ZN9ParamListC1Ej", "_ZN9ParamListD1Ev"):
        m3.callbacks[name] = lambda mach, _a: mach.ret(0)
    for busy in (0, 1):
        for zoom_busy in (0, 1, 2):
            calls: list[int] = []

            def info(mach, call_args, c=calls):
                c.append(call_args[2])
                mach.ret(1)

            m3.callbacks["_ZN8ViewBase12getModelInfoEPKciP9ParamListPKS2_"] = info
            m3.callbacks[0x20D3C] = lambda mach, _a: mach.ret(mach.OBJ + 0x1000)
            m3.callbacks[0x20D34] = lambda mach, _a, b=busy: mach.ret(b)
            m3.callbacks[0x20C48] = lambda mach, _a: mach.ret(mach.OBJ + 0x1000)
            m3.callbacks[0x20C40] = lambda mach, _a, b=zoom_busy: mach.ret(b)
            got = m3.invoke(0x421D1, m3.OBJ)
            ck(got == int(not busy and zoom_busy == 0))
            ck(calls == ([0xD22, 3] if not busy else [0xD22]))

    res = {
        "passed": True,
        "native_checks": checks,
        "original_pins": pins,
        "RAW_chain": cases[:2],
        "SinglePb_accepts": accepted,
    }
    if args.json_out:
        args.json_out.write_text(json.dumps(res, indent=2) + "\n")
    print(json.dumps(res))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
