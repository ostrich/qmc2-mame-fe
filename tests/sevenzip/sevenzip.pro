QT += core testlib
CONFIG += testcase console c++17
macx: CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_sevenzip
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
DEFINES += Z7_PPMD_SUPPORT Z7_ST Z7_NO_UNALIGNED_ACCESS
INCLUDEPATH += ../../src ../../src/lzma
SOURCES += tst_sevenzip.cpp \
    ../../src/sevenzipfile.cpp \
    ../../src/bigbytearray.cpp \
    ../../src/lzma/7zAlloc.c \
    ../../src/lzma/7zBuf2.c \
    ../../src/lzma/7zBuf.c \
    ../../src/lzma/7zCrc.c \
    ../../src/lzma/7zCrcOpt.c \
    ../../src/lzma/7zDec.c \
    ../../src/lzma/7zFile.c \
    ../../src/lzma/7zArcIn.c \
    ../../src/lzma/7zStream.c \
    ../../src/lzma/Alloc.c \
    ../../src/lzma/Bcj2.c \
    ../../src/lzma/Bra86.c \
    ../../src/lzma/Bra.c \
    ../../src/lzma/BraIA64.c \
    ../../src/lzma/CpuArch.c \
    ../../src/lzma/Delta.c \
    ../../src/lzma/Lzma2Dec.c \
    ../../src/lzma/LzmaDec.c \
    ../../src/lzma/Ppmd7.c \
    ../../src/lzma/Ppmd7Dec.c \
    ../../src/lzma/Sha256.c \
    ../../src/lzma/Sha256Opt.c
HEADERS += ../../src/sevenzipfile.h ../../src/bigbytearray.h
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
