QT += core testlib
CONFIG += testcase console c++17 link_pkgconfig
TEMPLATE = app
TARGET = tst_archivefile
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
DEFINES += QMC2_ARCHIVE_MAX_ENTRY_SIZE=128 QMC2_BBA_CHUNK_SIZE=32 QMC2_QBYTEARRAY_LIMIT=256
PKGCONFIG += libarchive zlib
INCLUDEPATH += ../../src
SOURCES += tst_archivefile.cpp ../../src/archivefile.cpp ../../src/bigbytearray.cpp
HEADERS += ../../src/archivefile.h ../../src/bigbytearray.h
