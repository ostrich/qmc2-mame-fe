QT += core gui widgets testlib
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
CONFIG += console testcase
TEMPLATE = app
TARGET = tst_qchdman_script

QCHDMAN_DIR = ../../../src/tools/qchdman
QMC2_SRC = ../../../src

greaterThan(QT_MAJOR_VERSION, 5) {
    QTSCRIPT_PREFIX = $$(QTSCRIPT_PREFIX)
    !isEmpty(QTSCRIPT_PREFIX) {
        CONFIG -= link_prl
        INCLUDEPATH = $$QTSCRIPT_PREFIX/include/qt6 $$QTSCRIPT_PREFIX/include $$INCLUDEPATH
        QMAKE_LIBDIR = $$QTSCRIPT_PREFIX/lib $$QMAKE_LIBDIR
        unix:!macx: LIBS += $$QTSCRIPT_PREFIX/lib/libQt6ScriptTools.so $$QTSCRIPT_PREFIX/lib/libQt6Script.so
        macx: LIBS += -F$$QTSCRIPT_PREFIX/lib -framework QtScriptTools -framework QtScript
        win32: LIBS += $$QTSCRIPT_PREFIX/lib/Qt6ScriptTools.lib $$QTSCRIPT_PREFIX/lib/Qt6Script.lib
        DEFINES += QT_SCRIPTTOOLS_LIB QT_SCRIPT_LIB
        unix: QMAKE_RPATHDIR += $$QTSCRIPT_PREFIX/lib
    } else {
        QT += script scripttools
    }
} else {
    QT += script scripttools
}

DEFINES += QCHDMAN_VERSION=0.244 QCHDMAN_SCRIPT_TEST
INCLUDEPATH += $$QCHDMAN_DIR $$QMC2_SRC

SOURCES += tst_qchdman_script.cpp \
    $$QCHDMAN_DIR/mainwindow.cpp \
    $$QCHDMAN_DIR/projectwindow.cpp \
    $$QCHDMAN_DIR/projectwidget.cpp \
    $$QCHDMAN_DIR/preferencesdialog.cpp \
    $$QCHDMAN_DIR/aboutdialog.cpp \
    $$QCHDMAN_DIR/scriptwidget.cpp \
    $$QCHDMAN_DIR/scriptengine.cpp \
    $$QCHDMAN_DIR/ecmascripthighlighter.cpp \
    $$QCHDMAN_DIR/scripteditor.cpp \
    $$QMC2_SRC/settings.cpp

HEADERS += $$QCHDMAN_DIR/mainwindow.h \
    $$QCHDMAN_DIR/projectwindow.h \
    $$QCHDMAN_DIR/projectwidget.h \
    $$QCHDMAN_DIR/preferencesdialog.h \
    $$QCHDMAN_DIR/aboutdialog.h \
    $$QCHDMAN_DIR/scriptwidget.h \
    $$QCHDMAN_DIR/scriptengine.h \
    $$QCHDMAN_DIR/ecmascripthighlighter.h \
    $$QCHDMAN_DIR/scripteditor.h \
    $$QCHDMAN_DIR/qchdmansettings.h \
    $$QMC2_SRC/settings.h

FORMS += $$QCHDMAN_DIR/mainwindow.ui \
    $$QCHDMAN_DIR/projectwidget.ui \
    $$QCHDMAN_DIR/preferencesdialog.ui \
    $$QCHDMAN_DIR/aboutdialog.ui \
    $$QCHDMAN_DIR/scriptwidget.ui

RESOURCES += $$QCHDMAN_DIR/qchdman.qrc

DISTFILES += ../slot-manifest.json
