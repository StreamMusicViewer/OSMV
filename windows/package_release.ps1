param(
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'

$RootDir = Split-Path -Parent $PSScriptRoot
Set-Location $RootDir

$CMakeText = Get-Content -Path (Join-Path $RootDir 'CMakeLists.txt') -Raw
if ($CMakeText -notmatch 'project\s*\(\s*OSMV\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)') {
    throw "Could not parse OSMV version from CMakeLists.txt"
}
$Version = $Matches[1]
$Tag = "v$Version"
$ZipName = "OSMV_${Tag}_win.zip"

if (-not $SkipBuild) {
    Write-Host "Building OSMV $Tag (Windows Release)..."
    & cmd /c "`"$RootDir\windows\build.bat`""
    if ($LASTEXITCODE -ne 0) {
        throw "windows\build.bat failed with exit code $LASTEXITCODE"
    }
}

$ExePath = Join-Path $RootDir 'OSMV.exe'
if (-not (Test-Path $ExePath)) {
    throw "OSMV.exe not found at project root. Build first or omit -SkipBuild."
}

$ReleaseDir = Join-Path $RootDir 'release'
$StageDir = Join-Path $ReleaseDir 'win'
$SharedStageDir = Join-Path $StageDir 'shared'
$ZipPath = Join-Path $ReleaseDir $ZipName
$SumsPath = Join-Path $ReleaseDir 'SHA256SUMS.txt'

New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null
Get-ChildItem -Path $ReleaseDir -Filter 'OSMV_v*_win.zip' -ErrorAction SilentlyContinue | Remove-Item -Force

if (Test-Path $StageDir) {
    Remove-Item -Recurse -Force $StageDir
}
New-Item -ItemType Directory -Force -Path $SharedStageDir | Out-Null

foreach ($doc in @('OSMV.exe', 'LICENSE', 'README.md', 'TROUBLESHOOTING.md', 'CHANGELOG.md')) {
    $src = Join-Path $RootDir $doc
    if (-not (Test-Path $src)) {
        throw "Required release file missing: $src"
    }
    Copy-Item -Path $src -Destination (Join-Path $StageDir $doc) -Force
}

foreach ($asset in @('index.html', 'preview.html', 'style.css', 'widget.js')) {
    $src = Join-Path $RootDir "shared\$asset"
    if (-not (Test-Path $src)) {
        throw "Required shared widget asset missing: $src"
    }
    Copy-Item -Path $src -Destination (Join-Path $SharedStageDir $asset) -Force
}

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}

Compress-Archive -Path "$StageDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal -Force

$ZipHash = (Get-FileHash -Path $ZipPath -Algorithm SHA256).Hash.ToLowerInvariant()
$ExeHash = (Get-FileHash -Path (Join-Path $StageDir 'OSMV.exe') -Algorithm SHA256).Hash.ToLowerInvariant()

$SumsContent = @(
    "$ZipHash  $ZipName"
    "$ExeHash  win/OSMV.exe"
) -join "`n"
Set-Content -Path $SumsPath -Value "$SumsContent`n" -Encoding Ascii -NoNewline

Write-Host ""
Write-Host "Prepared Windows release $Tag in $ReleaseDir"
Write-Host "  Archive : $ZipPath"
Write-Host "  SHA-256 : $ZipHash  $ZipName"
Write-Host "  SHA-256 : $ExeHash  win/OSMV.exe"

