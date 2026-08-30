#!/bin/sh

set -eu

linux=false
macos=false
windows=false
core=false
qchdman=false
xml=false
qtscript=false
ftp=false
archive=false
differential=false

enable_platforms() {
    linux=true
    macos=true
    windows=true
}

enable_core() {
    enable_platforms
    core=true
}

enable_qchdman() {
    enable_platforms
    qchdman=true
    differential=true
}

enable_xml() {
    enable_platforms
    xml=true
}

enable_qtscript() {
    enable_platforms
    qtscript=true
}

enable_ftp() {
    enable_platforms
    ftp=true
}

enable_archive() {
    enable_platforms
    archive=true
}

enable_all() {
    enable_platforms
    core=true
    qchdman=true
    xml=true
    qtscript=true
    ftp=true
    archive=true
    differential=true
}

classify_path() {
    path=$1
    case "$path" in
        README.md|LICENSE|docs/*|pkg-specs/*|local/*|.gitignore)
            ;;
        .github/workflows/*|scripts/classify-ci-paths.sh|scripts/test-classify-ci-paths.sh)
            enable_all
            ;;
        scripts/install-jom.ps1)
            windows=true
            core=true
            qchdman=true
            qtscript=true
            ;;
        scripts/bootstrap-qtscript-quickjs*|scripts/qtscript-quickjs-patches/*)
            differential=true
            ;;
        scripts/bootstrap-qtscript*|scripts/qtscript-patches/*|scripts/patch-qtscript.cmake)
            enable_qtscript
            enable_qchdman
            ;;
        tests/qchdman-script/*)
            enable_qchdman
            ;;
        tests/qtscript/*)
            enable_qtscript
            ;;
        tests/xmlmachine/*)
            enable_xml
            ;;
        tests/ftp/*)
            enable_ftp
            ;;
        tests/archivefile/*|tests/bigbytearray/*|tests/sevenzip/*)
            enable_archive
            ;;
        src/tools/qchdman/*)
            enable_qchdman
            ;;
        src/settings.cpp|src/settings.h)
            enable_core
            enable_qchdman
            ;;
        src/xmlmachine.cpp|src/xmlmachine.h)
            enable_core
            enable_xml
            ;;
        src/ftpreply.cpp|src/ftpreply.h)
            enable_core
            enable_ftp
            ;;
        src/archivefile.cpp|src/archivefile.h|src/bigbytearray.cpp|src/bigbytearray.h|src/sevenzipfile.cpp|src/sevenzipfile.h|src/lzma/*|src/minizip/*|src/zlib/*)
            enable_core
            enable_archive
            ;;
        Makefile|qmc2.pro|qmc2.qrc|arch/*|data/*|inst/*|src/*|ui/*|scripts/sdl-version.sh)
            enable_core
            ;;
        *)
            # New or unfamiliar areas receive complete coverage until their
            # ownership is made explicit here.
            enable_all
            ;;
    esac
}

if [ "$#" -gt 0 ]; then
    for path do
        classify_path "$path"
    done
else
    while IFS= read -r path; do
        [ -n "$path" ] && classify_path "$path"
    done
fi

printf 'linux=%s\n' "$linux"
printf 'macos=%s\n' "$macos"
printf 'windows=%s\n' "$windows"
printf 'core=%s\n' "$core"
printf 'qchdman=%s\n' "$qchdman"
printf 'xml=%s\n' "$xml"
printf 'qtscript=%s\n' "$qtscript"
printf 'ftp=%s\n' "$ftp"
printf 'archive=%s\n' "$archive"
printf 'differential=%s\n' "$differential"
