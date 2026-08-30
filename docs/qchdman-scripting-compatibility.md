# qchdman scripting compatibility

qchdman treats the original Qt 5.15.19 QtScript behavior as its scripting
contract. The integration suite builds the real qchdman widgets and
`ScriptEngine`, drives them with an offscreen GUI, and compares normalized
observations from two Qt 6 engines with the checked-in Qt 5 reference.

The immutable engine inputs are:

- Qt 5 reference: qmc2 `1fb6d2b7429f6b1b709ed836f79ec2a9d11493d8`
  with Qt 5.15.19;
- Qt 6 JSC port: `1122594ab02aeb07c7a862738ef36486bab1ed7a` plus
  the compatibility patches in `scripts/qtscript-patches`;
- Qt 6 QuickJS port: `09a5abc7b5cc41c8d99b34f0a66fa44f61d3a98e`
  with QuickJS-NG `954dc53628e36891f93c359aa60895c2ae3dac6b` plus
  `scripts/qtscript-quickjs-patches`.

Build each Qt 6 engine into an isolated prefix; never install either into the
host Qt tree. Run `tests/qchdman-script/run-differential.sh` with
`QTSCRIPT_JSC_PREFIX` and `QTSCRIPT_QUICKJS_PREFIX` set. The runner first
requires all explicit QtTest assertions to pass and then compares each result
with `reference/qt5-5.15.19.json`.

Reference regeneration is a maintenance operation:

```sh
QCHDMAN_QT5_QMAKE=/path/to/qt-5.15.19/bin/qmake \
tests/qchdman-script/run-reference.sh --update-reference
```

CI never regenerates it. Any allowed difference must identify one fixture,
one exact language or diagnostic value, both expected values, and a rationale.
The comparator refuses qchdman project, command, signal, file, cleanup,
interruption, and debugger fields.

QuickJS-NG is qchdman's default backend; JSC is retained as the explicit
`QTSCRIPT_BACKEND=jsc` compatibility option. Pull requests run both Qt 6
engines on Linux Qt 6.8. Scheduled runs add Intel
and Apple Silicon macOS, Windows, ASan/UBSan, full debugger automation, and
small real-chdman create/verify/copy/extract/metadata workflows. Release
validation additionally requires the existing current-Qt platform builds.
