#!/usr/bin/env python3
"""Offline host test runner for `a7r3-full-touch` (`make test`).

Runs 100% offline with ZERO Sony firmware blobs required:
  1. Python unittest suite (`test_sidecar_and_manifest.py`): F910 codec ops 1..12,
     V17 RP profiler delta decoder, ELF32 DT_RELCOUNT normalizer & 128B A7TBM001 sealer.
  2. Host ASan+UBSan C/C++ suite 1 (`test_runtime_pinch`): 89 V17 runtime_pinch cases,
     108 4x canvas capture cases, 48 held-GUI mouse0 fence cases, 24 squared-distance
     geometry cases, and 2 RP slot 15 normal-exit-only zoom dispatch profiler cases.
  3. Host ASan+UBSan C suite 2 (`test_menu_regions`): Top-tab 5px-overlap midpoint split
     & 48px page-dot/arrow hit-zone verification across 640x480 ViewMenu (0xAA).
  4. Host ASan+UBSan C suite 3 (`test_boot_manifest_and_lease`): 128B A7TBM001 validator
     & STREAM_FILE88B {5, 15} JPEG/RAW discrete image lease & double-read reference reader.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TESTS_DIR.parent
CORE_SRC = REPO_ROOT / "touch-core/src"


def _run(cmd: list[str], env: dict[str, str] | None = None) -> str:
    proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Command failed ({' '.join(cmd)}):\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}"
        )
    return proc.stdout


def run_python_tests() -> None:
    print("=== [1/4] Running Python unit tests (F910 codec, V17 profiler, A7TBM001 sealer) ===")
    suite = unittest.defaultTestLoader.discover(str(TESTS_DIR), pattern="test_*.py")
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    if not result.wasSuccessful():
        raise SystemExit(1)


def run_host_c_suites() -> None:
    cc = os.environ.get("CC", shutil.which("clang") or "cc")
    cxx = os.environ.get("CXX", shutil.which("clang++") or "c++")
    san_flags = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    warn_flags = ["-Wall", "-Wextra", "-Werror"]
    run_env = {
        **os.environ,
        "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
        "UBSAN_OPTIONS": "halt_on_error=1",
    }

    with tempfile.TemporaryDirectory(prefix="a7r3-touch-tests-") as tmp_str:
        tmp = Path(tmp_str)

        # Suite 2: V17 runtime_pinch + capture + held_gui + geometry + dispatch_profile
        print("\n=== [2/4] Building & running ASan/UBSan V17 runtime_pinch + geometry + profiler suite ===")
        c_sources = [
            TESTS_DIR / "test_runtime_pinch.c",
            TESTS_DIR / "geometry.c",
            TESTS_DIR / "test_capture.c",
            TESTS_DIR / "test_held_gui.c",
            CORE_SRC / "pinch.c",
            CORE_SRC / "pinch_capture.c",
            CORE_SRC / "pinch_passive.c",
            CORE_SRC / "image_lease.c",
            CORE_SRC / "image_reference_read.c",
            CORE_SRC / "pb_held_gui.c",
            CORE_SRC / "input_tap.c",
        ]
        cpp_sources = [
            CORE_SRC / "input_hook.cpp",
            CORE_SRC / "pinch_dispatch.cpp",
            CORE_SRC / "pinch_timer.cpp",
            TESTS_DIR / "throw_native.cpp",
            TESTS_DIR / "dispatch_profile.cpp",
        ]
        inc_flags = [f"-I{TESTS_DIR}", f"-I{CORE_SRC}"]
        objs: list[str] = []
        for idx, src in enumerate(c_sources):
            obj = tmp / f"pinch_{idx}.o"
            _run([cc, "-std=c11", *warn_flags, "-funwind-tables", *san_flags, *inc_flags, "-c", str(src), "-o", str(obj)])
            objs.append(str(obj))
        pinch_bin = tmp / "test_runtime_pinch"
        _run(
            [
                cxx,
                "-std=c++17",
                *warn_flags,
                "-funwind-tables",
                *san_flags,
                *inc_flags,
                "-Drt_input_hook=pb_test_input_hook",
                *(str(s) for s in cpp_sources),
                *objs,
                "-o",
                str(pinch_bin),
            ]
        )
        out_pinch = _run([str(pinch_bin)], env=run_env)
        print(out_pinch, end="")
        assert "runtime_pinch cases=89 checks=127 error=0" in out_pinch
        assert "PB geometry checks=32" in out_pinch
        assert "PB dispatch profile cases=2 normal_only=1 slot=15" in out_pinch

        # Suite 3: Menu semantic regions (top-tab midpoint split & 48px page dots/arrows)
        print("\n=== [3/4] Building & running ASan/UBSan ViewMenu (0xAA) semantic hit-region suite ===")
        menu_bin = tmp / "test_menu_regions"
        _run(
            [
                cc,
                "-std=c11",
                *warn_flags,
                *san_flags,
                f"-I{CORE_SRC}",
                str(TESTS_DIR / "test_menu_regions.c"),
                str(CORE_SRC / "menu_regions.c"),
                str(CORE_SRC / "surface_actions.c"),
                "-o",
                str(menu_bin),
            ]
        )
        out_menu = _run([str(menu_bin)], env=run_env)
        print(out_menu, end="")
        assert "passed" in out_menu

        # Suite 4: 128B A7TBM001 boot_manifest + STREAM_FILE88B {5, 15} JPEG/RAW lease
        print("\n=== [4/4] Building & running ASan/UBSan A7TBM001 manifest & {5, 15} JPEG/RAW lease suite ===")
        lease_bin = tmp / "test_boot_manifest_and_lease"
        _run(
            [
                cc,
                "-std=c11",
                *warn_flags,
                *san_flags,
                f"-I{CORE_SRC}",
                str(TESTS_DIR / "test_boot_manifest_and_lease.c"),
                str(CORE_SRC / "boot_manifest.c"),
                str(CORE_SRC / "image_lease.c"),
                str(CORE_SRC / "image_reference_read.c"),
                "-o",
                str(lease_bin),
            ]
        )
        out_lease = _run([str(lease_bin)], env=run_env)
        print(out_lease, end="")
        assert "passed" in out_lease

    print("\nALL OFFLINE HOST TESTS PASSED (0 Sony firmware blobs required).")


def main() -> int:
    run_python_tests()
    run_host_c_suites()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
