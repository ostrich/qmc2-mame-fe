VCPKG_PREFIX = $$(VCPKG_INSTALLATION_ROOT)
isEmpty(VCPKG_PREFIX): VCPKG_PREFIX = $$(VCPKG_ROOT)
VCPKG_TEST_ROOT = $$VCPKG_PREFIX/installed/x64-windows
INCLUDEPATH += "$$VCPKG_TEST_ROOT/include"

# vcpkg's MSVC import library was renamed in zlib 1.4.
exists("$$VCPKG_TEST_ROOT/lib/libz.lib") {
    LIBS += "$$VCPKG_TEST_ROOT/lib/libz.lib"
} else: exists("$$VCPKG_TEST_ROOT/lib/z.lib") {
    LIBS += "$$VCPKG_TEST_ROOT/lib/z.lib"
} else {
    error("Cannot find libz.lib or z.lib in $$VCPKG_TEST_ROOT/lib")
}
