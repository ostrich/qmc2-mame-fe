#!/usr/bin/env bash

set -euo pipefail

readonly PORT_REPO=https://github.com/ostrich/qtscript-qt6.git
readonly PORT_REV=1122594ab02aeb07c7a862738ef36486bab1ed7a
qt_root="${QT_ROOT_DIR:-}"
prefix="${QTSCRIPT_PREFIX:-}"
work_root="${QTSCRIPT_WORK_ROOT:-}"
parallel="${QTSCRIPT_JOBS:-4}"

while (($#)); do
	case "$1" in
		--qt-root) qt_root="$2"; shift 2 ;;
		--prefix) prefix="$2"; shift 2 ;;
		--work-root) work_root="$2"; shift 2 ;;
		--parallel) parallel="$2"; shift 2 ;;
		*) echo "unknown argument: $1" >&2; exit 2 ;;
	esac
done

[[ -n "$qt_root" && -d "$qt_root" ]] || { echo "pass --qt-root PATH" >&2; exit 2; }
[[ -n "$prefix" ]] || prefix="$PWD/.deps/qtscript"
[[ -n "$work_root" ]] || work_root="$PWD/.deps/qtscript-work"
[[ "$work_root" != "/" ]] || { echo "refusing root as --work-root" >&2; exit 2; }

port_dir="$work_root/port"
source_dir="$work_root/src"
build_dir="$work_root/build"

if [[ ! -d "$port_dir/.git" ]]; then
	git clone "$PORT_REPO" "$port_dir"
fi
git -C "$port_dir" remote set-url origin "$PORT_REPO"
git -C "$port_dir" fetch --quiet origin "$PORT_REV"
git -C "$port_dir" checkout --quiet --detach "$PORT_REV"
rm -rf "$source_dir" "$build_dir"
bash "$port_dir/scripts/apply-patches.sh" "$source_dir"

qt_cmake=""
for candidate in "$qt_root/bin/qt-cmake-private" "$qt_root/libexec/qt-cmake-private"; do
	[[ -x "$candidate" ]] && qt_cmake="$candidate" && break
done
if [[ -n "$qt_cmake" ]]; then
	"$qt_cmake" -S "$source_dir" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$prefix" \
		-DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF \
		-DWARNINGS_ARE_ERRORS=OFF
else
	cmake -S "$source_dir" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$prefix" \
		-DCMAKE_PREFIX_PATH="$qt_root" \
		-DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF \
		-DWARNINGS_ARE_ERRORS=OFF
fi
cmake --build "$build_dir" --parallel "$parallel"
cmake --install "$build_dir"

module_root="$prefix"
if [[ -f "$prefix/lib/qt6/mkspecs/modules/qt_lib_script.pri" ]]; then
	module_root="$prefix/lib/qt6"
fi
test -f "$module_root/mkspecs/modules/qt_lib_script.pri"
test -f "$module_root/mkspecs/modules/qt_lib_scripttools.pri"
echo "QtScript $PORT_REV installed in $prefix"
echo "Add $module_root to QMAKEPATH when building qchdman"
