VERSION = 0.244

!versionAtLeast(QT_VERSION, 6.8.0): error("qchdman requires Qt 6.8 or newer")
QT += core core5compat gui script scripttools widgets
TARGET = qchdman
TEMPLATE = app

greaterThan(DEBUG, 0) | contains(DEFINES, "QCHDMAN_DEBUG") {
    !contains(DEFINES, "QCHDMAN_DEBUG"): DEFINES += QCHDMAN_DEBUG
    !contains(CONFIG, "warn_on debug"): CONFIG += warn_on debug
} else {
    !contains(DEFINES, "QCHDMAN_RELEASE"): DEFINES += QCHDMAN_RELEASE
    !contains(CONFIG, "warn_off release"): CONFIG += warn_off release
}

!equals(GIT_REV, ) {
    DEFINES += QCHDMAN_GIT_REV=$$GIT_REV
}

macx {
    QMAKE_INFO_PLIST = Info.plist
    contains(DEFINES, QCHDMAN_MAC_UNIVERSAL): CONFIG += x86_64 arm64
}

win32 {
    RC_FILE = qchdman.rc
}

DEFINES += QCHDMAN_VERSION=$$VERSION

SOURCES += main.cpp\
    mainwindow.cpp \
    projectwindow.cpp \
    projectwidget.cpp \
    preferencesdialog.cpp \
    aboutdialog.cpp \
    scriptwidget.cpp \
    scriptengine.cpp \
    ecmascripthighlighter.cpp \
    scripteditor.cpp \
    ../../settings.cpp

HEADERS  += mainwindow.h \
    macros.h \
    projectwindow.h \
    projectwidget.h \
    preferencesdialog.h \
    aboutdialog.h \
    scriptwidget.h \
    scriptengine.h \
    ecmascripthighlighter.h \
    scripteditor.h \
    qchdmansettings.h \
    ../../settings.h

FORMS += mainwindow.ui \
    projectwidget.ui \
    preferencesdialog.ui \
    aboutdialog.ui \
    scriptwidget.ui

RESOURCES += qchdman.qrc

TRANSLATIONS += translations/qchdman_de.ts \
    translations/qchdman_es.ts \
    translations/qchdman_el.ts \
    translations/qchdman_it.ts \
    translations/qchdman_fr.ts \
    translations/qchdman_pl.ts \
    translations/qchdman_pt.ts \
    translations/qchdman_pt_BR.ts \
    translations/qchdman_ro.ts \
    translations/qchdman_sv.ts \
    translations/qchdman_us.ts
