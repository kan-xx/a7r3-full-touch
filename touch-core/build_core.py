#!/usr/bin/env python3
"""Self-contained builder & manifest sealer for A7R III Full Touch V17 (`a7r3-full-touch`).

Builds from repository sources:
  1. `liba7r_touch_full_runtime_v7.so` — 34-TU V17 full-screen touch & pinch-zoom core
  2. `libproF9.so`                     — Senser `0xF910` diagnostic/control sidecar
  3. `a7r_touch_boot_v1.bin`           — 128-byte `A7TBM001` cold-boot manifest seal
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parent
CORE_SRC_DIR = HERE / "src"
SIDECAR_DIR = REPO_ROOT / "service-sidecar"

if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from elf_tools import Elf32Image  # noqa: E402

TBM_MAGIC = b"A7TBM001"
TBM_ABI = 1
TBM_BYTES = 128
TBM_BUILD = 0x3111FE07
TBM_ENTRY_API = 1

# 34 translation units in link order
CORE_TRANSLATION_UNITS = [
    "native.c",
    "input_hook.cpp",
    "autoload_flow.cpp",
    "autoload_ui.c",
    "boot_manifest.c",
    "bounded_file_hash.c",
    "power_gate.c",
    "tap_session.c",
    "report_cache.c",
    "marker_event.c",
    "surface_actions.c",
    "surface_hit.c",
    "native_catalog.c",
    "menu_regions.c",
    "envelope_event.c",
    "envelope_receiver.c",
    "synchronous_directed.cpp",
    "native_mode_policy.c",
    "touch_diagnostic.c",
    "ffc_adapter.c",
    "ffc_projection.c",
    "ffc_raw_pair.cpp",
    "ffc_geometry.c",
    "ffc_runtime_bridge.c",
    "input_tap.c",
    "tree.c",
    "navigation.c",
    "navigation_catch.cpp",
    "menu_snapshot.c",
    "menu_maps.c",
    # Sony sysroot DSOs are inserted between menu_maps.c and pinch.c to preserve exact symbol/GOT layout
    "__SONY_LIBS__",
    "pinch.c",
    "pinch_dispatch.cpp",
    "image_lease.c",
    "image_reference_read.c",
    "pinch_timer.cpp",
    "pinch_capture.c",
    "pb_held_gui.c",
    "pinch_passive.c",
]

CORE_SONY_LIBS = [
    "libdl-2.11.2.so",
    "libpthread-2.11.2.so",
    "libc-2.11.2.so",
    "libstdc++.so.6.0.14",
    "libgcc_s.so.1",
]

SIDECAR_SONY_LIBS = [
    "libdl-2.11.2.so",
    "libpthread-2.11.2.so",
    "libc-2.11.2.so",
    "libgcc_s.so.1",
]


def resolve_sysroot_lib_dir(fw_root: Path) -> Path:
    """Accept either extracted firmware root (.../verified-3.10) or direct lib/ directory."""
    candidates = [
        fw_root / "filesystems/nflasha7/export/lib",
        fw_root / "nflasha7/export/lib",
        fw_root / "lib",
        fw_root,
    ]
    for cand in candidates:
        if (cand / "libc-2.11.2.so").is_file() and (cand / "libstdc++.so.6.0.14").is_file():
            return cand
    raise FileNotFoundError(
        f"Could not locate Sony A7R III nflasha7 sysroot libraries under {fw_root} "
        "(expected filesystems/nflasha7/export/lib/libc-2.11.2.so)"
    )


def seal_boot_manifest(
    core_elf_bytes: bytes,
    owner_sha256_hex: str,
    install_nonce_hex: str,
    entry_symbol: str = "A7R_Touch_Autostart",
) -> bytes:
    """Seal a 128-byte `A7TBM001` cold-boot manifest (`struct touch_boot_manifest`).

    Verifies that `entry_symbol` resides inside an executable `PT_LOAD` segment,
    extracts its 16-byte instruction anchor at `entry_va & ~1`, and binds the core
    SHA-256, autoload owner SHA-256, and 16-byte installation nonce.
    """
    owner_sha = bytes.fromhex(owner_sha256_hex)
    nonce = bytes.fromhex(install_nonce_hex)
    if len(owner_sha) != 32 or not any(owner_sha):
        raise ValueError("owner_sha256_hex must be 64 hex chars (32 non-zero bytes)")
    if len(nonce) != 16 or not any(nonce):
        raise ValueError("install_nonce_hex must be 32 hex chars (16 non-zero bytes)")

    core_len = len(core_elf_bytes)
    if not (16384 <= core_len <= 1048576):
        raise ValueError(f"core_bytes={core_len} outside tbm_validate [16384, 1048576] range")

    elf = Elf32Image(core_elf_bytes)
    info = elf.verify_dso_invariants()
    entries: dict[str, int] = info["export_entries"]  # type: ignore[assignment]
    if entry_symbol not in entries:
        raise ValueError(f"Export symbol {entry_symbol!r} not found in core ELF")

    entry_va = entries[entry_symbol]
    if entry_va == 0 or (entry_va & ~1) > 0x01000000:
        raise ValueError(f"Invalid entry_offset 0x{entry_va:x}")

    anchor_16 = elf.read_vaddr(entry_va & ~1, 16, require_exec=True)
    if not any(anchor_16):
        raise ValueError("16-byte entry instruction anchor cannot be all-zero")

    core_sha = hashlib.sha256(core_elf_bytes).digest()
    manifest = (
        TBM_MAGIC
        + struct.pack(
            "<6I",
            TBM_ABI,
            TBM_BYTES,
            TBM_BUILD,
            TBM_ENTRY_API,
            core_len,
            entry_va,
        )
        + core_sha
        + anchor_16
        + owner_sha
        + nonce
    )
    assert len(manifest) == TBM_BYTES
    return manifest


def _compile_and_normalize(cmd: list[str], output_path: Path) -> dict[str, object]:
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Build failed for {output_path.name}:\n{proc.stderr}")
    elf = Elf32Image(output_path.read_bytes())
    elf.zero_dt_relcount()
    normalized = bytes(elf.raw)
    output_path.write_bytes(normalized)
    meta = elf.verify_dso_invariants()
    return {
        "name": output_path.name,
        "bytes": len(normalized),
        "sha256": hashlib.sha256(normalized).hexdigest(),
        **meta,
    }


def build_all(
    fw_root: Path,
    out_dir: Path,
    zig_bin: str = "zig",
    owner_so: Path | None = None,
    owner_sha256_hex: str | None = None,
    install_nonce_hex: str | None = None,
) -> dict[str, object]:
    lib_dir = resolve_sysroot_lib_dir(fw_root)
    out_dir.mkdir(parents=True, exist_ok=True)

    common_flags = [
        zig_bin,
        "cc",
        "-target",
        "arm-linux-gnueabi",
        "-mcpu=cortex_a9",
        "-marm",
        "-mfloat-abi=soft",
        "-Os",
        f"-I{CORE_SRC_DIR}",
        f"-I{SIDECAR_DIR}",
        "-fno-stack-protector",
        "-fno-builtin",
        "-funwind-tables",
        "-fvisibility=hidden",
        "-fomit-frame-pointer",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-nostdlib",
        "-shared",
        "-fPIC",
        "-Wl,--hash-style=sysv",
        "-Wl,-z,defs",
        "-Wl,-z,noexecstack",
        "-Wl,-z,now",
    ]

    # 1. Build liba7r_touch_full_runtime_v7.so
    core_out = out_dir / "liba7r_touch_full_runtime_v7.so"
    core_inputs: list[str] = []
    for item in CORE_TRANSLATION_UNITS:
        if item == "__SONY_LIBS__":
            core_inputs.extend(str(lib_dir / lib) for lib in CORE_SONY_LIBS)
        else:
            core_inputs.append(str(CORE_SRC_DIR / item))

    core_cmd = [
        *common_flags,
        f"-Wl,--version-script={CORE_SRC_DIR / 'liba7r_touch_full_runtime_v7.so.map'}",
        "-Wl,-soname,liba7r_touch_full_runtime_v7.so",
        *core_inputs,
        "-o",
        str(core_out),
    ]
    core_meta = _compile_and_normalize(core_cmd, core_out)
    if core_meta["exports"] != ["A7R_Touch_Autostart", "a7r_touch_full_runtime_v7"]:
        raise RuntimeError(f"Unexpected core exports: {core_meta['exports']}")

    # 2. Build libproF9.so
    f9_out = out_dir / "libproF9.so"
    f9_cmd = [
        *common_flags,
        f"-Wl,--version-script={SIDECAR_DIR / 'libproF9.so.map'}",
        "-Wl,-soname,libproF9.so",
        str(SIDECAR_DIR / "senser_f910.c"),
        str(SIDECAR_DIR / "service_provider.c"),
        *(str(lib_dir / lib) for lib in SIDECAR_SONY_LIBS),
        "-o",
        str(f9_out),
    ]
    f9_meta = _compile_and_normalize(f9_cmd, f9_out)
    if f9_meta["exports"] != ["commandF910"]:
        raise RuntimeError(f"Unexpected libproF9.so exports: {f9_meta['exports']}")

    result: dict[str, object] = {
        "core": core_meta,
        "sidecar": f9_meta,
    }

    if owner_so is not None and not owner_sha256_hex:
        owner_sha256_hex = hashlib.sha256(owner_so.read_bytes()).hexdigest()

    # 3. Optional 128B A7TBM001 cold-boot manifest sealing
    if owner_sha256_hex:
        if not install_nonce_hex:
            raw_nonce = os.urandom(16)
            if not any(raw_nonce):
                raw_nonce = b"\x01" + bytes(15)
            install_nonce_hex = raw_nonce.hex()
        manifest_bytes = seal_boot_manifest(
            core_out.read_bytes(),
            owner_sha256_hex=owner_sha256_hex,
            install_nonce_hex=install_nonce_hex,
        )
        manifest_out = out_dir / "a7r_touch_boot_v1.bin"
        manifest_out.write_bytes(manifest_bytes)
        result["boot_manifest"] = {
            "name": manifest_out.name,
            "bytes": len(manifest_bytes),
            "sha256": hashlib.sha256(manifest_bytes).hexdigest(),
            "owner_sha256": owner_sha256_hex,
            "install_nonce": install_nonce_hex,
            "entry_offset": core_meta["export_entries"]["A7R_Touch_Autostart"],  # type: ignore[index]
        }

    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fw-root",
        type=Path,
        default=Path(os.environ["A7R3_FW_ROOT"]) if "A7R3_FW_ROOT" in os.environ else None,
        help="Path to extracted A7R III FW 3.10 root (or set A7R3_FW_ROOT)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=REPO_ROOT / "build",
        help="Output directory for compiled DSOs and optional boot manifest (default: ./build)",
    )
    parser.add_argument(
        "--zig",
        default=os.environ.get("ZIG", shutil.which("zig") or "zig"),
        help="Zig executable path (default: $ZIG or `zig` in PATH)",
    )
    parser.add_argument(
        "--owner-so",
        type=Path,
        default=Path(os.environ["A7R3_TOUCH_OWNER_SO"]) if "A7R3_TOUCH_OWNER_SO" in os.environ else None,
        help="Optional path to built liba7r_startup_full_v14.so to auto-compute --owner-sha256",
    )
    parser.add_argument(
        "--owner-sha256",
        default=os.environ.get("A7R3_TOUCH_OWNER_SHA256"),
        help="Optional 64-hex SHA-256 of the cold-boot autoload owner SO to seal a7r_touch_boot_v1.bin",
    )
    parser.add_argument(
        "--install-nonce",
        default=os.environ.get("A7R3_TOUCH_INSTALL_NONCE"),
        help="Optional 32-hex installation nonce (auto-generated if omitted when sealing manifest)",
    )
    args = parser.parse_args()
    if args.fw_root is None:
        parser.error("--fw-root or A7R3_FW_ROOT environment variable is required")

    summary = build_all(
        fw_root=args.fw_root,
        out_dir=args.out_dir,
        zig_bin=args.zig,
        owner_so=args.owner_so,
        owner_sha256_hex=args.owner_sha256,
        install_nonce_hex=args.install_nonce,
    )
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
