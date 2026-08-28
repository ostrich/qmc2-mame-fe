QT += core testlib
CONFIG += testcase console c++17
macx: CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_archivefile
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
DEFINES += QMC2_ARCHIVE_MAX_ENTRY_SIZE=128 QMC2_BBA_CHUNK_SIZE=32 QMC2_QBYTEARRAY_LIMIT=256
!win32 {
	CONFIG += link_pkgconfig
	PKGCONFIG += libarchive zlib
} else {
	VCPKG_PREFIX = $$(VCPKG_INSTALLATION_ROOT)
	isEmpty(VCPKG_PREFIX): VCPKG_PREFIX = $$(VCPKG_ROOT)
	ARCHIVE_ROOT = $$VCPKG_PREFIX/installed/x64-windows
	INCLUDEPATH += $$ARCHIVE_ROOT/include
	LIBS += /LIBPATH:$$ARCHIVE_ROOT/lib archive.lib zlib.lib
}
INCLUDEPATH += ../../src
SOURCES += tst_archivefile.cpp ../../src/archivefile.cpp ../../src/bigbytearray.cpp
HEADERS += ../../src/archivefile.h ../../src/bigbytearray.h
