#!/usr/bin/env python3
import argparse
import json
import pathlib
import sys


def load(path):
    with path.open(encoding="utf-8") as stream:
        data = json.load(stream)
    if data.get("format") != 1 or not isinstance(data.get("fixtures"), list):
        raise ValueError(f"{path}: unsupported qchdman result format")
    return data


def main():
    parser = argparse.ArgumentParser(description="Compare normalized qchdman script observations")
    parser.add_argument("reference", type=pathlib.Path)
    parser.add_argument("actual", type=pathlib.Path)
    args = parser.parse_args()
    reference = load(args.reference)
    actual = load(args.actual)
    if reference == actual:
        return 0
    reference_text = json.dumps(reference, indent=2, sort_keys=True).splitlines()
    actual_text = json.dumps(actual, indent=2, sort_keys=True).splitlines()
    import difflib
    sys.stderr.write("\n".join(difflib.unified_diff(
        reference_text, actual_text,
        fromfile=str(args.reference), tofile=str(args.actual), lineterm="")) + "\n")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
