#!/usr/bin/env bash
set -euo pipefail

readonly test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
readonly repository_root=$(cd "$test_root/../.." && pwd)
readonly expected_qt=5.15.19

update=false
if [[ ${1:-} == --update-reference ]]; then
    update=true
    shift
fi
[[ $# -eq 0 ]] || { echo "usage: $0 [--update-reference]" >&2; exit 2; }

qmake_bin=${QCHDMAN_QT5_QMAKE:-qmake-qt5}
actual_qt=$($qmake_bin -query QT_VERSION)
[[ $actual_qt == "$expected_qt" ]] || {
    echo "Qt $expected_qt is required for canonical reference generation (found $actual_qt)" >&2
    exit 1
}

build_root=$(mktemp -d "${TMPDIR:-/tmp}/qchdman-qt5-reference.XXXXXX")
trap 'rm -rf "$build_root"' EXIT
(
    cd "$build_root"
    "$qmake_bin" "$test_root/qchdman-script.pro" CONFIG+=release
    make -j"${QCHDMAN_TEST_JOBS:-2}"
)

actual="$build_root/qt5-reference.json"
QT_QPA_PLATFORM=offscreen QCHDMAN_TEST_RESULTS="$actual" \
    QCHDMAN_FAKE_CHDMAN="$build_root/fake-chdman/fake-chdman" \
    "$build_root/harness/tst_qchdman_script"

reference="$test_root/reference/qt5-5.15.19.json"
if $update; then
    cp "$actual" "$reference"
    echo "updated $reference"
else
    "$test_root/tools/compare_results.py" "$reference" "$actual"
fi
