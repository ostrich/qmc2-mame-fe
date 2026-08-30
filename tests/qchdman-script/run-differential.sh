#!/usr/bin/env bash
set -euo pipefail

readonly test_root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
readonly reference=$test_root/reference/qt5-5.15.19.json
qmake_bin=${QCHDMAN_QT6_QMAKE:-qmake6}
jsc_prefix=${QTSCRIPT_JSC_PREFIX:-}
quickjs_prefix=${QTSCRIPT_QUICKJS_PREFIX:-}
[[ -n $jsc_prefix && -d $jsc_prefix ]] || { echo "set QTSCRIPT_JSC_PREFIX" >&2; exit 2; }
[[ -n $quickjs_prefix && -d $quickjs_prefix ]] || { echo "set QTSCRIPT_QUICKJS_PREFIX" >&2; exit 2; }

work_root=$(mktemp -d "${TMPDIR:-/tmp}/qchdman-differential.XXXXXX")
trap 'rm -rf "$work_root"' EXIT

run_engine() {
    local name=$1 prefix=$2 build=$work_root/$1 actual=$work_root/$1.json
    mkdir -p "$build"
    (
        cd "$build"
        export QMAKEPATH="$prefix/lib/qt6${QMAKEPATH:+:$QMAKEPATH}"
        export QTSCRIPT_PREFIX="$prefix"
        "$qmake_bin" "$test_root/qchdman-script.pro" CONFIG+=release
        make -j"${QCHDMAN_TEST_JOBS:-2}"
    )
    QT_QPA_PLATFORM=offscreen QCHDMAN_TEST_RESULTS="$actual" \
        QCHDMAN_FAKE_CHDMAN="$build/fake-chdman/fake-chdman" \
        "$build/harness/tst_qchdman_script"
    "$test_root/tools/compare_results.py" "$reference" "$actual"
    echo "$name matches the Qt 5.15.19 qchdman scripting contract"
}

run_engine jsc "$jsc_prefix"
run_engine quickjs "$quickjs_prefix"
