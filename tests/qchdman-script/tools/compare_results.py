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


def fixture_map(data):
    return {fixture["id"]: fixture for fixture in data["fixtures"]}


def apply_allowlist(reference, actual, path, engine):
    if not path:
        return
    with path.open(encoding="utf-8") as stream:
        entries = json.load(stream).get("differences", [])
    reference_fixtures = fixture_map(reference)
    actual_fixtures = fixture_map(actual)
    for entry in entries:
        if entry.get("engine") != engine:
            continue
        if entry.get("category") not in {"language", "diagnostic"}:
            raise ValueError(f"{path}: forbidden difference category")
        fixture_id = entry["fixture"]
        value_path = entry["path"].split(".")
        if not value_path or value_path[0] != "result":
            raise ValueError(f"{path}: differences may only name fixture language results")
        if any(token in entry["path"].lower() for token in
               ("project", "command", "signal", "debugger", "file", "cleanup", "interrupt")):
            raise ValueError(f"{path}: qchdman contract fields cannot be allowlisted")
        if not entry.get("rationale"):
            raise ValueError(f"{path}: every difference needs a rationale")
        expected = reference_fixtures[fixture_id]
        observed = actual_fixtures[fixture_id]
        for component in value_path[:-1]:
            expected = expected[component]
            observed = observed[component]
        key = value_path[-1]
        if expected[key] != entry["reference"] or observed[key] != entry["actual"]:
            raise ValueError(f"{path}: stale difference for {fixture_id}:{entry['path']}")
        observed[key] = expected[key]


def main():
    parser = argparse.ArgumentParser(description="Compare normalized qchdman script observations")
    parser.add_argument("reference", type=pathlib.Path)
    parser.add_argument("actual", type=pathlib.Path)
    parser.add_argument("--engine", default="")
    parser.add_argument("--allowlist", type=pathlib.Path)
    args = parser.parse_args()
    reference = load(args.reference)
    actual = load(args.actual)
    apply_allowlist(reference, actual, args.allowlist, args.engine)
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
