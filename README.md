# QMC2 Qt 6 fork

The `qt6` branch ports qmc2 and qchdman to Qt 6.8 and newer while retaining
their existing configuration and qchdman script interfaces. Qt 5 and
qmc2-arcade are not supported by this branch.

- Ports the XML, WebEngine, networking, multimedia, input, and other changed or
  removed Qt APIs.
- Replaces bundled QFtp with libcurl and enables libarchive support.
- Updates bundled zlib, minizip-ng, and LZMA SDK sources.
- Replaces obsolete ProjectMESS web defaults with Arcade Database while
  preserving custom lookup URLs.
- Builds qmc2 and qchdman in CI on Linux, macOS Intel and Apple Silicon, and
  Windows MSVC 2022, with Qt 6.8 LTS and latest-Qt coverage where applicable.

qchdman retains QtScript and its embedded debugger through the maintained
[qtscript-qt6 compatibility port](https://github.com/ostrich/qtscript-qt6).

## Original README

```text
What is the M.A.M.E. Catalog / Launcher II?
-------------------------------------------

M.A.M.E. Catalog / Launcher II - also referred to as QMC2 - is the successor of
one of the first UNIX M.A.M.E. GUI front ends available on this planet called
QMamecat (derived from MAMECAT, which was text-only). QMamecat was based upon
Qt 2; its development was frozen in 2003.

By the beginning of March 2006, we started to build QMC2 from scratch as a Qt 4
project. Parts of the design and code were inspired by its predecessor, but it's
not just a remake. We tried to make the new design as flexible as possible to
minimize dependencies from front end and CLI related MAME changes, which was a
major deficiency of QMamecat. QMC2 now uses a template based emulator config
scheme, which can easily be enhanced with additional command line options
(defined in an XML template file).

As a result of this flexible design and countless hours of work, QMC2 today is
not only a stable, feature-rich and fast multi-platform GUI front-end for
M.A.M.E. but also a fully-featured ROM manager for this emulator (and its
derivatives, older versions of MAME or even foreign emulators when they use the
same/similar XML data) through the built-in ROMAlyzer.

QMC2 and qchdman require Qt 6.8 or newer. Qt 5 is no longer supported.
qmc2-arcade has not yet been ported to Qt 6 and is therefore excluded from
this release series.

The qmc2 GUI also requires libcurl for FTP downloads. Optional libarchive
support is enabled with `LIBARCHIVE=1`. On Windows, the qmake project expects
the x64-windows libcurl and libarchive packages under VCPKG_ROOT.
Bundled zlib and minizip remain the default; distro builds can select their
system installations with `SYSTEM_ZLIB=1 SYSTEM_MINIZIP=1`.

Building and installing QMC2 from source
----------------------------------------

It's as simple as this:

$ make [-j <number of CPUs>] 
...

Followed by

$ sudo make install
...

This builds the main GUI - actually all you'd need. qchdman works stand-alone,
but needs the QtScript compatibility libraries described in
docs/qt6-port.md. Build and install it with:

$ make [-j <number of CPUs>] qchdman
$ sudo make qchdman-install

The qmc2-arcade targets are intentionally unsupported with Qt 6 in this
release series.

Run 

$ make help

to see the full list of build targets and

$ make config

to get a help on configuration variables that you'd want to change. However,
most of the time the defaults are okay.

License
-------

Copyright (C) 2006 - 2022 rene.reucher@batcom-it.net

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

Author
------

René Reucher (rene.reucher@batcom-it.net)
```
