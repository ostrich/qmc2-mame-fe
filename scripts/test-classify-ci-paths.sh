#!/bin/sh

set -eu

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
classifier="$script_dir/classify-ci-paths.sh"

profile() {
    platforms=$1
    core=$2
    qchdman=$3
    xml=$4
    qtscript=$5
    ftp=$6
    archive=$7
    differential=$8
    case "$platforms" in
        all) linux=true; macos=true; windows=true ;;
        windows) linux=false; macos=false; windows=true ;;
        none) linux=false; macos=false; windows=false ;;
    esac
    printf 'linux=%s\nmacos=%s\nwindows=%s\ncore=%s\nqchdman=%s\nxml=%s\nqtscript=%s\nftp=%s\narchive=%s\ndifferential=%s' \
        "$linux" "$macos" "$windows" "$core" "$qchdman" "$xml" "$qtscript" "$ftp" "$archive" "$differential"
}

assert_profile() {
    description=$1
    expected=$2
    shift 2
    actual=$($classifier "$@")
    if [ "$actual" != "$expected" ]; then
        printf 'CI classification failed: %s\nexpected:\n%s\nactual:\n%s\n' \
            "$description" "$expected" "$actual" >&2
        exit 1
    fi
}

none=$(profile none false false false false false false false)
core=$(profile all true false false false false false false)
qchdman=$(profile all false true false false false false true)
xml=$(profile all false false true false false false false)
qtscript=$(profile all false false false true false false false)
ftp=$(profile all false false false false true false false)
archive=$(profile all false false false false false true false)
quickjs=$(profile none false false false false false false true)
windows_tools=$(profile windows true true false true false false false)
full=$(profile all true true true true true true true)

assert_profile 'documentation only' "$none" docs/qt6-port.md README.md
assert_profile 'main application source' "$core" src/qmc2main.cpp
assert_profile 'shared settings affect both applications' \
    "$(profile all true true false false false false true)" src/settings.cpp
assert_profile 'qchdman source' "$qchdman" src/tools/qchdman/scriptengine.cpp
assert_profile 'qchdman harness' "$qchdman" tests/qchdman-script/harness/tst_qchdman_script.cpp
assert_profile 'XML fixture' "$xml" tests/xmlmachine/tst_xmlmachine.cpp
assert_profile 'QtScript fixture' "$qtscript" tests/qtscript/tst_qtscript.cpp
assert_profile 'FTP implementation' \
    "$(profile all true false false false true false false)" src/ftpreply.cpp
assert_profile 'archive implementation' \
    "$(profile all true false false false false true false)" src/archivefile.cpp src/sevenzipfile.cpp
assert_profile 'QuickJS dependency only' "$quickjs" scripts/qtscript-quickjs-patches/0001-enable-debugger-workflow.patch
assert_profile 'Windows build helper' "$windows_tools" scripts/install-jom.ps1
assert_profile 'workflow changes' "$full" .github/workflows/ci-linux.yml
assert_profile 'unknown paths default to full coverage' "$full" tools/new-checker.cpp
assert_profile 'profiles accumulate' \
    "$(profile all true true false true false false true)" \
    src/qmc2main.cpp tests/qtscript/tst_qtscript.cpp src/tools/qchdman/scriptengine.cpp

printf '%s\n' 'CI path classification tests passed.'
