@echo off
REM ─────────────────────────────────────────────────────────────────────────────
REM compile.bat — Windows build script for OBS Stream Music Viewer v2 (C++ / Qt 6)
REM ─────────────────────────────────────────────────────────────────────────────
setlocal

echo -------------------------------------------------------
echo  OBS Stream Music Viewer - Windows Build (C++ / Qt 6)
echo -------------------------------------------------------
echo.

REM ── 1. Check CMake ───────────────────────────────────────────────────────────
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    REM Try Qt-bundled cmake as fallback
    if exist "C:\Qt\Tools\CMake_64\bin\cmake.exe" (
        set "PATH=C:\Qt\Tools\CMake_64\bin;%PATH%"
    ) else if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\CMake\bin;%PATH%"
    ) else (
        echo ERREUR: cmake n'est pas installe.
        echo Telechargez depuis: https://cmake.org/download/
        pause & exit /b 1
    )
)
echo OK: cmake trouve

REM ── 2. Check Qt 6 ────────────────────────────────────────────────────────────
REM Adjust this path to your Qt installation if needed
if not defined QTDIR (
    REM Try common install paths (Prioritize MSVC because WinRT requires it)
    if exist "C:\Qt\6.10.2\msvc2022_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.10.2\msvc2022_64
    ) else if exist "C:\Qt\6.10.2\msvc2019_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.10.2\msvc2019_64
    ) else if exist "C:\Qt\6.7.0\msvc2019_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.7.0\msvc2019_64
    ) else if exist "C:\Qt\6.6.0\msvc2019_64\bin\qmake.exe" (
        set QTDIR=C:\Qt\6.6.0\msvc2019_64
    ) else (
        echo AVERTISSEMENT: Qt 6 ^(MSVC^) non trouve automatiquement.
        echo Assurez-vous d'avoir installe Qt avec le composant MSVC 2022 64-bit.
        echo Definissez QTDIR manuellement, ex:
        echo   set QTDIR=C:\Qt\6.10.2\msvc2022_64
        echo puis relancez ce script.
        pause & exit /b 1
    )
)

REM ── Detect generator (MinGW vs MSVC) ────────────────────────────────────────
set CMAKE_GENERATOR=NMake Makefiles
set CMAKE_EXTRA_ARGS=
set BUILD_BIN=build\osmv.exe
set "PATH=%QTDIR%\bin;%PATH%"
echo OK: Qt 6 trouve: %QTDIR%

REM ── 3. Find Visual Studio and setup environment ────────────────────────────────
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set VS_INSTALL_DIR=%%i
    )
)
if defined VS_INSTALL_DIR (
    if exist "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
        echo Configuration de l'environnement Visual Studio...
        call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
)

REM ── 4. Kill running instance ──────────────────────────────────────────────────
taskkill /F /IM "osmv.exe" >nul 2>&1
timeout /t 1 /nobreak >nul

REM ── 5. Configure ────────────────────────────────────────────────────────────
echo.
echo Configuration CMake...
echo    Generator : %CMAKE_GENERATOR%
cmake .. -B build -G "%CMAKE_GENERATOR%" %CMAKE_EXTRA_ARGS% ^
    -DCMAKE_PREFIX_PATH="%QTDIR%" ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

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
cmake --build build --config RelWithDebInfo

if %ERRORLEVEL% EQU 0 (
    echo.
    echo -------------------------------------------------------
    echo OK: Build reussi!
    echo    Binaire: %BUILD_BIN%
    echo.
    echo Mise en place du dossier de deploiement...
    copy /Y "%BUILD_BIN%" "..\OSMV.exe" >nul
    
    echo Deploiement des DLL Qt avec windeployqt...
    "%QTDIR%\bin\windeployqt.exe" "..\OSMV.exe" --no-translations --compiler-runtime >nul 2>&1
    
    REM Force copy VC runtime just in case windeployqt misses it
    copy /Y "C:\Windows\System32\vcruntime140*.dll" "..\" >nul 2>&1
    copy /Y "C:\Windows\System32\msvcp140*.dll" "..\" >nul 2>&1
    copy /Y "C:\Windows\System32\concrt140*.dll" "..\" >nul 2>&1
    copy /Y "C:\Windows\System32\ucrtbase.dll" "..\" >nul 2>&1
    
    echo Nettoyage des fichiers inutiles...
    if exist "..\qt.conf" del /Q "..\qt.conf"
    if exist "..\bin" rmdir /S /Q "..\bin"
    
    REM ── Check for WinRAR to create a true Standalone EXE ──
    set WINRAR="C:\Program Files\WinRAR\WinRAR.exe"
    if exist %WINRAR% (
        echo Creation de l'executable Standalone ^(SFX^) avec WinRAR...
        echo ;The comment below contains SFX script commands > sfx_cfg.txt
        echo Setup=OSMV.exe >> sfx_cfg.txt
        echo TempMode >> sfx_cfg.txt
        echo Silent=1 >> sfx_cfg.txt
        echo Overwrite=1 >> sfx_cfg.txt
        echo Title=OSMV >> sfx_cfg.txt
        
        REM Package everything into the Standalone EXE
        %WINRAR% a -sfx -ep1 -z"sfx_cfg.txt" "..\OSMV_Standalone.exe" "..\OSMV.exe" "..\*.dll" "..\shared" >nul
        if exist sfx_cfg.txt del /Q sfx_cfg.txt
        
        echo.
        echo -------------------------------------------------------
        echo OK: Executable unique genere : OSMV_Standalone.exe
        echo    Ce fichier contient TOUT ^(DLLs + Web^) et peut etre deplace seul.
        echo -------------------------------------------------------
    ) else (
        echo.
        echo -------------------------------------------------------
        echo OK: Deploiement termine avec succes !
        echo    WinRAR non trouve, utilisez le dossier actuel ^(OSMV.exe + DLLs^).
        echo -------------------------------------------------------
    )
) else (
    echo.
    echo ERREUR: La compilation a echoue.
)

exit /b 0
