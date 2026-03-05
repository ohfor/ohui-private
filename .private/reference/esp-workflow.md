# ESP Workflow

## When You Need an ESP

An ESP plugin file is needed when your mod adds:
- **Spells/Powers** (lesser powers for activation, toggle powers)
- **Magic Effects** (MGEF records with attached Papyrus scripts)
- **Keywords** (for script properties, faction checks, etc.)
- **Globals** (GLOB records for cross-system communication, e.g., "is crafting active")
- **Quests** (for MCM, script aliases, initialization)
- **Shader Effects** (EFSH records for visual feedback)
- **Factions** (for grouping, detection)

If your plugin is pure C++ (hooks only, no game objects), you may not need an ESP at all.

## ESL Flag

Always ESL-flag your ESP unless you have a specific reason not to. ESL-flagged plugins don't count toward the 255 plugin limit.

Requirements for ESL:
- FormIDs must fit in the `FE XXX YYY` range (max 4096 new records)
- Set the ESL flag in the file header

Most SKSE plugin ESPs easily fit within the 4096 record limit.

**VR note**: VR doesn't natively support ESL. VR users need [Skyrim VR ESL Support](https://www.nexusmods.com/skyrimspecialedition/mods/106712). Document this requirement if your ESP is ESL-flagged.

## Creation and Editing

### xEdit (Primary Tool)

[SSEEdit](https://www.nexusmods.com/skyrimspecialedition/mods/164) is the standard tool for creating and editing ESP files. Use it for:

- Creating new records (spells, keywords, globals, quests)
- Editing existing record properties
- Attaching scripts to magic effects (VMAD records)
- Setting ESL flag
- Inspecting load order conflicts

### Script Attachment via xEdit

Attaching a Papyrus script to a Magic Effect (MGEF) in xEdit:

1. Open your ESP in SSEEdit
2. Navigate to the MGEF record
3. Right-click the record → Add → VMAD (Virtual Machine Adapter)
4. Under VMAD → Scripts → Add script entry
5. Set script name (must match `.psc` ScriptName exactly)
6. Add properties: right-click script → Add Property
7. Set property type, name, and value (FormID references use the full load-order FormID)

This is a manual process. Document the exact steps and FormIDs for your ESP so it's reproducible.

### Programmatic ESP Creation

For patches that follow a pattern (e.g., duplicating vanilla recipes with modified conditions), a Python script can generate the ESP binary directly. This is worthwhile when:

- You're creating many similar records (10+ duplicate recipes)
- The records follow a mechanical pattern
- Manual xEdit work would be error-prone or tedious

The ESP binary format is documented at [UESP](https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format). Libraries exist for reading/writing it, or you can write the binary directly for simple cases.

### What Goes in the Repo

The ESP itself (binary file) is tracked in the repo. It's a build artifact of xEdit work, not a compiled output. When you make changes in xEdit:

1. Save in xEdit
2. Copy the updated ESP from the game's Data directory to your repo
3. Commit the ESP

ESPs are typically small (< 100KB for SKSE plugin support files), so tracking the binary is fine.

## Repository Structure

```
Data/
├── MyPlugin.esp                    Main ESP (tracked in git)
└── MyPluginPatch.esp               Optional patch ESP (tracked in git)
```

The `.gitignore` should exclude ESPs at the repo root (dropped in for analysis from other mods) but allow ESPs under `Data/`:

```gitignore
*.esp
!Data/*.esp
```

## Common ESP Patterns for SKSE Plugins

### MCM Quest

A quest record that hosts the MCM script. The quest must:
- Start game enabled
- Run once
- Have a player alias (for `OnPlayerLoadGame` event)

The MCM script extends `SKI_ConfigBase` and registers pages/options.

### Toggle Power

A lesser power spell → magic effect → Papyrus script chain:

```
SPEL (Lesser Power) → MGEF (Script Effect) → Papyrus Script
```

The script receives an `OnEffectStart` event with the caster and target.

### Global Variables

GLOB records for communication between C++ and Papyrus, or for COBJ recipe conditions:

```
SCIE_CraftingActive (GLOB, Short, default 0)
  → Set to 1 by C++ when crafting with external sources
  → Checked by COBJ conditions: GetGlobalValue >= 1
```

### Shaders

EFSH records for visual feedback (container highlighting, activation confirmation). Reference these by FormID from C++ via `TESDataHandler::LookupForm`.

## FormID Management

Keep a reference table of your ESP's FormIDs. You'll need these for:
- C++ `LookupForm` calls
- xEdit script property assignment
- INI config files
- Documentation

Example:

```
0x800  KYWD  MyPlugin_SomeKeyword
0x801  MGEF  MyPlugin_ToggleEffect
0x802  SPEL  MyPlugin_TogglePower
0x803  QUST  MyPlugin_MCMQuest
0x805  EFSH  MyPlugin_EnabledShader
0x807  EFSH  MyPlugin_DisabledShader
0x809  GLOB  MyPlugin_CraftingActive
```

Document these in your Architecture.md or CLAUDE.md for quick reference.
