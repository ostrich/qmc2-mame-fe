#!/usr/bin/env bash

set -euo pipefail

readonly PORT_REPO=https://github.com/JulienMaille/qtscript-qt6.git
readonly PORT_REV=3228aeb249f372c68882d1a658a347b93bda9f21
readonly REPOSITORY_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
qt_root="${QT_ROOT_DIR:-}"
prefix="${QTSCRIPT_PREFIX:-}"
work_root="${QTSCRIPT_JSC_WORK_ROOT:-}"
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
[[ -n "$prefix" ]] || prefix="$PWD/.deps/qtscript-jsc"
[[ -n "$work_root" ]] || work_root="$PWD/.deps/qtscript-jsc-work"
[[ "$work_root" != "/" ]] || { echo "refusing root as --work-root" >&2; exit 2; }

port_dir="$work_root/port"
source_dir="$work_root/src"
build_dir="$work_root/build"

if [[ ! -d "$port_dir/.git" ]]; then git clone "$PORT_REPO" "$port_dir"; fi
git -C "$port_dir" remote set-url origin "$PORT_REPO"
git -C "$port_dir" fetch --quiet origin "$PORT_REV"
git -C "$port_dir" checkout --quiet --detach "$PORT_REV"
rm -rf "$source_dir" "$build_dir"
platform_patches=()
[[ $(uname -s) != Darwin ]] || platform_patches=(--include-macos)
bash "$port_dir/scripts/apply-patches.sh" "$source_dir" "${platform_patches[@]}"
for compatibility_patch in "$REPOSITORY_ROOT/scripts/qtscript-patches"/*.patch; do
	git -C "$source_dir" apply "$compatibility_patch"
done

qt_cmake=""
apple_check_args=()
if [[ $(uname -s) == Darwin ]]; then
	# Match upstream's support for Command Line Tools-only macOS hosts.
	apple_check_args=(-DQT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK=ON)
	if ! command -v xcodebuild >/dev/null || ! xcodebuild -version >/dev/null 2>&1; then
		apple_check_args+=(-DQT_NO_XCODE_MIN_VERSION_CHECK=ON)
	fi
fi
for candidate in "$qt_root/bin/qt-cmake-private" "$qt_root/libexec/qt-cmake-private"; do
	[[ -x "$candidate" ]] && qt_cmake="$candidate" && break
done
if [[ -n "$qt_cmake" ]]; then
	"$qt_cmake" -S "$source_dir" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
		-DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF \
		-DWARNINGS_ARE_ERRORS=OFF -DQT_REPO_NOT_WARNINGS_CLEAN=ON "${apple_check_args[@]}"
else
	cmake -S "$source_dir" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
		-DCMAKE_PREFIX_PATH="$qt_root" \
		-DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF \
		-DWARNINGS_ARE_ERRORS=OFF -DQT_REPO_NOT_WARNINGS_CLEAN=ON "${apple_check_args[@]}"
fi
cmake --build "$build_dir" --parallel "$parallel"
cmake --install "$build_dir"

module_root="$prefix"
if [[ -f "$prefix/lib/qt6/mkspecs/modules/qt_lib_script.pri" ]]; then module_root="$prefix/lib/qt6"; fi
test -f "$module_root/mkspecs/modules/qt_lib_script.pri"
test -f "$module_root/mkspecs/modules/qt_lib_scripttools.pri"
echo "QtScript JSC $PORT_REV installed in $prefix"
echo "Add $module_root to QMAKEPATH and set QTSCRIPT_BACKEND=jsc when building qchdman"
