QT += core network testlib
CONFIG += testcase console c++17
TEMPLATE = app
TARGET = tst_ftp
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
SOURCES += tst_ftp.cpp \
    ../../src/ftpreply.cpp \
    ../../src/qftp/qftp.cpp \
    ../../src/qftp/qurlinfo.cpp
HEADERS += ../../src/ftpreply.h \
    ../../src/qftp/qftp.h \
    ../../src/qftp/qurlinfo.h
INCLUDEPATH += ../../src ../../src/qftp
