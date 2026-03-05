#!/usr/bin/env python3
"""
Check version consistency across all OHUI source files.

Usage:
    python scripts/check_versions.py           # Check current versions
    python scripts/check_versions.py 1.0.0     # Verify specific version
"""

import re
import sys
from pathlib import Path


def find_project_root():
    path = Path(__file__).resolve().parent
    for _ in range(5):
        if (path / "CLAUDE.md").exists():
            return path
        path = path.parent
    print("ERROR: Cannot find project root")
    sys.exit(1)


def extract_version(filepath, patterns):
    """Extract version from file using regex patterns."""
    content = filepath.read_text(encoding="utf-8")
    versions = {}
    for name, pattern in patterns.items():
        match = re.search(pattern, content)
        if match:
            versions[name] = match.group(1)
    return versions


def main():
    root = find_project_root()
    expected = sys.argv[1] if len(sys.argv) > 1 else None

    files = {
        "include/Version.h": {
            "MAJOR": r"MAJOR\s*=\s*(\d+)",
            "MINOR": r"MINOR\s*=\s*(\d+)",
            "PATCH": r"PATCH\s*=\s*(\d+)",
        },
        "CMakeLists.txt": {
            "MAJOR": r"VERSION_MAJOR\s+(\d+)",
            "MINOR": r"VERSION_MINOR\s+(\d+)",
            "PATCH": r"VERSION_PATCH\s+(\d+)",
        },
        "vcpkg.json": {
            "version": r'"version-string"\s*:\s*"(\d+\.\d+\.\d+)"',
        },
        "version.rc": {
            "FileVersion": r'"FileVersion",\s*"(\d+\.\d+\.\d+)',
        },
    }

    print("OHUI Version Check")
    print("=" * 40)

    all_versions = []
    failed = False

    for filepath, patterns in files.items():
        full_path = root / filepath
        if not full_path.exists():
            print(f"  {filepath}: NOT FOUND")
            failed = True
            continue

        versions = extract_version(full_path, patterns)

        if "MAJOR" in versions:
            v = f"{versions['MAJOR']}.{versions['MINOR']}.{versions['PATCH']}"
        elif "version" in versions:
            v = versions["version"]
        elif "FileVersion" in versions:
            v = versions["FileVersion"]
        else:
            print(f"  {filepath}: PARSE ERROR")
            failed = True
            continue

        all_versions.append(v)
        status = ""
        if expected and v != expected:
            status = f" (expected {expected})"
            failed = True
        print(f"  {filepath}: {v}{status}")

    print("=" * 40)

    unique = set(all_versions)
    if len(unique) == 1 and not failed:
        print(f"All files consistent: {all_versions[0]}")
        return 0
    else:
        print("VERSION MISMATCH - update all files before release")
        return 1


if __name__ == "__main__":
    sys.exit(main())
