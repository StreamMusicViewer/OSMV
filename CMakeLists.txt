cmake_minimum_required(VERSION 3.21)
project(OSMV VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # Auto-run moc for Qt
set(CMAKE_AUTORCC ON)

# ── Qt 6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# ── discord-rpc (fetched automatically) ───────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    discord_rpc
    GIT_REPOSITORY https://github.com/discord/discord-rpc.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(ENABLE_IO_THREAD OFF CACHE BOOL "" FORCE)
set(CLANG_FORMAT_SUFFIX "-dummy-disabled" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(discord_rpc)

# Fix: discord-rpc's bundled rapidjson is not C++20-compliant (GCC 15).
# Apply relaxed flags only to the discord-rpc target so our code stays strict.
target_compile_options(discord-rpc PRIVATE
    -fpermissive
    -Wno-error
    -Wno-class-memaccess
    -Wno-uninitialized
    # Remove Clang-only flags that GCC doesn't know about:
    -Wno-unknown-warning-option
)


# ── Source files ──────────────────────────────────────────────────────────────
set(COMMON_SRC
    src/main.cpp
    src/app.cpp
    src/app.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/discordrpc.cpp
    src/discordrpc.h
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
    # WIN32 flag hides the console window on Windows
    qt_add_executable(osmv WIN32 ${COMMON_SRC} ${PLATFORM_SRC} windows/OSMV.rc)
else()
    qt_add_executable(osmv ${COMMON_SRC} ${PLATFORM_SRC})
endif()

target_include_directories(osmv PRIVATE
    src
    ${discord_rpc_SOURCE_DIR}/include
)

target_link_libraries(osmv
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        discord-rpc
        ${PLATFORM_LIBS}
)

# Windows: enable WinRT and C++/WinRT
if(WIN32)
    target_compile_options(osmv PRIVATE /std:c++20 /EHsc)
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
