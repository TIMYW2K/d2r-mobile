"""Check Willowisp resource reads in a DM_FSLOG trace using the matching build manifest."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    assets = defaultdict(list)
    with args.manifest.open(encoding="utf-8-sig") as stream:
        for row in csv.reader(stream, delimiter="|"):
            if len(row) < 10:
                continue
            path = row[9].lower()
            if "willowisp" not in path or "/hd/" not in path or "/sfx/" in path:
                continue
            key = bytes.fromhex(row[6])
            archive = f"{key[9]:02d}-{key[10]:08d}.data"
            start = int.from_bytes(key[11:], "big")
            assets[archive].append((start, start + int(row[7]), row[9]))
    if not assets:
        parser.error("No Willowisp assets in manifest")
    reads = defaultdict(list)
    with args.trace.open(encoding="utf-8-sig", errors="replace") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if len(row) < 5:
                continue
            archive = row[1].replace("\\", "/").rsplit("/", 1)[-1]
            if archive not in assets:
                continue
            start, length = int(row[2]), int(row[3])
            for lo, hi, name in assets[archive]:
                left, right = max(start, lo), min(start + length, hi)
                if left < right:
                    reads[name].append((left, right))
    print("read_bytes\tencoded_bytes\tresource")
    for entries in assets.values():
        for lo, hi, name in entries:
            covered, end = 0, lo
            for left, right in sorted(reads[name]):
                covered += max(0, right - max(left, end))
                end = max(end, right)
            print(f"{covered}\t{hi-lo}\t{name}")


if __name__ == "__main__":
    main()
