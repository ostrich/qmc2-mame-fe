#!/usr/bin/env bash
set -euo pipefail

readonly PORT_REPO=https://github.com/JulienMaille/qtscript-qt6.git
readonly PORT_REV=09a5abc7b5cc41c8d99b34f0a66fa44f61d3a98e
readonly QUICKJS_REPO=https://github.com/quickjs-ng/quickjs.git
readonly QUICKJS_REV=954dc53628e36891f93c359aa60895c2ae3dac6b
readonly REPOSITORY_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
qt_root=${QT_ROOT_DIR:-}
prefix=${QTSCRIPT_PREFIX:-}
work_root=${QTSCRIPT_QUICKJS_WORK_ROOT:-}
parallel=${QTSCRIPT_JOBS:-4}

while (($#)); do
    case "$1" in
        --qt-root) qt_root=$2; shift 2 ;;
        --prefix) prefix=$2; shift 2 ;;
        --work-root) work_root=$2; shift 2 ;;
        --parallel) parallel=$2; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
[[ -n $qt_root && -d $qt_root ]] || { echo "pass --qt-root PATH" >&2; exit 2; }
[[ -n $prefix ]] || prefix=$PWD/.deps/qtscript-quickjs
[[ -n $work_root ]] || work_root=$PWD/.deps/qtscript-quickjs-work
[[ $work_root != / ]] || { echo "refusing root as --work-root" >&2; exit 2; }

port_dir=$work_root/port
quickjs_dir=$port_dir/third_party/quickjs-ng
source_dir=$work_root/src
build_dir=$work_root/build
quickjs_work=$work_root/quickjs-build

if [[ ! -d $port_dir/.git ]]; then git clone "$PORT_REPO" "$port_dir"; fi
git -C "$port_dir" remote set-url origin "$PORT_REPO"
git -C "$port_dir" fetch --quiet origin "$PORT_REV"
git -C "$port_dir" checkout --quiet --detach "$PORT_REV"
if [[ ! -d $quickjs_dir/.git ]]; then git clone "$QUICKJS_REPO" "$quickjs_dir"; fi
git -C "$quickjs_dir" remote set-url origin "$QUICKJS_REPO"
git -C "$quickjs_dir" fetch --quiet origin "$QUICKJS_REV"
git -C "$quickjs_dir" checkout --quiet --detach "$QUICKJS_REV"

# The pinned port initializes its default job count with Linux's nproc before
# parsing --parallel. Supply the equivalent shell function on every Unix host.
nproc() { printf '%s\n' "$parallel"; }
export -f nproc
bash "$port_dir/scripts/build-quickjs-ng.sh" --configuration Release \
    --work-root "$quickjs_work" --quickjs-source "$quickjs_dir" --parallel "$parallel"
quickjs_library=$quickjs_work/Release/build/libqjs.a

rm -rf "$source_dir" "$build_dir"
bash "$port_dir/scripts/apply-patches.sh" "$source_dir"
for compatibility_patch in "$REPOSITORY_ROOT/scripts/qtscript-quickjs-patches"/*.patch; do
    git -C "$source_dir" apply "$compatibility_patch"
done

cmake -S "$source_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_PREFIX_PATH="$qt_root" \
    -DQTSCRIPT_QUICKJS_INCLUDE_DIR="$quickjs_dir" \
    -DQTSCRIPT_QUICKJS_LIBRARY="$quickjs_library" \
    -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DWARNINGS_ARE_ERRORS=OFF \
    -DQT_REPO_NOT_WARNINGS_CLEAN=ON
cmake --build "$build_dir" --parallel "$parallel"
cmake --install "$build_dir"
module_root="$prefix"
if [[ -f "$prefix/lib/qt6/mkspecs/modules/qt_lib_script.pri" ]]; then
    module_root="$prefix/lib/qt6"
fi
test -f "$module_root/mkspecs/modules/qt_lib_script.pri"
test -f "$module_root/mkspecs/modules/qt_lib_scripttools.pri"
echo "QtScript QuickJS $PORT_REV with QuickJS-NG $QUICKJS_REV installed in $prefix"
