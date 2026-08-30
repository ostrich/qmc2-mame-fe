#!/usr/bin/env bash
set -euo pipefail

chdman_bin=${CHDMAN:-chdman}
command -v "$chdman_bin" >/dev/null || { echo "chdman was not found" >&2; exit 1; }
work_root=$(mktemp -d "${TMPDIR:-/tmp}/qchdman-real-smoke.XXXXXX")
trap 'rm -rf "$work_root"' EXIT

raw=$work_root/raw.bin
raw_chd=$work_root/raw.chd
copy_chd=$work_root/copy.chd
extracted=$work_root/extracted.bin
hd=$work_root/hd.bin
hd_chd=$work_root/hd.chd
metadata=$work_root/metadata.txt

dd if=/dev/zero of="$raw" bs=4096 count=256 status=none
printf 'qchdman-real-smoke' | dd of="$raw" conv=notrunc status=none
"$chdman_bin" createraw -i "$raw" -o "$raw_chd" -hs 4096 -us 1 -c none
"$chdman_bin" info -i "$raw_chd"
"$chdman_bin" verify -i "$raw_chd"
"$chdman_bin" copy -i "$raw_chd" -o "$copy_chd" -c none
"$chdman_bin" verify -i "$copy_chd"
"$chdman_bin" extractraw -i "$copy_chd" -o "$extracted"
cmp "$raw" "$extracted"

"$chdman_bin" addmeta -i "$copy_chd" -t TEST -vt qchdman-metadata
"$chdman_bin" dumpmeta -i "$copy_chd" -t TEST -o "$metadata"
grep -Fq qchdman-metadata "$metadata"
"$chdman_bin" delmeta -i "$copy_chd" -t TEST
if "$chdman_bin" dumpmeta -i "$copy_chd" -t TEST -o "$metadata.after-delete" 2>/dev/null; then
    echo "deleted metadata remained readable" >&2
    exit 1
fi

dd if=/dev/zero of="$hd" bs=512 count=2016 status=none
"$chdman_bin" createhd -i "$hd" -o "$hd_chd" -chs 2,16,63 -ss 512 -c none
"$chdman_bin" info -i "$hd_chd"
"$chdman_bin" verify -i "$hd_chd"
echo "real chdman smoke passed: $($chdman_bin -help 2>&1 | head -1)"
