QT += core testlib xml
CONFIG += testcase console c++17
TEMPLATE = app
TARGET = tst_xmlmachine
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
SOURCES += tst_xmlmachine.cpp ../../src/xmlmachine.cpp
INCLUDEPATH += ../../src
