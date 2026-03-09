@echo off
REM Script de compilation du OBS Stream Music Viewer (Windows)
REM Les fichiers du widget OBS (index.html, style.css) se trouvent dans ..\shared\

echo Compilation de l'interface graphique (OBS-StreamMusicViewer.exe)...
echo.

REM Vérifier si dotnet est installé
where dotnet >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERREUR: .NET SDK n'est pas installe.
    echo.
    echo Telechargez et installez .NET SDK depuis:
    echo https://dotnet.microsoft.com/download
    echo.
    pause
    exit /b 1
)

REM Fermer l'application si elle tourne (pour libérer le .exe avant de l'écraser)
echo Fermeture de l'application si elle est en cours d'exécution...
taskkill /F /IM "OBS-StreamMusicViewer.exe" >nul 2>&1
timeout /t 1 /nobreak >nul

REM Compiler le projet
dotnet publish OBS-StreamMusicViewer.csproj -c Release -o .

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo Compilation reussie!
    echo L'executable OBS-StreamMusicViewer.exe est pret.
    echo.
    echo N'oubliez pas de copier les fichiers OBS widget:
    echo   ..\shared\index.html
    echo   ..\shared\style.css
    echo dans le meme dossier que l'executable.
    echo ============================================
    echo.
) else (
    echo.
    echo ============================================
    echo ERREUR lors de la compilation
    echo ============================================
    echo.
)

pause
