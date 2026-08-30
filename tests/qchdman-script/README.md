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

## Canonical Qt 5 reference

`./run-reference.sh` builds the harness with exactly Qt 5.15.19 and compares
its normalized fixture observations with the checked-in reference. It never
changes that reference. Maintainers regenerate it explicitly with
`./run-reference.sh --update-reference` and review the resulting Git diff.

## Three-engine differential run

Build the pinned JSC and QuickJS variants into separate prefixes with
`scripts/bootstrap-qtscript.sh` and `scripts/bootstrap-qtscript-quickjs.sh`.
Then compare both Qt 6 engines against the locked Qt 5 contract:

```sh
QTSCRIPT_JSC_PREFIX=/path/to/jsc \
QTSCRIPT_QUICKJS_PREFIX=/path/to/quickjs \
./run-differential.sh
```

The runner never updates reference data. Both engines must pass the explicit
QtTest assertions before their normalized result can be compared.
