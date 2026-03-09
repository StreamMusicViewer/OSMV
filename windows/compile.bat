@echo off
REM ─────────────────────────────────────────────────────────────────────────────
REM compile.bat — Windows build script for OBS Stream Music Viewer v2 (C++ / Qt 6)
REM ─────────────────────────────────────────────────────────────────────────────
setlocal

echo ═══════════════════════════════════════════════════════
echo  OBS Stream Music Viewer — Windows Build (C++ / Qt 6)
echo ═══════════════════════════════════════════════════════
echo.

REM ── 1. Check CMake ───────────────────────────────────────────────────────────
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERREUR: cmake n'est pas installe.
    echo Telechargez depuis: https://cmake.org/download/
    pause & exit /b 1
)
echo OK: cmake trouve

REM ── 2. Check Qt 6 ────────────────────────────────────────────────────────────
REM Adjust this path to your Qt installation if needed
if not defined QTDIR (
    REM Try common install paths
    if exist "C:\Qt\6.7.0\msvc2019_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.7.0\msvc2019_64
    ) else if exist "C:\Qt\6.6.0\msvc2019_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.6.0\msvc2019_64
    ) else (
        echo AVERTISSEMENT: Qt 6 non trouve automatiquement.
        echo Definissez QTDIR manuellement, ex:
        echo   set QTDIR=C:\Qt\6.7.0\msvc2019_64
        echo puis relancez ce script.
        pause & exit /b 1
    )
)
set PATH=%QTDIR%\bin;%PATH%
echo OK: Qt 6 trouve: %QTDIR%

REM ── 3. Kill running instance ──────────────────────────────────────────────────
taskkill /F /IM "osmv.exe" >nul 2>&1
timeout /t 1 /nobreak >nul

REM ── 4. Configure ────────────────────────────────────────────────────────────
echo.
echo Configuration CMake...
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%QTDIR%" ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERREUR: Configuration CMake echouee.
    echo Si vous utilisez MinGW, utilisez:
    echo   cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QTDIR%" -DCMAKE_BUILD_TYPE=Release
    pause & exit /b 1
)

REM ── 5. Build ─────────────────────────────────────────────────────────────────
echo.
echo Compilation...
cmake --build build --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ═══════════════════════════════════════════════════════
    echo OK: Build reussi!
    echo    Binaire: build\Release\osmv.exe
    echo.
    echo    Pour deployer, copiez dans le meme dossier:
    echo    - build\Release\osmv.exe
    echo    - shared\index.html
    echo    - shared\style.css
    echo    - settings.json ^(optionnel^)
    echo ═══════════════════════════════════════════════════════
) else (
    echo.
    echo ERREUR: La compilation a echoue.
)

pause
