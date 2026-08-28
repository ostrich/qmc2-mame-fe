QT += core testlib widgets
CONFIG += testcase console c++17
TEMPLATE = app
TARGET = tst_qtscript
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete

QTSCRIPT_PREFIX = $$(QTSCRIPT_PREFIX)
!isEmpty(QTSCRIPT_PREFIX) {
    CONFIG -= link_prl
    INCLUDEPATH = $$QTSCRIPT_PREFIX/include/qt6 $$INCLUDEPATH
    unix:!macx: LIBS += $$QTSCRIPT_PREFIX/lib/libQt6ScriptTools.so $$QTSCRIPT_PREFIX/lib/libQt6Script.so
    macx: LIBS += $$QTSCRIPT_PREFIX/lib/libQt6ScriptTools.dylib $$QTSCRIPT_PREFIX/lib/libQt6Script.dylib
    win32: LIBS += $$QTSCRIPT_PREFIX/lib/Qt6ScriptTools.lib $$QTSCRIPT_PREFIX/lib/Qt6Script.lib
    DEFINES += QT_SCRIPTTOOLS_LIB QT_SCRIPT_LIB
    unix: QMAKE_RPATHDIR += $$QTSCRIPT_PREFIX/lib
} else {
    QT += script scripttools
}

SOURCES += tst_qtscript.cpp
