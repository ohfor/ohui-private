# OHUI Deploy Script
# Usage:
#   .\scripts\deploy.ps1              - Build + deploy DLL only
#   .\scripts\deploy.ps1 -Papyrus     - Build + deploy DLL + compile all Papyrus
#   .\scripts\deploy.ps1 -Papyrus -Scripts OHUI_MCM,OHUI_Reset  - specific scripts only
#   .\scripts\deploy.ps1 -PapyrusOnly - Compile Papyrus only (skip C++ build)
#   .\scripts\deploy.ps1 -PapyrusOnly -Scripts OHUI_MCM         - single script only
#   .\scripts\deploy.ps1 -SkipTests   - Build + deploy DLL, skip test gate

param(
    [switch]$Papyrus,
    [switch]$PapyrusOnly,
    [switch]$SkipTests,
    [string[]]$Scripts
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
$BuildDir = "$RepoRoot/build/release"
$SkyrimDir = "D:/SteamLibrary/steamapps/common/Skyrim Special Edition"
$PluginDir = "$SkyrimDir/Data/SKSE/Plugins"
$GameScripts = "$SkyrimDir/Data/Scripts"
$Compiler = "$SkyrimDir/Papyrus Compiler/PapyrusCompiler.exe"
$Flags = "$SkyrimDir/Data/source/Scripts/TESV_Papyrus_Flags.flg"
$Imports = "$SkyrimDir/Data/Scripts/Source;$SkyrimDir/Data/source/Scripts;$RepoRoot/scripts/papyrus/source"

$failed = $false

# --- C++ Build + Deploy ---
if (-not $PapyrusOnly) {
    Write-Host "`n=== C++ Build ===" -ForegroundColor Cyan
    cmake --build $BuildDir --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "BUILD FAILED" -ForegroundColor Red
        exit 1
    }

    # --- Test gate (after build, before deploy) ---
    if (-not $SkipTests) {
        $testExe = "$BuildDir/tests/Release/ohui_tests.exe"
        if (Test-Path $testExe) {
            Write-Host "`n=== Running Tests ===" -ForegroundColor Cyan
            ctest --test-dir $BuildDir --build-config Release --output-on-failure
            if ($LASTEXITCODE -ne 0) {
                Write-Host "TESTS FAILED - deploy blocked" -ForegroundColor Red
                Write-Host "Use -SkipTests to deploy anyway" -ForegroundColor Yellow
                exit 1
            }
            Write-Host "  All tests passed" -ForegroundColor Green
        } else {
            Write-Host "`n=== Tests not built - skipping ===" -ForegroundColor Yellow
        }
    }

    Write-Host "`n=== Deploy DLL ===" -ForegroundColor Cyan
    Copy-Item "$BuildDir/Release/OHUI.dll" $PluginDir -Force
    $dll = Get-Item "$PluginDir/OHUI.dll"
    Write-Host "  DLL deployed: $($dll.LastWriteTime)"
}

# --- Papyrus Compile ---
if ($Papyrus -or $PapyrusOnly) {
    Write-Host "`n=== Papyrus Compile ===" -ForegroundColor Cyan

    if ($Scripts) {
        # Handle both space-separated and comma-separated input
        $scriptList = $Scripts | ForEach-Object { $_ -split ',' } | Where-Object { $_ -ne '' }
    } else {
        $scriptList = Get-ChildItem "$RepoRoot/scripts/papyrus/source/OHUI_*.psc" | ForEach-Object { $_.BaseName }
    }

    foreach ($script in $scriptList) {
        $psc = "$RepoRoot/scripts/papyrus/source/$script.psc"
        if (-not (Test-Path $psc)) {
            Write-Host "  SKIP: $script.psc not found" -ForegroundColor Yellow
            continue
        }

        Write-Host "  Compiling $script..."
        & $Compiler $psc "-f=$Flags" "-i=$Imports" "-o=$GameScripts" 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  FAILED: $script" -ForegroundColor Red
            $failed = $true
            continue
        }

        # Copy compiled .pex back to repo
        $pex = "$GameScripts/$script.pex"
        if (Test-Path $pex) {
            Copy-Item $pex "$RepoRoot/scripts/papyrus/compiled/" -Force
            Copy-Item $pex "$RepoRoot/dist/Scripts/" -Force
            $ts = (Get-Item $pex).LastWriteTime
            Write-Host "  OK: $script.pex ($ts)"
        }
    }
}

# --- Summary ---
Write-Host "`n=== Done ===" -ForegroundColor $(if ($failed) { 'Yellow' } else { 'Green' })
if ($failed) {
    Write-Host "Some steps failed - check output above" -ForegroundColor Yellow
    exit 1
}
