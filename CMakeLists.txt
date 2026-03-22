cmake_minimum_required(VERSION 3.21)
project(OSMV VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce dynamic MSVC runtime (/MD or /MDd) to match Qt libraries and prevent ucrtbase.dll crashes
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_AUTOMOC ON)   # Auto-run moc for Qt
set(CMAKE_AUTORCC ON)

# ── Qt 6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)


# ── Source files ──────────────────────────────────────────────────────────────
set(COMMON_SRC
    src/main.cpp
    src/app.cpp
    src/app.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/utils.cpp
    src/utils.h
    src/mediaprovider.h
)

if(WIN32)
    set(PLATFORM_SRC src/mediaprovider_win.cpp)
    # Windows-specific: WinRT requires /std:c++20 and WindowsAppSDK or SDK headers
    set(PLATFORM_LIBS windowsapp)   # Needed for WinRT async
else()
    set(PLATFORM_SRC src/mediaprovider_linux.cpp)
    set(PLATFORM_LIBS "")
endif()

# ── Executable ────────────────────────────────────────────────────────────────
if(WIN32)
    # Restored WIN32 flag to hide console window in production
    qt_add_executable(osmv WIN32 ${COMMON_SRC} ${PLATFORM_SRC} windows/OSMV.rc)
else()
    qt_add_executable(osmv ${COMMON_SRC} ${PLATFORM_SRC})
endif()

target_include_directories(osmv PRIVATE
    src
)

target_link_libraries(osmv
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        ${PLATFORM_LIBS}
)

# Windows: enable WinRT and C++/WinRT
if(WIN32)
    if(MSVC)
        target_compile_options(osmv PRIVATE /std:c++20 /EHsc)
    endif()
    target_compile_definitions(osmv PRIVATE
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        WINVER=0x0A00          # Windows 10+
        _WIN32_WINNT=0x0A00
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS osmv DESTINATION .)
install(DIRECTORY shared/ DESTINATION .)   # Copy index.html, style.css
install(FILES settings.json DESTINATION . OPTIONAL)
