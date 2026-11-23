"""Offline Python unit tests for F910 sidecar codec, V17 profiler, and 128B A7TBM001 sealer."""
from __future__ import annotations

import hashlib
import struct
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SIDECAR_DIR = REPO_ROOT / "service-sidecar"
CORE_DIR = REPO_ROOT / "touch-core"

for p in (SIDECAR_DIR, CORE_DIR):
    if str(p) not in sys.path:
        sys.path.insert(0, str(p))

import build_core  # noqa: E402
import codec  # noqa: E402
import decode_v17_performance  # noqa: E402
from elf_tools import DT_NULL, DT_RELCOUNT, EM_ARM, ET_DYN, PF_X, PT_LOAD, SHT_DYNSYM, Elf32Image  # noqa: E402


def _make_synthetic_core_elf(
    code_size: int = 24576,
    exec_segment: bool = True,
    relcount: int = 7,
) -> bytes:
    """Build a minimal valid 32-bit ARM ET_DYN ELF image for offline manifest sealer tests."""
    # Layout:
    #   0x0000..0x0034: Elf32_Ehdr (52B)
    #   0x0034..0x0054: 1x Elf32_Phdr (32B)
    #   0x0100..0x0140: .dynstr
    #   0x0140..0x0170: .dynsym (3 symbols: NULL, a7r_touch_full_runtime_v7, A7R_Touch_Autostart)
    #   0x0170..0x0188: .dynamic (DT_RELCOUNT, DT_NULL)
    #   0x0200..0x0230: .shstrtab
    #   0x1000..0x1000+code_size: .text (contains non-zero ARM instructions at Autostart VA)
    #   aligned end: 5x Elf32_Shdr (5 * 40B)
    dynstr = b"\x00a7r_touch_full_runtime_v7\x00A7R_Touch_Autostart\x00"
    off_sym1 = 1
    off_sym2 = 1 + len("a7r_touch_full_runtime_v7") + 1

    shstrtab = b"\x00.dynstr\x00.dynsym\x00.dynamic\x00.shstrtab\x00"
    sh_dynstr_name = 1
    sh_dynsym_name = 9
    sh_dynamic_name = 17
    sh_shstr_name = 26

    text_off = 0x1000
    autostart_va = 0x2401  # Thumb symbol at 0x2400 | 1
    api_va = 0x1800

    total_load = text_off + code_size
    sh_off = (total_load + 15) & ~15
    buf = bytearray(sh_off + 5 * 40)

    # Place non-zero Thumb push prologue at 0x2400
    anchor = bytes.fromhex("2de9f04f83b0064600af8846054600f1")
    buf[0x2400 : 0x2410] = anchor
    buf[0x1800 : 0x1804] = b"\x70\x47\x00\xbf"

    buf[0x100 : 0x100 + len(dynstr)] = dynstr
    dynsym_off = 0x140
    # Sym 0: NULL
    # Sym 1: a7r_touch_full_runtime_v7 (STB_GLOBAL<<4 | STT_FUNC = 0x12, shndx=1)
    struct.pack_into("<IIIBBH", buf, dynsym_off + 16, off_sym1, api_va, 4, 0x12, 0, 1)
    # Sym 2: A7R_Touch_Autostart
    struct.pack_into("<IIIBBH", buf, dynsym_off + 32, off_sym2, autostart_va, 16, 0x12, 0, 1)

    dyn_off = 0x180
    struct.pack_into("<II", buf, dyn_off, DT_RELCOUNT, relcount)
    struct.pack_into("<II", buf, dyn_off + 8, DT_NULL, 0)

    buf[0x200 : 0x200 + len(shstrtab)] = shstrtab

    # Phdr[0]: PT_LOAD covering 0..total_load (R|X, never RWX)
    p_flags = (0x4 | PF_X) if exec_segment else 0x4
    struct.pack_into("<8I", buf, 52, PT_LOAD, 0, 0, 0, total_load, total_load, p_flags, 0x1000)

    # Section headers: 0=NULL, 1=.dynstr, 2=.dynsym, 3=.dynamic, 4=.shstrtab
    struct.pack_into("<10I", buf, sh_off + 1 * 40, sh_dynstr_name, 3, 2, 0x100, 0x100, len(dynstr), 0, 0, 1, 0)
    struct.pack_into("<10I", buf, sh_off + 2 * 40, sh_dynsym_name, SHT_DYNSYM, 2, dynsym_off, dynsym_off, 48, 1, 1, 4, 16)
    struct.pack_into("<10I", buf, sh_off + 3 * 40, sh_dynamic_name, 6, 2, dyn_off, dyn_off, 16, 1, 0, 4, 8)
    struct.pack_into("<10I", buf, sh_off + 4 * 40, sh_shstr_name, 3, 0, 0, 0x200, len(shstrtab), 0, 0, 1, 0)

    # Ehdr
    buf[0:16] = b"\x7fELF\x01\x01\x01" + b"\x00" * 9
    struct.pack_into(
        "<HHIIIIIHHHHHH",
        buf,
        16,
        ET_DYN,
        EM_ARM,
        1,
        0,
        52,
        sh_off,
        0x05000200,
        52,
        32,
        1,
        40,
        5,
        4,
    )
    return bytes(buf)


class SidecarCodecTests(unittest.TestCase):
    def test_f910_control_requests_and_op2_reply_decoding(self) -> None:
        nonce = bytes(range(1, 17))
        epoch = ("01234567-0123-4567-89ab-0123456789ab", 123, 456)
        metadata = {"major": 252, "minor": 15, "inode": 12345, "base": 0, "uid": 0, "gid": 0, "mode": 0o555}

        for op in (o for o in codec.OPS if o != 12):
            req = codec.request(
                op,
                nonce,
                epoch,
                321,
                metadata,
                row=2 if op in (7, 9) else None,
                enable=False if op == 11 else None,
            )
            self.assertEqual(len(req), 132)
            self.assertEqual(req[:4], struct.pack("<HH", 0, 0xF910))
            self.assertEqual(req[4:12], b"A7TRQ007")
            self.assertEqual(struct.unpack_from("<II", req, 12), (7, op))

        rep = bytearray(codec.REPLY_BYTES)
        rep[:8] = b"A7TRR007"
        struct.pack_into("<10I", rep, 8, 7, 2, 0, 0, 123, 333, 0, codec.REPORT_BYTES, codec.REPLY_BYTES, codec.BUILD)
        rep[48:64] = nonce
        meta = [0] * 24
        for k, v in {
            "version": 7,
            "bytes": codec.REPORT_BYTES,
            "build": codec.BUILD,
            "pid": 123,
            "ui_tid": 321,
            "phase": 2,
            "callbacks": 1,
            "callback_tid": 321,
            "callback_pid": 123,
            "dispatch_flag": 1,
            "return_matches": 1,
            "safe_to_unload": 1,
            "owns_reference": 1,
            "attempted": 513,
            "identity_valid": 1,
            "callback_operation": 7,
        }.items():
            meta[codec.META.index(k)] = v
        struct.pack_into("<24I", rep, 64, *meta)
        rep[160:176] = nonce

        rt = [0] * 1024
        for k, v in {
            "abi": 7,
            "bytes": 4096,
            "state": 2,
            "installed": 1,
            "gate": 1,
            "raw_callbacks": 1,
            "original_callbacks": 1,
            "source_lr": 0x10833605,
            "source_tid": 321,
            "root": 0x27A50580,
            "tab": 2,
            "page": 9,
            "row": 2,
            "row_count": 5,
            "synthetic_count": 1,
            "power_request": 13,
            "power_state": 4,
        }.items():
            rt[codec.RUNTIME_NAMES.index(k)] = v
        rt[64:78] = [
            0x1000,
            0x2000,
            0x11C8,
            0x3000,
            0,
            2,
            0x1EB0,
            0,
            *struct.unpack("<4I", struct.pack("<4f", 17.0, 126.0, 606.0, 48.0)),
            1,
            0,
        ]
        struct.pack_into("<1024I", rep, 22380, *rt)
        decoded = codec.reply(rep, 2, nonce)
        self.assertTrue(codec.callback_completed(decoded, epoch, 321, 7, 2))
        self.assertEqual(decoded["report"]["runtime"]["rows"][0]["world_bounds"]["height"], 48.0)

    def test_op12_autoload_discovery_roundtrip_and_rejections(self) -> None:
        epoch = ("01234567-0123-4567-89ab-0123456789ab", 152, 107)
        req = codec.discovery_request(epoch, 392)
        self.assertEqual(len(req), 132)
        self.assertEqual(struct.unpack_from("<II", req, 12), (7, 12))
        self.assertFalse(any(req[20:36]))  # op12 strictly uses all-zero nonce

        rep = bytearray(488)
        rep[:8] = b"A7TRD007"
        struct.pack_into("<10I", rep, 8, 7, 12, 0, 4, 152, 460, 0x105000, 424, 488, codec.BUILD)
        struct.pack_into("<12I", rep, 64, 7, 424, codec.BUILD, 0, 2, 21, 0x100000, 0x105000, 0x106000, 1, 2, 0)
        struct.pack_into(
            "<II16s40sIIQIIQ6I",
            rep,
            112,
            7,
            1,
            bytes(range(1, 17)),
            epoch[0].encode() + bytes(4),
            152,
            392,
            107,
            252,
            15,
            150,
            0,
            0,
            0,
            0o555,
            0,
            0,
        )
        core_path = b"/usr/lib/liba7r_touch_full_runtime_v7.so"
        rep[232 : 232 + len(core_path)] = core_path
        # file_count == 7 for V13..V17 viewUnified3.so verification stage
        struct.pack_into("<16I", rep, 328, 1, 64, 6, 0, 392, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 12)
        struct.pack_into("<4I", rep, 392, 1, 1, 0, 0)

        disc = codec.discovery_reply(rep, epoch)
        args = codec.identity_arguments(disc)
        self.assertEqual(args["ui_tid"], 392)
        self.assertEqual(len(codec.request(2, **args)), 132)

        zero_nonce = bytearray(rep)
        zero_nonce[128:144] = bytes(16)
        with self.assertRaises(ValueError):
            codec.discovery_reply(zero_nonce, epoch)


class V17ProfilerDecoderTests(unittest.TestCase):
    def test_v17_profiler_delta_and_monotonic_guard(self) -> None:
        before = [0] * 1024
        after = [0] * 1024
        for w in (before, after):
            w[0], w[1] = 7, 4096
            w[668], w[669] = 0x41375042, 2  # A7PB schema v2
        # Populate RP slot 15 (`native_wheel_return` at 840 + 15*3 = 885) and slot 8 (`fence` at 864)
        before[864:867] = [2, 1400, 1800]
        after[864:867] = [5, 1120, 1950]
        before[885:888] = [1, 820, 820]
        after[885:888] = [4, 790, 910]

        with tempfile.TemporaryDirectory() as tmp:
            b_path = Path(tmp) / "before.bin"
            a_path = Path(tmp) / "after.bin"
            b_path.write_bytes(struct.pack("<1024I", *before))
            a_path.write_bytes(struct.pack("<1024I", *after))

            report = decode_v17_performance.decode(b_path, a_path)
            self.assertEqual(report["perf"]["fence"]["count_delta"], 3)
            self.assertEqual(report["perf"]["native_wheel_return"]["count_delta"], 3)
            self.assertEqual(report["perf"]["native_wheel_return"]["last_us"], 790)
            self.assertEqual(report["perf"]["native_wheel_return"]["max_since_install_us"], 910)

            # Counter decrease must raise ValueError (different boot or installation)
            with self.assertRaises(ValueError):
                decode_v17_performance.decode(a_path, b_path)


class BootManifestSealerTests(unittest.TestCase):
    def test_elf_dt_relcount_zeroing_and_128b_manifest_seal(self) -> None:
        raw_elf = _make_synthetic_core_elf(code_size=24576, exec_segment=True, relcount=9)
        img = Elf32Image(raw_elf)
        self.assertTrue(img.zero_dt_relcount())
        self.assertEqual(dict(img.dynamic_tags())[DT_RELCOUNT], 0)

        owner_hex = "ab" * 32
        nonce_hex = "cd" * 16
        sealed = build_core.seal_boot_manifest(bytes(img.raw), owner_hex, nonce_hex)
        self.assertEqual(len(sealed), 128)
        self.assertEqual(sealed[:8], b"A7TBM001")
        abi, nbytes, build_id, entry_api, core_len, entry_off = struct.unpack_from("<6I", sealed, 8)
        self.assertEqual((abi, nbytes, build_id, entry_api), (1, 128, 0x3111FE07, 1))
        self.assertEqual(core_len, len(img.raw))
        self.assertEqual(entry_off, 0x2401)
        self.assertEqual(sealed[32:64], hashlib.sha256(bytes(img.raw)).digest())
        self.assertEqual(sealed[64:80], bytes.fromhex("2de9f04f83b0064600af8846054600f1"))
        self.assertEqual(sealed[80:112], bytes.fromhex(owner_hex))
        self.assertEqual(sealed[112:128], bytes.fromhex(nonce_hex))

        # Non-executable PT_LOAD or zero nonce must fail sealing
        non_exec_elf = _make_synthetic_core_elf(code_size=24576, exec_segment=False)
        with self.assertRaises(ValueError):
            build_core.seal_boot_manifest(non_exec_elf, owner_hex, nonce_hex)
        with self.assertRaises(ValueError):
            build_core.seal_boot_manifest(bytes(img.raw), owner_hex, "00" * 16)


if __name__ == "__main__":
    unittest.main(verbosity=2)
