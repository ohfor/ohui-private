# Distribution Automation

## Overview

A PowerShell build script automates the package-and-verify cycle for Nexus releases. This eliminates the "forgot to update version.rc" and "accidentally shipped Debug DLL" classes of errors.

The script in `scripts/build_distribution.ps1` handles: version consistency, smart rebuild detection, Papyrus compilation, file staging, archive creation, structure validation, and old version archival.

## Version Consistency Checking

The most valuable safety check. Extract version strings from all source files and abort if they don't match.

```powershell
function Get-VersionFromSources {
    $versions = @{}

    # Version.h — parse MAJOR/MINOR/PATCH constants
    $content = Get-Content $VersionH -Raw
    if ($content -match 'MAJOR\s*=\s*(\d+)') { $major = $matches[1] }
    if ($content -match 'MINOR\s*=\s*(\d+)') { $minor = $matches[1] }
    if ($content -match 'PATCH\s*=\s*(\d+)') { $patch = $matches[1] }
    $versions["Version.h"] = "$major.$minor.$patch"

    # version.rc — parse FileVersion string
    $content = Get-Content $VersionRC -Raw
    if ($content -match '"FileVersion"\s*,\s*"(\d+\.\d+\.\d+)') {
        $versions["version.rc"] = $matches[1]
    }

    # CMakeLists.txt — parse VERSION_MAJOR/MINOR/PATCH
    $content = Get-Content $CMakeLists -Raw
    if ($content -match 'VERSION_MAJOR\s+(\d+)') { $cmMajor = $matches[1] }
    if ($content -match 'VERSION_MINOR\s+(\d+)') { $cmMinor = $matches[1] }
    if ($content -match 'VERSION_PATCH\s+(\d+)') { $cmPatch = $matches[1] }
    $versions["CMakeLists.txt"] = "$cmMajor.$cmMinor.$cmPatch"

    # vcpkg.json — parse version-string
    $content = Get-Content $VcpkgJson -Raw
    if ($content -match '"version-string"\s*:\s*"(\d+\.\d+\.\d+)"') {
        $versions["vcpkg.json"] = $matches[1]
    }

    # Check all match
    $unique = $versions.Values | Sort-Object -Unique
    if (($unique | Measure-Object).Count -gt 1) {
        Write-Host "VERSION MISMATCH:" -ForegroundColor Red
        foreach ($file in $versions.Keys) {
            Write-Host "  $file : $($versions[$file])"
        }
        return @{ Version = $null; Valid = $false }
    }

    return @{ Version = $unique[0]; Valid = $true }
}
```

Abort the build on mismatch. A mismatched version means someone forgot to update one of the files — shipping this is worse than not shipping at all.

## Timestamp-Based Rebuild Detection

Skip rebuilding packages when nothing has changed since the last build.

```powershell
function Get-NewestFileTime {
    param([string[]]$Paths, [string[]]$Patterns = @("*"))

    $newest = [DateTime]::MinValue
    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) { continue }
        $item = Get-Item $path
        if ($item.PSIsContainer) {
            foreach ($pattern in $Patterns) {
                Get-ChildItem -Path $path -Filter $pattern -Recurse -File |
                    ForEach-Object {
                        if ($_.LastWriteTime -gt $newest) { $newest = $_.LastWriteTime }
                    }
            }
        } else {
            if ($item.LastWriteTime -gt $newest) { $newest = $item.LastWriteTime }
        }
    }
    return $newest
}

function Test-NeedsRebuild {
    param([string]$ZipPath, [string[]]$SourcePaths, [string[]]$Patterns)

    if (-not (Test-Path $ZipPath)) { return $true }

    $zipTime = (Get-Item $ZipPath).LastWriteTime
    $sourceTime = Get-NewestFileTime -Paths $SourcePaths -Patterns $Patterns

    return ($sourceTime -gt $zipTime)
}
```

Use a `-Force` parameter to bypass this check when needed.

## Papyrus Compilation Verification

After running the Papyrus compiler, verify every `.psc` produced a `.pex` and that timestamps are fresh:

```powershell
function Verify-PapyrusCompilation {
    param([string]$SourceDir, [string]$OutputDir, [string]$Prefix)

    $pscFiles = Get-ChildItem -Path $SourceDir -Filter "${Prefix}_*.psc"
    $allGood = $true

    foreach ($psc in $pscFiles) {
        $pexPath = Join-Path $OutputDir "$($psc.BaseName).pex"

        if (-not (Test-Path $pexPath)) {
            Write-Error "Script failed to compile: $($psc.Name)"
            $allGood = $false
            continue
        }

        # Allow 5 second tolerance for filesystem delays
        $pexTime = (Get-Item $pexPath).LastWriteTime
        if ($pexTime -lt $psc.LastWriteTime.AddSeconds(-5)) {
            Write-Warning "Script may be stale: $($psc.Name)"
        }
    }

    return $allGood
}
```

After verification, copy `.pex` files back to the repo's compiled directory.

## File Staging

Build the archive by staging files into a clean directory that mirrors the final structure:

```powershell
function Build-Package {
    param([string]$Version)

    # 1. Validate all sources exist before touching filesystem
    $valid = $true
    $valid = (Test-Path $DllPath) -and $valid
    $valid = (Test-Path $EspPath) -and $valid
    if (-not $valid) {
        Write-Error "Missing required files"
        return $false
    }

    # 2. Clean staging area
    if (Test-Path $StagingDir) {
        Remove-Item -Recurse -Force $StagingDir
    }

    # 3. Create directory tree
    New-Item -ItemType Directory -Path "$StagingDir\SKSE\Plugins" -Force | Out-Null
    New-Item -ItemType Directory -Path "$StagingDir\Scripts" -Force | Out-Null

    # 4. Copy files to staged locations
    Copy-Item $EspPath $StagingDir                          # Root
    Copy-Item $DllPath "$StagingDir\SKSE\Plugins\"          # SKSE/Plugins/
    Copy-Item "$CompiledDir\*.pex" "$StagingDir\Scripts\"   # Scripts/

    # 5. Create archive from staging contents
    $zipPath = Join-Path $OutputDir "MyPlugin-v$Version.zip"
    Compress-Archive -Path "$StagingDir\*" -DestinationPath $zipPath

    return $true
}
```

Key: archive from `$StagingDir\*` (not `$StagingDir`) to avoid an outer folder in the zip.

## Archive Structure Validation

After creating the zip, inspect it programmatically:

```powershell
function Verify-ArchiveStructure {
    param([string]$ZipPath)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($ZipPath)
    $entries = $zip.Entries | ForEach-Object { $_.FullName }
    $zip.Dispose()

    # CRITICAL: No Data/ wrapper
    $hasData = $entries | Where-Object { $_ -like "Data/*" -or $_ -like "Data\*" }
    if ($hasData) {
        Write-Error "Archive contains Data/ folder at root — mod managers will misinstall"
        return $false
    }

    # Verify expected folders
    $hasSKSE = $entries | Where-Object { $_ -like "SKSE/*" }
    if (-not $hasSKSE) { Write-Warning "Missing SKSE/ folder" }

    Write-Host "Verified: $($entries.Count) files, no Data/ wrapper" -ForegroundColor Green
    return $true
}
```

Check both `/` and `\` separators — zip tools vary.

## Old Version Archival

Before building a new version, move old versioned zips to an archive directory:

```powershell
function Archive-OldVersions {
    param([string]$OutputDir, [string]$Pattern, [string]$CurrentFilename)

    $archiveDir = Join-Path $OutputDir "archive"
    $oldZips = Get-ChildItem -Path $OutputDir -Filter $Pattern -ErrorAction SilentlyContinue

    foreach ($zip in $oldZips) {
        if ($zip.Name -eq $CurrentFilename) { continue }  # Skip current version

        if (-not (Test-Path $archiveDir)) {
            New-Item -ItemType Directory -Path $archiveDir -Force | Out-Null
        }
        Move-Item $zip.FullName $archiveDir -Force
        Write-Host "Archived: $($zip.Name)"
    }
}

# Usage
Archive-OldVersions -OutputDir $ModsDir `
    -Pattern "MyPlugin-v*.zip" `
    -CurrentFilename "MyPlugin-v$Version.zip"
```

## Multi-Package Support

For mods with optional components, build packages independently:

```powershell
# Determine what to build
param(
    [switch]$All,
    [switch]$Force,
    [switch]$SkipRebuild
)

# Each package has its own source paths and rebuild detection
$mainSources = @("src", "include", $EspPath, $SourceScripts)
$optionalSources = @($ConfigDir)

$mainNeedsRebuild = $Force -or (Test-NeedsRebuild -ZipPath $MainZip `
    -SourcePaths $mainSources -Patterns @("*.cpp","*.h","*.psc","*.esp"))

$optionalNeedsRebuild = $Force -or (Test-NeedsRebuild -ZipPath $OptionalZip `
    -SourcePaths $optionalSources -Patterns @("*.ini"))

# Build independently
$success = $true
if ($mainNeedsRebuild) {
    if (-not (Build-MainPackage)) { $success = $false }
}
if ($All -and $optionalNeedsRebuild) {
    if (-not (Build-OptionalPackage)) { $success = $false }
}
```

Convention: main package gets version in filename (`MyPlugin-v1.2.0.zip`), config-only packages don't (`MyPlugin-Configs.zip`) since they may update independently of the mod version.

## Overall Script Flow

```
1. Locate project root (validate CLAUDE.md exists)
2. Define all paths (source, output, game directory)
3. Parse command-line options (-Force, -All, -SkipRebuild)
4. VERSION CONSISTENCY CHECK — abort on mismatch
5. Archive old versions
6. Determine what needs rebuilding (per package)
7. Early exit if nothing to do
8. Rebuild artifacts (DLL, Papyrus) if needed
9. Stage and package each component
10. Validate archive structure
11. Report results
```

Each step either succeeds and continues, or fails and aborts with a clear message. No partial builds shipped.
