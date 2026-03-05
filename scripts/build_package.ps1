<#
.SYNOPSIS
    Build OHUI Nexus release packages.

.DESCRIPTION
    Builds DLL, creates OHUI-Main.zip from dist/.

.PARAMETER Version
    Expected version (e.g., "1.0.0"). If provided, validates all files match.

.PARAMETER SkipBuild
    Skip DLL rebuild (use existing build output).

.EXAMPLE
    .\scripts\build_package.ps1 -Version 1.0.0
#>

param(
    [string]$Version,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

# Find project root (where CLAUDE.md lives)
$root = $PSScriptRoot
while ($root -and -not (Test-Path (Join-Path $root "CLAUDE.md"))) {
    $root = Split-Path -Parent $root
}
if (-not $root) {
    Write-Error "Cannot find project root (no CLAUDE.md)"
    exit 1
}

# Paths
$distDir = Join-Path $root "dist"
$assetsDir = Join-Path $root "assets"
$buildDir = Join-Path $root "build\release\Release"
$dllPath = Join-Path $buildDir "OHUI.dll"

# Ensure assets directory exists
New-Item -ItemType Directory -Path $assetsDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $assetsDir "archive") -Force | Out-Null

# Read version from Version.h (source of truth)
$versionH = Get-Content (Join-Path $root "include\Version.h") -Raw
$vMajor = [regex]::Match($versionH, 'MAJOR\s*=\s*(\d+)').Groups[1].Value
$vMinor = [regex]::Match($versionH, 'MINOR\s*=\s*(\d+)').Groups[1].Value
$vPatch = [regex]::Match($versionH, 'PATCH\s*=\s*(\d+)').Groups[1].Value
$detectedVersion = "$vMajor.$vMinor.$vPatch"

# If -Version provided, validate it matches; otherwise use detected
if ($Version) {
    if ($Version -ne $detectedVersion) {
        Write-Error "Requested version $Version but Version.h says $detectedVersion"
        exit 1
    }
} else {
    $Version = $detectedVersion
}

# Version consistency check
Write-Host "Checking versions..." -ForegroundColor Cyan
& python "$root\scripts\check_versions.py" "$Version"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Version check failed - update version files before packaging"
    exit 1
}

# Build DLL if needed
if (-not $SkipBuild) {
    Write-Host "`nBuilding DLL..." -ForegroundColor Cyan
    & cmake --build "$root\build\release" --config Release
    if ($LASTEXITCODE -ne 0) {
        Write-Error "DLL build failed"
        exit 1
    }
}

# Verify DLL exists
if (-not (Test-Path $dllPath)) {
    Write-Error "DLL not found: $dllPath"
    exit 1
}
$dllSize = [math]::Round((Get-Item $dllPath).Length / 1KB)
Write-Host "  DLL: $dllSize KB"

# Archive old versioned zips matching OHUI-Main-v*.zip
$mainZip = Join-Path $assetsDir "OHUI-Main-v$Version.zip"
Get-ChildItem $assetsDir -Filter "OHUI-Main-v*.zip" -ErrorAction SilentlyContinue | ForEach-Object {
    $timestamp = $_.LastWriteTime.ToString("yyyyMMdd-HHmmss")
    $archiveName = $_.BaseName + "-$timestamp.zip"
    Move-Item $_.FullName (Join-Path $assetsDir "archive\$archiveName")
    Write-Host "  Archived previous: $archiveName"
}

# Create temp staging directory (copy dist/ + add DLL)
Write-Host "`nStaging OHUI-Main..." -ForegroundColor Cyan
$tempStaging = Join-Path $env:TEMP "OHUI-Main-$(Get-Random)"
New-Item -ItemType Directory -Path $tempStaging -Force | Out-Null

try {
    # Copy entire dist/ structure (if it has content)
    if (Test-Path $distDir) {
        $distContent = Get-ChildItem $distDir -ErrorAction SilentlyContinue
        if ($distContent) {
            Copy-Item "$distDir\*" $tempStaging -Recurse
        }
    }

    # Ensure SKSE/Plugins/ exists and add DLL
    $sksePluginsDir = Join-Path $tempStaging "SKSE\Plugins"
    New-Item -ItemType Directory -Path $sksePluginsDir -Force | Out-Null
    Copy-Item $dllPath $sksePluginsDir

    # Create zip
    Write-Host "Creating OHUI-Main.zip..." -ForegroundColor Cyan
    Compress-Archive -Path "$tempStaging\*" -DestinationPath $mainZip -Force

    # Report
    $zipInfo = Get-Item $mainZip
    $zipSize = [math]::Round($zipInfo.Length / 1KB)
    Write-Host "  Created: $($zipInfo.Name) ($zipSize KB)" -ForegroundColor Green

    # List contents summary
    Write-Host "  Contents:"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($mainZip)
    $zip.Entries | Group-Object { Split-Path $_.FullName } | ForEach-Object {
        $folder = if ($_.Name) { $_.Name } else { "(root)" }
        Write-Host "    $folder : $($_.Count) files"
    }
    $zip.Dispose()

} finally {
    # Cleanup temp staging
    Remove-Item -Recurse -Force $tempStaging -ErrorAction SilentlyContinue
}

Write-Host "`nPackaging complete!" -ForegroundColor Green
Write-Host "  assets/OHUI-Main-v$Version.zip - upload to Nexus Main Files"
