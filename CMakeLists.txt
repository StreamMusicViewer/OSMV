cmake_minimum_required(VERSION 3.20)
project(OSMV VERSION 5.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

# ── Common Library (zero GUI/GPU dependencies) ────────────────────────────────
set(OSMV_COMMON_SOURCES
    src/common/formatter.cpp
    src/common/http_common.cpp
    src/common/ipc.cpp
    src/common/os_utils.cpp
    src/common/settings.cpp
)

if(WIN32)
    list(APPEND OSMV_COMMON_SOURCES src/common/http_win.cpp)
else()
    list(APPEND OSMV_COMMON_SOURCES src/common/http_linux.cpp)
endif()

add_library(osmv_common STATIC ${OSMV_COMMON_SOURCES})
target_include_directories(osmv_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

if(WIN32)
    target_link_libraries(osmv_common PUBLIC winhttp psapi shell32 advapi32)
else()
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(osmv_common PUBLIC CURL::libcurl Threads::Threads)
endif()

# ── Shared Daemon Sources ─────────────────────────────────────────────────────
set(OSMV_DAEMON_SOURCES
    src/daemon/daemon_main.cpp
    src/daemon/discord_rpc.cpp
    src/daemon/dynamic_files.cpp
    src/daemon/twitch_client.cpp
)

if(WIN32)
    list(APPEND OSMV_DAEMON_SOURCES
        src/daemon/media_win.cpp
        src/daemon/sysinfo_win.cpp
        src/daemon/tray_win.cpp
    )
else()
    list(APPEND OSMV_DAEMON_SOURCES
        src/daemon/media_linux.cpp
        src/daemon/sysinfo_linux.cpp
        src/daemon/tray_linux.cpp
        src/common/embedded_logo.cpp
    )
endif()

# ── Shared GUI + Dear ImGui Sources ───────────────────────────────────────────
set(OSMV_IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/misc/cpp/imgui_stdlib.cpp
)

set(OSMV_GUI_SOURCES
    src/common/embedded_fonts.cpp
    src/common/embedded_logo.cpp
    src/gui/gui_main.cpp
    src/gui/locale.cpp
    src/gui/theme.cpp
    src/gui/twitch_oauth.cpp
    src/gui/view_discord.cpp
    src/gui/view_now_playing.cpp
    src/gui/view_settings_help.cpp
    src/gui/view_sysinfo.cpp
    src/gui/view_text_files.cpp
    src/gui/view_time_date.cpp
    src/gui/view_timers.cpp
    src/gui/view_twitch.cpp
)

if(WIN32)
    list(APPEND OSMV_IMGUI_SOURCES
        third_party/imgui/backends/imgui_impl_win32.cpp
        third_party/imgui/backends/imgui_impl_dx11.cpp
    )
    list(APPEND OSMV_GUI_SOURCES src/gui/backend_win32_dx11.cpp)

    # Single Windows executable with delay-loaded DX11/DWM so Daemon loads 0 GPU DLLs
    add_executable(OSMV WIN32
        src/main.cpp
        ${OSMV_DAEMON_SOURCES}
        ${OSMV_GUI_SOURCES}
        ${OSMV_IMGUI_SOURCES}
        windows/OSMV.rc
    )
    target_include_directories(OSMV PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    )
    target_link_libraries(OSMV PRIVATE
        osmv_common
        windowsapp
        iphlpapi
        ws2_32
        user32
        gdi32
        d3d11
        dxgi
        dwmapi
        delayimp
    )
    if(MSVC)
        target_compile_definitions(osmv_common PUBLIC UNICODE _UNICODE)
        target_compile_definitions(OSMV PRIVATE UNICODE _UNICODE)
        target_compile_options(OSMV PRIVATE /utf-8 /EHsc /bigobj /W3 /Gw /Gy)
        target_compile_options(osmv_common PRIVATE /utf-8 /EHsc /W3 /Gw /Gy)
        target_link_options(OSMV PRIVATE
            /DELAYLOAD:d3d11.dll
            /DELAYLOAD:dxgi.dll
            /DELAYLOAD:dwmapi.dll
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
        )
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS1 REQUIRED IMPORTED_TARGET dbus-1)
    find_package(glfw3 3.3 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    # 1. Pure ultra-lightweight Daemon (0 X11/Wayland/OpenGL/GTK dependencies)
    add_executable(osmv
        src/main.cpp
        ${OSMV_DAEMON_SOURCES}
    )
    target_link_libraries(osmv PRIVATE
        osmv_common
        PkgConfig::DBUS1
        Threads::Threads
    )
    target_compile_options(osmv PRIVATE -Wall -Wextra -Wno-missing-field-initializers -ffunction-sections -fdata-sections)
    target_link_options(osmv PRIVATE -Wl,--gc-sections)

    # 2. Dedicated GUI process (ImGui + GLFW + OpenGL3)
    list(APPEND OSMV_IMGUI_SOURCES
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
    )
    add_executable(osmv-gui
        src/gui/linux_gui_entry.cpp
        ${OSMV_GUI_SOURCES}
        ${OSMV_IMGUI_SOURCES}
    )
    target_include_directories(osmv-gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    )
    target_link_libraries(osmv-gui PRIVATE
        osmv_common
        glfw
        OpenGL::GL
        Threads::Threads
    )
    target_compile_options(osmv-gui PRIVATE -Wall -Wextra -Wno-missing-field-initializers -ffunction-sections -fdata-sections)
    target_link_options(osmv-gui PRIVATE -Wl,--gc-sections)
endif()

# ── Unit Tests ────────────────────────────────────────────────────────────────
if(BUILD_TESTING)
    add_executable(osmv_tests tests/test_main.cpp)
    target_link_libraries(osmv_tests PRIVATE osmv_common)
    if(MSVC)
        target_compile_options(osmv_tests PRIVATE /utf-8 /EHsc /W3)
    endif()
    add_test(NAME osmv_unit_tests COMMAND osmv_tests)
endif()
