#!/usr/bin/env python3
import re
import shutil
import sys
import tempfile
import urllib.request
from pathlib import Path

import pyarrow.parquet as pq

REVISION = "b04c8d1ceb2f5cd4588862100d08de323dccfbaa"
BASE_URL = (
    "https://huggingface.co/datasets/wikimedia/wikipedia/resolve/"
    f"{REVISION}/20231101.en/"
)
SHARDS = [
    "train-00016-of-00041.parquet",
    "train-00037-of-00041.parquet",
    "train-00039-of-00041.parquet",
]
FILES_PER_DIR = 1000
MAX_NAME_LENGTH = 100
RESERVED_NAMES = {"con", "prn", "aux", "nul"} | {
    f"{prefix}{n}" for prefix in ("com", "lpt") for n in range(1, 10)
}


def file_name(title, article_id, used):
    name = re.sub(r"[^\w.-]+", "_", title)[:MAX_NAME_LENGTH].strip("._")
    if not name or name.lower() in used or name.lower() in RESERVED_NAMES:
        name = f"{name}_{article_id}"
    used.add(name.lower())
    return name + ".txt"


def main():
    data_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "data")
    done_dir = data_dir / "wikipedia"
    work_dir = data_dir / "wikipedia.tmp"
    if done_dir.exists():
        print(f"{done_dir} already exists, skipping")
        return
    shutil.rmtree(work_dir, ignore_errors=True)

    used = set()
    count = 0
    with tempfile.TemporaryDirectory() as download_dir:
        for shard in SHARDS:
            path = Path(download_dir) / shard
            print(f"downloading {shard}", flush=True)
            urllib.request.urlretrieve(BASE_URL + shard, path)
            table = pq.read_table(path, columns=["id", "title", "text"])
            for article_id, title, text in zip(
                *(column.to_pylist() for column in table.columns)
            ):
                directory = work_dir / f"{count // FILES_PER_DIR:04d}"
                directory.mkdir(parents=True, exist_ok=True)
                name = file_name(title, article_id, used)
                (directory / name).write_text(f"{title}\n\n{text}\n", encoding="utf-8")
                count += 1
            path.unlink()
            print(f"{shard}: {count} articles", flush=True)
    work_dir.rename(done_dir)


if __name__ == "__main__":
    main()
