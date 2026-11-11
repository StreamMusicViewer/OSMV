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
    src/common/obs_widget.cpp
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
    src/gui/css_editor.cpp
    src/gui/gui_main.cpp
    src/gui/locale.cpp
    src/gui/obs_preview.cpp
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
    include(GNUInstallDirs)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS1 REQUIRED IMPORTED_TARGET dbus-1)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    # Prefer system GLFW 3.4+ (required for GLFW_WAYLAND_APP_ID / xdg_toplevel app_id).
    # On older distros shipping GLFW 3.3 (e.g. Ubuntu 24.04), fetch GLFW 3.4 automatically
    # when build dependencies are installed unless disabled via -DOSMV_FETCH_GLFW=OFF.
    option(OSMV_FETCH_GLFW "Fetch GLFW 3.4 when system GLFW >= 3.4 is unavailable" ON)
    find_package(glfw3 3.4 QUIET)
    if(glfw3_FOUND)
        message(STATUS "OSMV: Using system GLFW ${glfw3_VERSION} (Wayland app_id & X11 WM_CLASS enabled)")
    else()
        pkg_check_modules(GLFW_WL_DEPS QUIET
            wayland-client>=0.2.7
            wayland-cursor>=0.2.7
            wayland-egl>=0.2.7
            xkbcommon>=0.5.0
        )
        find_program(WAYLAND_SCANNER_BIN wayland-scanner)
        find_package(X11 QUIET)

        set(_osmv_can_wl FALSE)
        if(GLFW_WL_DEPS_FOUND AND WAYLAND_SCANNER_BIN)
            set(_osmv_can_wl TRUE)
        endif()

        set(_osmv_can_x11 FALSE)
        if(X11_FOUND AND X11_X11_INCLUDE_PATH AND X11_Xrandr_INCLUDE_PATH AND X11_Xinerama_INCLUDE_PATH AND X11_Xcursor_INCLUDE_PATH AND X11_Xi_INCLUDE_PATH AND X11_Xkb_INCLUDE_PATH AND X11_Xshape_INCLUDE_PATH)
            set(_osmv_can_x11 TRUE)
        endif()

        find_package(glfw3 3.3 QUIET)

        if(OSMV_FETCH_GLFW AND (_osmv_can_wl OR (NOT glfw3_FOUND AND _osmv_can_x11)))
            if(NOT _osmv_can_wl)
                message(WARNING "OSMV: Wayland dev headers (libwayland-dev, libwayland-bin, libxkbcommon-dev) not found; fetched GLFW 3.4 will only support X11 (no native Wayland app_id).")
            endif()
            include(FetchContent)
            if(POLICY CMP0135)
                cmake_policy(SET CMP0135 NEW)
            endif()
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_WAYLAND ${_osmv_can_wl} CACHE BOOL "" FORCE)
            set(GLFW_BUILD_X11 ${_osmv_can_x11} CACHE BOOL "" FORCE)
            FetchContent_Declare(
                glfw
                URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
                URL_HASH SHA256=b5ec004b2712fd08e8861dc271428f048775200a2df719ccf575143ba749a3e9
            )
            FetchContent_MakeAvailable(glfw)
            message(STATUS "OSMV: Using fetched GLFW 3.4 (Wayland=${_osmv_can_wl}, X11=${_osmv_can_x11})")
        elseif(glfw3_FOUND)
            message(WARNING "OSMV: Using system GLFW ${glfw3_VERSION} (< 3.4). Native Wayland app_id requires GLFW 3.4+; install system GLFW 3.4+ or Wayland dev headers for automatic GLFW 3.4.")
        else()
            find_package(glfw3 3.4 REQUIRED)
        endif()
    endif()

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

    install(TARGETS osmv osmv-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES linux/io.github.streammusicviewer.OSMV.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES linux/io.github.streammusicviewer.OSMV.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/96x96/apps)
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
