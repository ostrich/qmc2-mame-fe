QT += core testlib xml
CONFIG += testcase console c++17
TEMPLATE = app
TARGET = tst_xmlmachine
SOURCES += tst_xmlmachine.cpp ../../src/xmlmachine.cpp
INCLUDEPATH += ../../src
