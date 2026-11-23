#!/usr/bin/env python3
"""Parameterized ARMv7 builder for the 3 read-only touch & playback diagnostic observers.

  - `ro211-observer-v14.elf`: Playback `ViewZoomPb` (`0xD3` / ID `211`) model & stream observer
  - `touch-observer-r2.elf`:  Ring-buffer (`A7TOBS01`) touch tap & callback observer
  - `ct-input-watch.elf`:     Cold-touch `input_watch` (`0x11013008` / `mouse0`) watcher
                              (built when `--fw-root` / `A7R3_FW_ROOT` is supplied)

(The general UI scene topology inspector `scene-current.elf` is maintained in `a7r3-native-ui`.)
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parent
CORE_SRC_DIR = REPO_ROOT / "touch-core/src"

CT_SONY_LIBS = [
    "libdl-2.11.2.so",
    "libpthread-2.11.2.so",
    "libc-2.11.2.so",
    "libm-2.11.2.so",
    "librt-2.11.2.so",
    "libstdc++.so.6.0.14",
    "libgcc_s.so.1",
]


def _run(cmd: list[str]) -> None:
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Command failed ({' '.join(cmd)}):\n{proc.stderr}")


def _pin(path: Path) -> dict[str, object]:
    raw = path.read_bytes()
    return {
        "name": path.name,
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
    }


def build_observers(out_dir: Path, zig_bin: str = "zig", fw_root: Path | None = None) -> dict[str, object]:
    out_dir.mkdir(parents=True, exist_ok=True)
    musl_base = [
        zig_bin,
        "cc",
        "-target",
        "arm-linux-musleabi",
        "-mcpu=cortex_a9",
        "-marm",
        "-mfloat-abi=soft",
        "-Os",
        "-static",
        f"-I{CORE_SRC_DIR}",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-unused-function",
    ]
    artifacts: list[dict[str, object]] = []

    # 1. ro211-observer-v14
    ro_dir = HERE / "ro211-observer-v14"
    ro_out = out_dir / "ro211-observer-v14.elf"
    _run(
        [
            *musl_base,
            f"-I{ro_dir}",
            str(ro_dir / "observer.c"),
            str(ro_dir / "typed_still.c"),
            "-o",
            str(ro_out),
        ]
    )
    artifacts.append(_pin(ro_out))

    # 2. touch-observer-r2
    tor_dir = HERE / "touch-observer-r2"
    tor_out = out_dir / "touch-observer-r2.elf"
    _run(
        [
            *musl_base,
            f"-I{tor_dir}",
            str(tor_dir / "observer.c"),
            str(CORE_SRC_DIR / "menu_snapshot.c"),
            "-o",
            str(tor_out),
        ]
    )
    artifacts.append(_pin(tor_out))

    # 3. ct-input-watch (links against Sony glibc-2.11 sysroot when --fw-root is provided)
    if fw_root is not None:
        lib_dir = fw_root / "filesystems/nflasha7/export/lib"
        if not (lib_dir / "libc-2.11.2.so").is_file():
            lib_dir = fw_root
        ct_dir = HERE / "ct-input-watch"
        ct_obj = out_dir / "input_watch.arm.o"
        ct_out = out_dir / "ct-input-watch.elf"
        gnu_base = [
            zig_bin,
            "cc",
            "-target",
            "arm-linux-gnueabi",
            "-mcpu=cortex_a9",
            "-marm",
            "-mfloat-abi=soft",
            "-Os",
            "-funwind-tables",
            "-fno-stack-protector",
            "-fno-builtin",
            "-nostdlib",
            "-Werror",
        ]
        _run([*gnu_base, "-DA7_MIN_DECL", f"-I{ct_dir}", "-c", str(ct_dir / "input_watch.c"), "-o", str(ct_obj)])
        _run(
            [
                *gnu_base,
                "-no-pie",
                "-Wl,--hash-style=sysv",
                "-Wl,--dynamic-linker=/lib/ld-linux.so.3",
                "-Wl,-e,_start",
                "-Wl,-z,now",
                "-Wl,-z,noexecstack",
                "-Wl,-z,defs",
                str(ct_dir / "entry.cpp"),
                str(ct_obj),
                *(str(lib_dir / lib) for lib in CT_SONY_LIBS),
                "-o",
                str(ct_out),
            ]
        )
        ct_obj.unlink(missing_ok=True)
        artifacts.append(_pin(ct_out))

    return {"artifacts": artifacts}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fw-root",
        type=Path,
        default=Path(os.environ["A7R3_FW_ROOT"]) if "A7R3_FW_ROOT" in os.environ else None,
        help="Optional extracted A7R III FW 3.10 root (enables building ct-input-watch.elf)",
    )
    parser.add_argument("--out-dir", type=Path, default=REPO_ROOT / "build/observers")
    parser.add_argument("--zig", default=os.environ.get("ZIG", shutil.which("zig") or "zig"))
    args = parser.parse_args()
    print(json.dumps(build_observers(args.out_dir, zig_bin=args.zig, fw_root=args.fw_root), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
