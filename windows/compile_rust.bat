@echo off
REM Build script for OSMV Rust — Windows
REM Adds Rust's cargo to PATH in case the terminal hasn't reloaded it yet.

set "PATH=%USERPROFILE%\.cargo\bin;%PATH%"

echo Building OSMV Rust (release)...
cargo build --release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed! Make sure Rust is installed:
    echo   winget install --id Rustlang.Rustup
    pause
    exit /b 1
)

echo.
echo Copying binary to project root...
copy /Y "target\release\osmv.exe" "OSMV.exe"

echo.
echo Done! OSMV.exe is ready.
echo Place it alongside shared\index.html and shared\style.css for OBS.
pause
