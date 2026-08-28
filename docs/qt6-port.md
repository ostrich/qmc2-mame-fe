# Qt 6 port notes

## Supported configurations

qmc2 and qchdman require Qt 6.8 or newer. The release build matrix covers Qt
6.8 LTS on Linux, macOS Intel, macOS Apple Silicon, and Windows with MSVC 2022.
Linux also has a latest-Qt compile lane. Qt 5 and qmc2-arcade are outside the
support scope of this port.

Existing qmc2 configuration keys and user-data formats remain supported. The
old ProjectMESS-named configuration keys are retained deliberately so existing
profiles continue to work. Only an untouched legacy default lookup URL is
migrated to Arcade Database; custom user URLs are never replaced.

## Building qchdman

Qt 6 no longer ships QtScript. qchdman uses the Qt 6 compatibility port from
<https://github.com/JulienMaille/qtscript-qt6>, pinned by the bootstrap scripts
to commit `b38a30b0f2324d23aa172d47c174b3f770753c8c`. The port in turn pins the
KDE QtScript source and its patch revision.

Build QtScript into an isolated prefix, never into the Qt installation:

```sh
scripts/bootstrap-qtscript.sh \
  --qt-root /path/to/Qt/6.8.x/platform \
  --prefix "$PWD/.deps/qtscript"

QMAKEPATH="$PWD/.deps/qtscript:$PWD/.deps/qtscript/lib/qt6" \
QTSCRIPT_PREFIX="$PWD/.deps/qtscript" \
make qchdman
```

On Windows, use `scripts/bootstrap-qtscript.ps1` with the equivalent `-QtRoot`
and `-Prefix` arguments from an MSVC 2022 development environment. Release
packages must include the Script and ScriptTools shared libraries from the
isolated prefix alongside qchdman.

## Script compatibility and security

The `.scr` file format, the `scriptEngine` and `qchdman` globals, and the
callable slot API remain unchanged. qchdman also retains its embedded
`QScriptEngineDebugger` rather than substituting a different JavaScript engine.

qchdman scripts are trusted native-application automation, not sandboxed web
content. The API can execute shell commands and create, modify, or remove files,
and the compatibility library retains the legacy JavaScriptCore engine. Only
run scripts whose source and effects you trust.

## Intentional compatibility boundaries

- qmc2-arcade is not built by Qt 6 CI and receives no Qt 6 compatibility claim.
- Qt 5 builds are intentionally unsupported.
- Arcade Database replaces the dead ProjectMESS defaults and labels while the
  legacy internal setting names remain stable.
- The QtScript compatibility libraries are private application dependencies;
  they must not be installed into the system Qt prefix.

Any further behavior difference discovered during platform smoke testing is a
release blocker until it is fixed or documented and explicitly accepted.
