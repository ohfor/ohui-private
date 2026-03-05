# Papyrus Workflow

## Overview

Papyrus is Skyrim's scripting language. Even if your plugin is primarily C++, you'll likely need Papyrus for MCM menus, magic effect scripts, and quest event handlers. The C++ side handles native function registration (see `papyrus-native-functions.md`). This doc covers the file management and compilation workflow.

## Repository Structure

```
scripts/
└── papyrus/
    ├── source/             .psc source files (tracked in git)
    │   ├── MyPlugin_MCM.psc
    │   ├── MyPlugin_NativeFunctions.psc
    │   └── MyPlugin_SomeScript.psc
    ├── compiled/           .pex compiled files (tracked in git, for distribution)
    │   ├── MyPlugin_MCM.pex
    │   ├── MyPlugin_NativeFunctions.pex
    │   └── MyPlugin_SomeScript.pex
    └── compile.bat         Compilation helper script
```

Both `.psc` and `.pex` live in the repo. The source files are the canonical version; compiled files are tracked so the distribution build can package them without requiring a Papyrus compiler on the build machine.

## The Compile Loop

Papyrus compilation targets the game directory (the compiler writes `.pex` files to `Data\Scripts\`). After compilation, copy the fresh `.pex` files back to the repo for distribution:

```
.psc in repo → compile → .pex in game directory → copy back → .pex in repo
```

1. Edit `.psc` in `scripts/papyrus/source/`
2. Run the compiler (targets game `Data\Scripts\`)
3. Test in-game
4. Copy `.pex` back to `scripts/papyrus/compiled/`
5. Commit both `.psc` and `.pex`

## Compilation

### Prerequisites

- **PapyrusCompiler.exe**: Ships with the Creation Kit. Located at `{SKYRIM_DIR}\Papyrus Compiler\PapyrusCompiler.exe`
- **Flags file**: `{SKYRIM_DIR}\Data\source\Scripts\TESV_Papyrus_Flags.flg`
- **Vanilla script sources**: `{SKYRIM_DIR}\Data\source\Scripts\` (shipped with Creation Kit)
- **SkyUI SDK** (if using MCM): See setup below

### Compiler Invocation

```cmd
PapyrusCompiler.exe "MyPlugin_MCM.psc" ^
    -f="{SKYRIM_DIR}\Data\source\Scripts\TESV_Papyrus_Flags.flg" ^
    -i="{SKYRIM_DIR}\Data\Scripts\Source;{SKYRIM_DIR}\Data\source\Scripts;scripts\papyrus\source" ^
    -o="{SKYRIM_DIR}\Data\Scripts"
```

Import path order matters: SKSE/SkyUI sources first, vanilla second, your sources last.

### Compile Script

A batch script avoids retyping paths. Template:

```bat
@echo off
setlocal

set SKYRIM=D:\SteamLibrary\steamapps\common\Skyrim Special Edition
set COMPILER=%SKYRIM%\Papyrus Compiler\PapyrusCompiler.exe
set FLAGS=%SKYRIM%\Data\source\Scripts\TESV_Papyrus_Flags.flg
set OUTPUT=%SKYRIM%\Data\Scripts
set IMPORTS=%SKYRIM%\Data\Scripts\Source;%SKYRIM%\Data\source\Scripts;%~dp0source

if not exist "%COMPILER%" (
    echo ERROR: PapyrusCompiler.exe not found at %COMPILER%
    exit /b 1
)

if "%~1"=="" (
    echo Compiling all scripts...
    for %%f in ("%~dp0source\MyPlugin_*.psc") do (
        echo   %%~nxf
        "%COMPILER%" "%%f" -f="%FLAGS%" -i="%IMPORTS%" -o="%OUTPUT%" -all
    )
) else (
    echo Compiling %1.psc...
    "%COMPILER%" "%~dp0source\%1.psc" -f="%FLAGS%" -i="%IMPORTS%" -o="%OUTPUT%" -all
)

echo Done.
```

Usage:
- Compile all: `scripts\papyrus\compile.bat`
- Compile one: `scripts\papyrus\compile.bat MyPlugin_MCM`

### Verification

After compilation, verify `.pex` timestamps updated:

```cmd
dir "{SKYRIM_DIR}\Data\Scripts\MyPlugin_*.pex"
```

If timestamps didn't change, compilation silently failed (usually an import path issue).

### Copy Back to Repo

```cmd
copy /Y "{SKYRIM_DIR}\Data\Scripts\MyPlugin_*.pex" scripts\papyrus\compiled\
```

## SkyUI SDK Setup (for MCM)

MCM scripts extend `SKI_ConfigBase` and use SkyUI types. The compiler needs the SkyUI source files on the import path.

### Required Files

Download from [SkyUI GitHub](https://github.com/schlangster/skyui) (`dist/Data/Scripts/Source/`):

```
SKI_ConfigBase.psc
SKI_ConfigManager.psc
SKI_PlayerLoadGameAlias.psc
SKI_QuestBase.psc
SKI_SettingsManager.psc
SKI_WidgetBase.psc
SKI_WidgetManager.psc
```

Place these in `{SKYRIM_DIR}\Data\Scripts\Source\` (alongside SKSE source files).

### Common Error

If you see `unknown type ski_configbase` during compilation, the SkyUI sources aren't on the import path. Verify the files exist and the import path includes their directory.

## Script Naming

Convention: prefix all scripts with your plugin's abbreviation to avoid collisions.

```
SCIE_MCM.psc              (not MCM.psc)
SCIE_NativeFunctions.psc  (not NativeFunctions.psc)
SCIE_ToggleScript.psc     (not ToggleScript.psc)
```

The compiled `.pex` filename must match the `ScriptName` declaration inside the `.psc` file exactly.

## Game Restart Requirement

Unlike DLL changes (which take effect on game launch), Papyrus scripts are loaded from the save game. After recompiling:

- **New scripts** (not in any save): Available immediately on next game load
- **Modified scripts** (already in a save): Requires game restart. The VM caches compiled scripts per session.

In practice: close the game, recompile, relaunch, load save.

## Distribution

In the Nexus archive, compiled scripts go at:
```
Scripts/
├── MyPlugin_MCM.pex
├── MyPlugin_NativeFunctions.pex
└── MyPlugin_SomeScript.pex
```

Papyrus sources are conventionally included (not required, but community norm):
```
SKSE/Plugins/MyPlugin/Source/
├── MyPlugin_MCM.psc
├── MyPlugin_NativeFunctions.psc
└── MyPlugin_SomeScript.psc
```

Or some authors place sources alongside compiled scripts in `Scripts/Source/`. Either convention works.
