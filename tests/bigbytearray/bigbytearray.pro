QT += core testlib
CONFIG += testcase console c++17
macx: CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_bigbytearray
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
DEFINES += QMC2_BBA_CHUNK_SIZE=8 QMC2_QBYTEARRAY_LIMIT=64
SOURCES += tst_bigbytearray.cpp ../../src/bigbytearray.cpp
INCLUDEPATH += ../../src
!win32 {
    CONFIG += link_pkgconfig
    PKGCONFIG += zlib
} else {
    VCPKG_PREFIX = $$(VCPKG_INSTALLATION_ROOT)
    isEmpty(VCPKG_PREFIX): VCPKG_PREFIX = $$(VCPKG_ROOT)
    ZLIB_ROOT = $$VCPKG_PREFIX/installed/x64-windows
    INCLUDEPATH += $$ZLIB_ROOT/include
    LIBS += /LIBPATH:$$ZLIB_ROOT/lib zlib.lib
}
