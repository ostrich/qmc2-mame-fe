if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "pass -DSOURCE_DIR=PATH")
endif()

function(replace_exact relative_path old_text new_text)
    set(path "${SOURCE_DIR}/${relative_path}")
    file(READ "${path}" contents)
    string(FIND "${contents}" "${old_text}" match_offset)
    if(match_offset EQUAL -1)
        message(FATAL_ERROR "expected text was not found in ${relative_path}")
    endif()
    string(REPLACE "${old_text}" "${new_text}" contents "${contents}")
    file(WRITE "${path}" "${contents}")
endfunction()

replace_exact("src/script/CMakeLists.txt"
[[    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)
]]
[[    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

# Prebuilt Qt packages can propagate -Werror through Qt::Platform. The legacy
# JavaScriptCore sources are not warning-clean with current Apple Clang.
set_property(TARGET Script PROPERTY QT_SKIP_WARNINGS_ARE_ERRORS ON)

# JavaScriptCore's Darwin clock implementation calls CFAbsoluteTime APIs
# directly, so QtCore's private transitive linkage is not sufficient.
if(APPLE)
    target_link_libraries(Script PRIVATE "-framework CoreFoundation")
endif()
]])

replace_exact("src/scripttools/CMakeLists.txt"
[[    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
        Qt::ScriptPrivate
)
]]
[[    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
        Qt::ScriptPrivate
)

set_property(TARGET ScriptTools PROPERTY QT_SKIP_WARNINGS_ARE_ERRORS ON)
]])

replace_exact("src/3rdparty/javascriptcore/JavaScriptCore/wtf/MathExtras.h"
[[#if OS(DARWIN)

// Work around a bug in the Mac OS X libc where ceil(-0.1) return +0.
inline double wtf_ceil(double x) { return copysign(ceil(x), x); }

#define ceil(x) wtf_ceil(x)

#endif

]] "")

replace_exact("src/3rdparty/javascriptcore/JavaScriptCore/runtime/Collector.cpp"
[[#elif CPU(ARM)
typedef arm_thread_state_t PlatformThreadRegisters;
#else
]]
[[#elif CPU(ARM)
typedef arm_thread_state_t PlatformThreadRegisters;
#elif CPU(AARCH64)
typedef arm_thread_state64_t PlatformThreadRegisters;
#else
]])

replace_exact("src/3rdparty/javascriptcore/JavaScriptCore/runtime/Collector.cpp"
[[#elif CPU(ARM)
    unsigned user_count = ARM_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE;
#else
]]
[[#elif CPU(ARM)
    unsigned user_count = ARM_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE;
#elif CPU(AARCH64)
    unsigned user_count = ARM_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE64;
#else
]])

replace_exact("src/3rdparty/javascriptcore/JavaScriptCore/runtime/Collector.cpp"
[[#elif CPU(ARM)
    return reinterpret_cast<void*>(regs.__sp);
#else
]]
[[#elif CPU(ARM) || CPU(AARCH64)
    return reinterpret_cast<void*>(regs.__sp);
#else
]])
