#!/usr/bin/env bash

set -euo pipefail

readonly script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export QTSCRIPT_PREFIX=${QTSCRIPT_PREFIX:-$PWD/.deps/qtscript}
export QTSCRIPT_QUICKJS_WORK_ROOT=${QTSCRIPT_QUICKJS_WORK_ROOT:-$PWD/.deps/qtscript-work}
exec "$script_dir/bootstrap-qtscript-quickjs.sh" "$@"
