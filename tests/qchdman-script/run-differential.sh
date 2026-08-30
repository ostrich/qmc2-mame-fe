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
    local module_root=$prefix
    if [[ -f $prefix/lib/qt6/mkspecs/modules/qt_lib_script.pri ]]; then
        module_root=$prefix/lib/qt6
    fi
    mkdir -p "$build"
    (
        cd "$build"
        export QMAKEPATH="$module_root${QMAKEPATH:+:$QMAKEPATH}"
        export QTSCRIPT_PREFIX="$prefix"
        qmake_args=("$test_root/qchdman-script.pro" CONFIG+=release)
        if [[ $(uname -s) == Darwin ]]; then
            qmake_args+=(CONFIG+=sdk_no_version_check
                'QMAKE_CXXFLAGS+=-Wno-error=implicit-function-declaration'
                'QMAKE_LIBS_OPENGL=-framework OpenGL')
        fi
        "$qmake_bin" "${qmake_args[@]}"
        make -j"${QCHDMAN_TEST_JOBS:-2}"
    )
    local harness_exe=$build/harness/tst_qchdman_script
    local fake_chdman=$build/fake-chdman/fake-chdman
    if [[ $(uname -s) == Darwin ]]; then
        harness_exe+=.app/Contents/MacOS/tst_qchdman_script
        fake_chdman+=.app/Contents/MacOS/fake-chdman
    fi
    local qpa_platform=offscreen
    if [[ $(uname -s) == Darwin ]]; then
        qpa_platform=cocoa
    fi
    QT_QPA_PLATFORM="$qpa_platform" QT_STYLE_OVERRIDE=Fusion QCHDMAN_TEST_RESULTS="$actual" \
        QCHDMAN_FAKE_CHDMAN="$fake_chdman" \
        "$harness_exe"
    compare_args=("$reference" "$actual")
    if [[ $name == quickjs ]]; then
        compare_args+=(--engine quickjs --allowlist "$test_root/allowed-differences.json")
    fi
    "$test_root/tools/compare_results.py" "${compare_args[@]}"
    echo "$name matches the Qt 5.15.19 qchdman scripting contract"
}

case ${QCHDMAN_ENGINES:-jsc,quickjs} in
    jsc) run_engine jsc "$jsc_prefix" ;;
    quickjs) run_engine quickjs "$quickjs_prefix" ;;
    jsc,quickjs) run_engine jsc "$jsc_prefix"; run_engine quickjs "$quickjs_prefix" ;;
    *) echo "QCHDMAN_ENGINES must be jsc, quickjs, or jsc,quickjs" >&2; exit 2 ;;
esac
