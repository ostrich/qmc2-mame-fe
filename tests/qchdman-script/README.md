# qchdman scripting compatibility tests

This directory builds the real qchdman widgets and `ScriptEngine` into an
offscreen QtTest harness. Scripts use the production `scriptEngine` and
`qchdman` globals and return machine-readable observations by logging one line
prefixed with `QCHDMAN_TEST_RESULT `.

Build and run the initial harness with:

```sh
cd tests/qchdman-script
qmake6 qchdman-script.pro
make
QT_QPA_PLATFORM=offscreen ./harness/tst_qchdman_script
```

For an isolated Qt 6 QtScript build, set both `QMAKEPATH` and
`QTSCRIPT_PREFIX`. Qt 5 uses its normal `script` and `scripttools` modules.

`fake-chdman` is a cross-platform process double. It appends one compact JSON
invocation per line to `QCHDMAN_FAKE_RECORD`; behavior is selected with
`QCHDMAN_FAKE_MODE` (`success`, `progress`, `exit`, `crash`, or `wait`). Output,
delay, and exit status are controlled by the other `QCHDMAN_FAKE_*`
environment variables in its source.

The integration cases include recursive directory discovery and bounded
project scheduling (derived from the external `04-recursive-copy.scr` stress
case), repeated eight-project parallel batches, deterministic cancellation and
subsequent recovery, exceptions raised from project callbacks, and loading and
executing real version-1 `.scr` files with CRLF, Unicode, older application
versions, embedded delimiters, and malformed input.  These all drive the real
`ScriptWidget` and `ScriptEngine`; they do not add script-only test APIs.

## Canonical Qt 5 reference

`./run-reference.sh` builds the harness with exactly Qt 5.15.19 and compares
its normalized fixture observations with the checked-in reference. It never
changes that reference. Maintainers regenerate it explicitly with
`./run-reference.sh --update-reference` and review the resulting Git diff.

## Three-engine differential run

Build the pinned JSC and QuickJS variants into separate prefixes with
`scripts/bootstrap-qtscript-jsc.sh` and
`scripts/bootstrap-qtscript-quickjs.sh`.
Then compare both Qt 6 engines against the locked Qt 5 contract:

```sh
QTSCRIPT_JSC_PREFIX=/path/to/jsc \
QTSCRIPT_QUICKJS_PREFIX=/path/to/quickjs \
./run-differential.sh
```

The runner never updates reference data. Both engines must pass the explicit
QtTest assertions before their normalized result can be compared.
Narrow language or diagnostic differences live in `allowed-differences.json`;
the comparator rejects entries for qchdman API, project, command, signal,
file, cleanup, interruption, or debugger behavior.

`./run-real-chdman-smoke.sh` exercises small repository-independent raw and
hard-disk images through create, info, verify, copy, extraction, and metadata
operations. Set `CHDMAN` when the executable is not on `PATH`.

Linux lifetime checks use `QTSCRIPT_PREFIX=/path/to/jsc ./run-sanitized.sh`.
They run the same contract under ASan and UBSan with leak detection enabled.

Pull requests run the complete fake-chdman differential suite on Linux Qt
6.8. Scheduled runs add both macOS architectures, Windows, sanitizers, and
real-chdman smoke coverage. QuickJS-NG is the product default, while the
differential suite continues to enforce the same qchdman contract for JSC.
