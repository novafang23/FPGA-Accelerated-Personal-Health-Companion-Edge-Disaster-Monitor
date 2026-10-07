#!/usr/bin/env python3
"""
Workspace Cleanliness and Integrity Verification Tool
Part of Project VALOR (SIH26181)

Ensures untracked build outputs, EDA tool caches, and transient simulation waveforms
are cleanly eradicated while strictly guaranteeing that environment configurations
(.env), core source code, and project configs are permanently preserved.
"""

import sys
import os
import shutil
import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# Files that MUST exist and must NEVER be deleted
PROTECTED_FILES = [
    ".env",
    ".env.example",
    "firmware/shrikefi/sdkconfig",
    "firmware/shrikefi/wifi_credentials.h",
    "hardware/zynq/vivado_project/zynq_ppg_system.xpr",
    "shrike_fpga/shrike_fpga.xpr",
    "data/mimic/mimic_eval_feed.csv",
]

# Core source directories that must have intact source trees
PROTECTED_DIRS = [
    "firmware/core",
    "firmware/shrikefi/main",
    "hardware/common",
    "hardware/zynq",
    "hardware/shrikefi",
    "scripts",
    "docs",
]

# Prohibited file extensions (transient build/sim artifacts)
PROHIBITED_EXTENSIONS = {".exe", ".o", ".vcd", ".vvp"}

# Prohibited specific files
PROHIBITED_FILES = {
    "hardware/zynq/vivado_project/vivado.jou",
    "hardware/zynq/vivado_project/vivado.log",
}

# Prohibited directory patterns or explicit paths
PROHIBITED_DIRS = [
    "firmware/shrikefi/build",
    "firmware/shrikefi/.cache",
    "hardware/shrikefi/forgefpga_project/ffpga/build",
    "hardware/shrikefi/pcb/.history",
    "hardware/zynq/vivado_project/zynq_ppg_system.cache",
    "hardware/zynq/vivado_project/zynq_ppg_system.hw",
    "hardware/zynq/vivado_project/zynq_ppg_system.ip_user_files",
    "hardware/zynq/vivado_project/zynq_ppg_system.runs",
    "hardware/zynq/vivado_project/zynq_ppg_system.sim",
    "shrike_fpga/shrike_fpga.cache",
    "shrike_fpga/shrike_fpga.hw",
    "shrike_fpga/shrike_fpga.ip_user_files",
    "shrike_fpga/shrike_fpga.sim",
]

# Prohibited cache names
PROHIBITED_CACHE_NAMES = {"__pycache__", ".pytest_cache", ".DS_Store"}


def check_protected_files():
    """Verify all protected files exist and have non-zero size."""
    missing_or_corrupt = []
    for rel_path in PROTECTED_FILES:
        full_path = REPO_ROOT / rel_path
        if not full_path.exists():
            missing_or_corrupt.append(f"MISSING: {rel_path}")
        elif full_path.is_file() and full_path.stat().st_size == 0:
            missing_or_corrupt.append(f"ZERO-BYTE / EMPTY: {rel_path}")
    return missing_or_corrupt


def find_prohibited_items():
    """Scan the repository for prohibited build/sim files, caches, and dirs."""
    found_files = []
    found_dirs = []

    # 1. Check explicit prohibited dirs
    for rel_dir in PROHIBITED_DIRS:
        full_dir = REPO_ROOT / rel_dir
        if full_dir.exists() and full_dir.is_dir():
            found_dirs.append(full_dir)

    # 2. Check explicit prohibited files
    for rel_file in PROHIBITED_FILES:
        full_file = REPO_ROOT / rel_file
        if full_file.exists() and full_file.is_file():
            found_files.append(full_file)

    # 3. Walk filesystem looking for prohibited extensions and cache dirs
    for root, dirs, files in os.walk(REPO_ROOT):
        root_path = Path(root)

        # Do not scan inside .git, .superpowers, or .venv
        rel_root = root_path.relative_to(REPO_ROOT)
        parts = rel_root.parts
        if any(p in {".git", ".superpowers", ".venv"} for p in parts):
            continue

        # Check for prohibited directory names (like __pycache__, .pytest_cache, .DS_Store)
        for d in list(dirs):
            if d in PROHIBITED_CACHE_NAMES:
                full_d = root_path / d
                if full_d not in found_dirs:
                    found_dirs.append(full_d)

        # Check files
        for f in files:
            file_path = root_path / f
            suffix = file_path.suffix.lower()
            if suffix in PROHIBITED_EXTENSIONS or f in PROHIBITED_CACHE_NAMES:
                if file_path not in found_files:
                    found_files.append(file_path)

    return found_files, found_dirs


def main():
    parser = argparse.ArgumentParser(description="Workspace cleanliness and integrity verifier.")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--dry-run", action="store_true", help="List targets to clean without deleting.")
    group.add_argument("--clean", action="store_true", help="Safely clean prohibited items while preserving protected files.")
    parser.add_argument("--check-only", action="store_true", help="Check only, exit 1 if dirty (default).")

    args = parser.parse_args()

    # Step A: Pre-check protected files
    protected_failures = check_protected_files()
    if protected_failures:
        print("CRITICAL ERROR: Protected file check FAILED before cleanup:")
        for err in protected_failures:
            print(f"  [!] {err}")
        sys.exit(2)

    found_files, found_dirs = find_prohibited_items()

    if args.dry_run:
        print(f"--- Dry Run Analysis ---")
        print(f"Protected Files: {len(PROTECTED_FILES)} verified OK.")
        print(f"Prohibited Files to Delete: {len(found_files)}")
        for f in found_files:
            print(f"  [FILE] {f.relative_to(REPO_ROOT)}")
        print(f"Prohibited Dirs to Delete: {len(found_dirs)}")
        for d in found_dirs:
            print(f"  [DIR]  {d.relative_to(REPO_ROOT)}")
        sys.exit(0)

    if args.clean:
        print(f"Cleaning {len(found_files)} prohibited files and {len(found_dirs)} directories...")
        deleted_files = 0
        deleted_dirs = 0

        for f in found_files:
            try:
                if f.exists():
                    f.unlink()
                    deleted_files += 1
            except Exception as e:
                print(f"Error removing {f}: {e}")

        for d in found_dirs:
            try:
                if d.exists():
                    shutil.rmtree(d, ignore_errors=True)
                    deleted_dirs += 1
            except Exception as e:
                print(f"Error removing {d}: {e}")

        # Post-check protected files
        post_protected_failures = check_protected_files()
        if post_protected_failures:
            print("FATAL ERROR: Protected files were damaged during cleanup!")
            for err in post_protected_failures:
                print(f"  [!] {err}")
            sys.exit(3)

        print(f"Clean complete: {deleted_files} files removed, {deleted_dirs} dirs removed.")
        print("All protected configurations (.env, code, configs) remain 100% intact.")
        sys.exit(0)

    # Default / --check-only mode
    if found_files or found_dirs:
        print("DIRTY: Prohibited build/sim artifacts or caches detected in workspace:")
        for f in found_files:
            print(f"  [FILE] {f.relative_to(REPO_ROOT)}")
        for d in found_dirs:
            print(f"  [DIR]  {d.relative_to(REPO_ROOT)}")
        print(f"Total: {len(found_files)} prohibited files, {len(found_dirs)} prohibited directories.")
        sys.exit(1)
    else:
        print("CLEAN: Workspace is pristine. 0 prohibited artifacts found.")
        print(f"Protected Files: {len(PROTECTED_FILES)} verified OK.")
        sys.exit(0)


if __name__ == "__main__":
    main()
