#!/usr/bin/env python3
"""Build the macOS arm64 Release standalone and refresh binaries/macos-arm64."""

from __future__ import annotations

import argparse
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def run(command: list[str], *, cwd: Path, dry_run: bool) -> None:
    print("+ " + " ".join(command))
    if dry_run:
        return

    subprocess.run(command, cwd=cwd, check=True)


def is_apple_silicon_hardware() -> bool:
    try:
        result = subprocess.run(
            ["sysctl", "-n", "hw.optional.arm64"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return False

    return result.stdout.strip() == "1"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build the supported macOS arm64 Release standalone and sync it to binaries/macos-arm64.",
    )
    parser.add_argument(
        "--build-dir",
        default="build-release-arm",
        help="CMake build directory relative to the repository root (default: build-release-arm).",
    )
    parser.add_argument("--clean", action="store_true", help="Remove the build directory before configuring.")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running them.")
    args = parser.parse_args()

    repo_dir = Path(__file__).resolve().parents[1]
    build_dir = repo_dir / args.build_dir
    binaries_dir = repo_dir / "binaries" / "macos-arm64"
    app_path = binaries_dir / "Neural Amp Modeler.app"
    zip_path = binaries_dir / "NeuralAmpModeler-macOS-arm64.zip"

    if platform.system() != "Darwin":
        print("This script is only supported on macOS.", file=sys.stderr)
        return 2

    if not is_apple_silicon_hardware():
        print("This script must be run on Apple Silicon hardware.", file=sys.stderr)
        return 2

    if args.clean and build_dir.exists():
        print(f"+ rm -rf {build_dir}")
        if not args.dry_run:
            shutil.rmtree(build_dir)

    run(
        [
            "cmake",
            "-S",
            str(repo_dir),
            "-B",
            str(build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_OSX_ARCHITECTURES=arm64",
        ],
        cwd=repo_dir,
        dry_run=args.dry_run,
    )

    run(
        [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            "Release",
            "--target",
            "NEURAL_AMP_MODELER_Standalone",
        ],
        cwd=repo_dir,
        dry_run=args.dry_run,
    )

    if args.dry_run:
        print(f"Would verify {app_path}")
        print(f"Would verify {zip_path}")
        return 0

    missing = [path for path in (app_path, zip_path) if not path.exists()]
    if missing:
        print("Build completed, but expected binaries were not refreshed:", file=sys.stderr)
        for path in missing:
            print(f"  missing: {path}", file=sys.stderr)
        return 1

    print(f"Refreshed {app_path}")
    print(f"Refreshed {zip_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
