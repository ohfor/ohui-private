# Packaging for Nexus Mods

## Archive Structure

The most critical rule: **NO `Data/` folder at the archive root.**

Mod managers (MO2, Vortex) expect archive contents at root level. They map the archive root to Skyrim's `Data/` folder automatically. If your archive has a `Data/` wrapper, MO2 will prompt the user to "set data directory" and Vortex may install incorrectly.

### Correct Structure

```
MyPlugin.zip/
├── SKSE/
│   └── Plugins/
│       ├── MyPlugin.dll
│       └── MyPlugin.ini              (optional)
├── Scripts/
│   └── *.pex                         (if Papyrus scripts)
├── Interface/
│   └── Translations/
│       └── MyPlugin_ENGLISH.txt      (if translated)
├── MyPlugin.esp                      (if ESP)
└── LICENSE.txt
```

### Wrong Structure

```
MyPlugin.zip/
└── Data/                             WRONG
    ├── SKSE/
    └── ...
```

## Verification Checklist

Before uploading to Nexus:

- [ ] Archive root has no `Data/` folder
- [ ] DLL is in `SKSE/Plugins/` at root
- [ ] ESP (if any) is at root
- [ ] Scripts (if any) are in `Scripts/` at root
- [ ] Version numbers match across all source files
- [ ] DLL was built in Release configuration
- [ ] Tested fresh install via mod manager (no "set data directory" prompt)

## Multi-Package Distribution

For mods with optional components, distribute as separate archives:

| Package | Contents | Versioned? |
|---------|----------|------------|
| Main | Core DLL, ESP, scripts, default configs | Yes: `MyPlugin-Main-vX.Y.Z.zip` |
| Optional | Extra configs, patches, addons | Depends on content |

## Build Script Pattern

A PowerShell distribution script should:

1. **Validate version consistency** across all source files (fail if mismatch)
2. **Rebuild DLL** (cmake --build)
3. **Recompile Papyrus** scripts (if applicable)
4. **Stage files** to a clean output directory (no stale artifacts)
5. **Create versioned zip** (e.g., `MyPlugin-vX.Y.Z.zip`)
6. **Verify archive structure** (no `Data/` at root)
7. **Archive old versions** (move previous zip to `archive/` subfolder)

Use smart rebuild detection: compare newest source file timestamp to existing zip timestamp. Skip rebuild if nothing changed (unless `-Force` flag).

## Common Mistakes

| Mistake | Symptom | Fix |
|---------|---------|-----|
| `Data/` wrapper | MO2 asks to set data directory | Fix archive paths |
| Missing PEX files | MCM doesn't work | Compile Papyrus scripts |
| Wrong DLL config | Debug DLL is huge, may crash | Build with `--config Release` |
| Stale staging dir | Old files included | Delete staging folder before rebuild |
| Version mismatch | DLL reports wrong version | Update all 4 version files |
