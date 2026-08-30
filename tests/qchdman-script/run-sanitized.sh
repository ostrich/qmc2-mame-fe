#!/usr/bin/env bash
set -euo pipefail

readonly test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
prefix=${QTSCRIPT_PREFIX:?set QTSCRIPT_PREFIX to the isolated JSC build}
qmake_bin=${QCHDMAN_QT6_QMAKE:-qmake6}
build_root=$(mktemp -d "${TMPDIR:-/tmp}/qchdman-sanitized.XXXXXX")
trap 'rm -rf "$build_root"' EXIT
(
    cd "$build_root"
    export QMAKEPATH="$prefix/lib/qt6${QMAKEPATH:+:$QMAKEPATH}"
    "$qmake_bin" "$test_root/qchdman-script.pro" CONFIG+=debug \
        'QMAKE_CXXFLAGS+=-fsanitize=address,undefined -fno-omit-frame-pointer' \
        'QMAKE_LFLAGS+=-fsanitize=address,undefined'
    make -j"${QCHDMAN_TEST_JOBS:-2}"
)
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
QT_QPA_PLATFORM=offscreen \
QCHDMAN_FAKE_CHDMAN="$build_root/fake-chdman/fake-chdman" \
QCHDMAN_TEST_RESULTS="$build_root/results.json" \
"$build_root/harness/tst_qchdman_script"
"$test_root/tools/compare_results.py" "$test_root/reference/qt5-5.15.19.json" "$build_root/results.json"
