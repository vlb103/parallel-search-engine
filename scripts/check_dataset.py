#!/usr/bin/env python3
import os
import sys

MIN_FILES = 10_000
MIN_BYTES = 2 * 1024**3


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "data"
    files = 0
    size = 0
    for directory, _, names in os.walk(root):
        for name in names:
            files += 1
            size += os.path.getsize(os.path.join(directory, name))
    print(f"files: {files}, size: {size / 1024**3:.2f} GB")
    if files < MIN_FILES or size < MIN_BYTES:
        sys.exit(f"dataset is too small: need {MIN_FILES} files and 2 GB")


if __name__ == "__main__":
    main()
