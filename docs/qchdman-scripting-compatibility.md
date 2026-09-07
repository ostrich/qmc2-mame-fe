# qchdman scripting compatibility

qchdman treats the original Qt 5.15.19 QtScript behavior as its scripting
contract. The integration suite builds the real qchdman widgets and
`ScriptEngine`, drives them with an offscreen GUI, and compares normalized
observations from two Qt 6 engines with the checked-in Qt 5 reference.

The immutable engine inputs are:

- Qt 5 reference: qmc2 `1fb6d2b7429f6b1b709ed836f79ec2a9d11493d8`
  with Qt 5.15.19;
- Qt 6 JSC port: `3228aeb249f372c68882d1a658a347b93bda9f21` plus
  the compatibility patches in `scripts/qtscript-patches`;
- Qt 6 QuickJS port: `3a7296a7a04b8c13e90ba89e656a077650918afb`
  with QuickJS-NG `954dc53628e36891f93c359aa60895c2ae3dac6b` plus
  `scripts/qtscript-quickjs-patches`.

Both Qt 6 ports are pinned from JulienMaille/qtscript-qt6. Upstream now
supplies the Clang and macOS AGL fixes. Local patches still provide JSC
debugger action/resource fixes and the QuickJS debugger and signal-handler
recovery behavior exercised by qchdman. The Qt 5 reference is unchanged.

Build each Qt 6 engine into an isolated prefix; never install either into the
host Qt tree. Use a fresh `--work-root` when upgrading or restarting a
QuickJS bootstrap: upstream patches the QuickJS checkout in place, and its
overlapping patches can prevent reusing an already patched checkout.
Run `tests/qchdman-script/run-differential.sh` with
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
