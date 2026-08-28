QT += core network testlib
CONFIG += testcase console c++17
macx: CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_ftp
greaterThan(QMAKE_GCC_MAJOR_VERSION, 15): QMAKE_CXXFLAGS_WARN_ON += -Wno-sfinae-incomplete
SOURCES += tst_ftp.cpp ../../src/ftpreply.cpp
HEADERS += ../../src/ftpreply.h
INCLUDEPATH += ../../src
!win32 {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcurl
} else {
	VCPKG_PREFIX = $$(VCPKG_INSTALLATION_ROOT)
	isEmpty(VCPKG_PREFIX): VCPKG_PREFIX = $$(VCPKG_ROOT)
	CURL_ROOT = $$VCPKG_PREFIX/installed/x64-windows
	INCLUDEPATH += $$CURL_ROOT/include
	LIBS += /LIBPATH:$$CURL_ROOT/lib libcurl.lib
}
