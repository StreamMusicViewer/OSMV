@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

echo Detecting Visual Studio C++ Toolchain...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio Installer vswhere.exe not found.
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo ERROR: Visual Studio C++ x64 Build Tools not found.
    exit /b 1
)

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 > nul

echo Configuring OSMV C++20 (Release)...
cmake -B windows\build -G Ninja -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 exit /b 1

rem Force clean compile so localized MSVC /showIncludes never leaves stale .obj offsets when headers change
echo Building OSMV...
cmake --build windows\build --clean-first -j
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Running unit tests...
ctest --test-dir windows\build --output-on-failure
if %ERRORLEVEL% NEQ 0 exit /b 1

copy /Y "windows\build\OSMV.exe" "OSMV.exe" > nul
echo.
echo Done! OSMV.exe is ready at project root (no external DLLs required).

