# Mod Detection and Compatibility

## Why Detect Other Mods

SKSE plugins often need to:
- **Adapt behavior** when a compatible mod is present (use its factions, access its containers)
- **Warn users** about known incompatibilities
- **Show status** on a Compatibility MCM page

## Detection Methods

Three methods, each suited to different mod types:

### 1. ESP/ESM Name Lookup (Most Common)

For mods that have an ESP or ESM plugin file:

```cpp
bool IsModInstalled(const char* pluginName) {
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return false;
    return dataHandler->LookupModByName(pluginName) != nullptr;
}

// Examples
bool hasLOTD = IsModInstalled("LegacyoftheDragonborn.esm");
bool hasNFF = IsModInstalled("nwsFollowerFramework.esp");
```

This checks the active load order. If the plugin isn't loaded (missing or disabled), it returns false.

### 2. DLL Filesystem Check

For SKSE plugins that are DLL-only (no ESP):

```cpp
bool IsDLLInstalled(const char* dllName) {
    auto path = std::filesystem::path("Data/SKSE/Plugins") / dllName;
    return std::filesystem::exists(path);
}

// Examples
bool hasEssentialFavorites = IsDLLInstalled("po3_EssentialFavorites.dll");
bool hasFavoriteMiscItems = IsDLLInstalled("po3_FavoriteMiscItems.dll");
```

This detects presence even if the DLL failed to load. For stricter checks, you could verify the DLL is actually loaded via `GetModuleHandle`, but filesystem existence is usually sufficient.

### 3. FormID Lookup (Deep Integration)

When you need specific records from another mod (not just "is it loaded"):

```cpp
void InitializeModIntegration() {
    auto* dh = RE::TESDataHandler::GetSingleton();
    if (!dh) return;

    // Step 1: Check if the mod is loaded
    if (!dh->LookupModByName("nwsFollowerFramework.esp")) return;

    // Step 2: Look up specific forms we need
    m_nffFaction = dh->LookupForm<RE::TESFaction>(0x42702A, "nwsFollowerFramework.esp");
    m_nffQuest = dh->LookupForm<RE::TESQuest>(0x4220F4, "nwsFollowerFramework.esp");

    // Step 3: Validate — null out everything if partial failure
    if (m_nffFaction && m_nffQuest) {
        logger::info("NFF integration active: faction {:08X}, quest {:08X}",
            m_nffFaction->GetFormID(), m_nffQuest->GetFormID());
    } else {
        logger::warn("NFF loaded but forms not found, disabling integration");
        m_nffFaction = nullptr;
        m_nffQuest = nullptr;
    }
}
```

Key points:
- Always check `LookupModByName` first (avoids pointless lookups)
- Use `LookupForm<T>` with explicit type for safety
- **Null out everything on partial failure** — don't leave half-initialized state
- Log what was found for debugging

## Organization Pattern

### Header

```cpp
class ModDetection {
public:
    // Simple presence checks
    static bool IsLOTDInstalled();
    static bool IsGeneralStoresInstalled();
    static bool IsLinkedCraftingStorageInstalled();  // Incompatible

    // DLL-only mods
    static bool IsEssentialFavoritesInstalled();

    // Deep integration (resolved at init time)
    void InitializeIntegrations();
    RE::TESFaction* GetNFFStorageFaction() const { return m_nffFaction; }

private:
    RE::TESFaction* m_nffFaction = nullptr;
    RE::TESQuest* m_nffQuest = nullptr;
};
```

### Timing

- **Simple presence checks** (`Is*Installed`): Can run anytime after `kDataLoaded`
- **FormID lookups** (`InitializeIntegrations`): Run once during `kDataLoaded`, store results as member pointers
- **Don't re-check every frame** — mod load order doesn't change during a session

## Exposing to Papyrus (MCM Compatibility Page)

Wrap each detection as a native function:

```cpp
// C++ registration
bool IsLOTDInstalled(RE::StaticFunctionTag*) {
    return ModDetection::IsLOTDInstalled();
}

a_vm->RegisterFunction("IsLOTDInstalled"sv, className, IsLOTDInstalled);
```

```papyrus
; Papyrus declaration
bool Function IsLOTDInstalled() global native
```

## MCM Compatibility Page Pattern

Display detected mods grouped by category, with status indicators:

```papyrus
Function RenderCompatibilityPage()
    AddHeaderOption("$MYPLUGIN_HeaderSupportedMods")

    ; Compatible mod — show detection status
    bool hasLOTD = MyPlugin_NativeFunctions.IsLOTDInstalled()
    if hasLOTD
        AddTextOption("$MYPLUGIN_CompatLOTD", "$MYPLUGIN_Detected")
    else
        AddTextOption("$MYPLUGIN_CompatLOTD", "$MYPLUGIN_NotInstalled",
                      OPTION_FLAG_DISABLED)
    endif

    ; Feature-gated display — show if feature is enabled/disabled
    bool featureEnabled = MyPlugin_NativeFunctions.GetFeatureEnabled()
    if hasLOTD
        if featureEnabled
            AddTextOption("$MYPLUGIN_CompatLOTD", "$MYPLUGIN_Detected")
        else
            AddTextOption("$MYPLUGIN_CompatLOTD", "$MYPLUGIN_DetectedFeatureOff")
        endif
    endif

    ; Incompatible mod — show warning
    AddHeaderOption("$MYPLUGIN_HeaderIncompatible")
    bool hasConflict = MyPlugin_NativeFunctions.IsConflictingModInstalled()
    if hasConflict
        AddTextOption("$MYPLUGIN_CompatConflict", "$MYPLUGIN_Conflict")
        AddTextOption("$MYPLUGIN_WarnChooseOne", "", OPTION_FLAG_DISABLED)
    else
        AddTextOption("$MYPLUGIN_NoConflicts", "", OPTION_FLAG_DISABLED)
    endif
EndFunction
```

### Display Conventions

| Status | Meaning | Visual |
|--------|---------|--------|
| Detected | Mod found, integration active | Normal text |
| Detected (feature off) | Mod found but related feature disabled in MCM | Normal text, hint to enable |
| Not Installed | Mod not in load order | Greyed out (`OPTION_FLAG_DISABLED`) |
| Conflict | Incompatible mod detected | Warning header, explanation text |
| Always Supported | Feature works without any additional mod | Static info text |

## Conditional Behavior Pattern

When adapting behavior based on detected mods:

```cpp
void ScanForSources() {
    // Always check vanilla factions
    if (actor->IsInFaction(m_playerHorseFaction)) {
        AddSource(actor);
    }

    // Conditional: only check CH faction if CH is loaded
    if (m_chHorseFaction && actor->IsInFaction(m_chHorseFaction)) {
        AddSource(actor);
    }
}
```

The null check on `m_chHorseFaction` is the gate. If Convenient Horses isn't installed, the pointer is null (never resolved), and the branch is never taken. No runtime overhead for uninstalled mods.

## Documenting Compatibility

In your CLAUDE.md or Architecture.md, maintain a compatibility table:

```markdown
| Mod | Detection | Type | Notes |
|-----|-----------|------|-------|
| LOTD | LookupModByName("LegacyoftheDragonborn.esm") | Compatible | Global containers |
| NFF | LookupModByName + LookupForm (faction, quest) | Compatible | Additional Inventory |
| Essential Favorites | Filesystem (po3_EssentialFavorites.dll) | Compatible | Favorited items excluded |
| Linked Crafting Storage | LookupModByName("LinkedCraftingStorage.esp") | Incompatible | Both intercept crafting |
```

This table serves double duty: it tells the AI assistant what's detected and how, and it documents the compatibility story for the mod page.
