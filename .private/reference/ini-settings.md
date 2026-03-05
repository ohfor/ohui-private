# INI Settings

## File Location

SKSE plugins conventionally store settings at:
```
Data/SKSE/Plugins/{PluginName}.ini
```

This path is relative to the game root. It's the same folder your DLL lives in.

## Two INI Patterns

Most SKSE plugins need one or both of these:

| Pattern | Use Case | Example |
|---------|----------|---------|
| **Single settings file** | User preferences, toggles, thresholds | `MyPlugin.ini` |
| **Config directory** | Extensible data (container lists, patch configs, per-mod presets) | `MyPlugin/*.ini` |

The config directory pattern is important when your plugin ships default configs and users (or other mod authors) need to add or override entries without editing your files. SimpleIni operates on a single file and doesn't support this multi-file-with-overrides workflow — this is the main reason to use a custom parser.

## Approach: Custom Parser

A lightweight hand-rolled parser gives full control and avoids external dependencies. Required if you need the config directory pattern.

### Reading

```cpp
void LoadSettings() {
    std::ifstream file("Data/SKSE/Plugins/MyPlugin.ini");
    if (!file.is_open()) {
        logger::info("No INI file found, using defaults");
        return;
    }

    std::string line, currentSection;
    while (std::getline(file, line)) {
        // Trim whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Skip comments
        if (line[0] == ';' || line[0] == '#') continue;

        // Section header
        if (line[0] == '[') {
            auto close = line.find(']');
            if (close != std::string::npos) {
                currentSection = line.substr(1, close - 1);
                std::transform(currentSection.begin(), currentSection.end(),
                               currentSection.begin(), ::tolower);
            }
            continue;
        }

        // Key=Value
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Strip inline comments
        auto comment = value.find(';');
        if (comment != std::string::npos) value = value.substr(0, comment);

        // Trim both
        // ... (trim whitespace from key and value)

        // Match settings (case-insensitive key comparison)
        if (currentSection == "general") {
            if (key == "bEnabled") m_enabled = ParseBool(value, true);
            if (key == "fDistance") m_distance = ParseFloat(value, 3000.0f);
        }
    }
}
```

### Parsing Helpers

```cpp
bool ParseBool(const std::string& value, bool fallback) {
    if (value.empty()) return fallback;
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1" || lower == "yes") return true;
    if (lower == "false" || lower == "0" || lower == "no") return false;
    return fallback;
}

float ParseFloat(const std::string& value, float fallback) {
    try { return std::stof(value); }
    catch (...) { return fallback; }
}

int32_t ParseInt(const std::string& value, int32_t fallback) {
    try { return std::stoi(value); }
    catch (...) { return fallback; }
}
```

### Writing

Regenerate the entire file on save. This is simpler than in-place editing and ensures consistent formatting:

```cpp
void SaveSettings() {
    std::ofstream file("Data/SKSE/Plugins/MyPlugin.ini");
    if (!file.is_open()) {
        logger::error("Failed to open INI for writing");
        return;
    }

    file << "; MyPlugin Configuration\n\n";
    file << "[General]\n";
    file << "bEnabled=" << (m_enabled ? "true" : "false") << "\n";
    file << "fDistance=" << std::fixed << std::setprecision(1) << m_distance << "\n";
    file << "\n";
    file << "[Debug]\n";
    file << "bDebugLogging=" << (m_debugLogging ? "true" : "false") << "\n";
}
```

## Config Directory Pattern (Multi-File with User Overrides)

When your plugin ships default data in INI files (e.g., container lists per player home, patch configs per supported mod), users need a way to add entries or override your defaults without editing your shipped files. This is the same problem Bethesda solves with plugin load order — later files override earlier ones.

### Structure

```
Data/SKSE/Plugins/MyPlugin/
├── MyPlugin_DefaultConfig.ini     (shipped with mod)
├── MyPlugin_SomeHomeMod.ini       (shipped with mod)
└── MyPlugin_ZZZ_UserOverrides.ini (user-created, loads last)
```

### Loading Order

Sort INI files alphabetically before processing. This gives users a deterministic override mechanism — a file named `ZZZ_Overrides.ini` always loads after your shipped configs:

```cpp
void LoadConfigDirectory() {
    auto configDir = std::filesystem::path("Data/SKSE/Plugins/MyPlugin");
    if (!std::filesystem::exists(configDir)) return;

    // Collect and sort alphabetically for deterministic load order
    std::vector<std::filesystem::path> iniFiles;
    for (const auto& entry : std::filesystem::directory_iterator(configDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ini") {
            iniFiles.push_back(entry.path());
        }
    }
    std::sort(iniFiles.begin(), iniFiles.end());

    for (const auto& path : iniFiles) {
        ParseConfigFile(path);
    }
}
```

### Override Semantics

The critical design choice: `=false` must actively remove entries added by earlier files, not just skip them. Without this, users cannot disable entries from your shipped configs.

```cpp
// Inside ParseConfigFile:
bool enabled = (valueLower == "true" || valueLower == "1");
bool disabled = (valueLower == "false" || valueLower == "0");

if (!enabled && !disabled) continue;  // Unrecognized value

auto formID = ResolveEntry(key);
if (!formID) continue;

if (enabled) {
    m_entries.insert(*formID);
    logger::debug("Entry added: {:08X}", *formID);
} else {
    // =false REMOVES an entry added by an earlier INI file
    if (m_entries.erase(*formID)) {
        logger::info("Entry removed by override: {:08X}", *formID);
    }
}
```

### Why Not SimpleIni Here

SimpleIni operates on a single file. The config directory pattern requires:
1. Iterating multiple files in sorted order
2. Accumulating entries across files
3. Allowing later files to negate earlier entries

These are fundamentally multi-file semantics that don't map to SimpleIni's single-file API.

## Approach: SimpleIni (External Library)

[SimpleIni](https://github.com/brofield/simpleini) is a popular single-header library used by many SKSE plugins. It handles parsing, writing, and preserving comments/ordering. Good for the single settings file case.

### Setup

SimpleIni is header-only. Either drop `SimpleIni.h` into your `include/` directory, or add to vcpkg:

```json
"dependencies": ["simpleini"]
```

### Usage

```cpp
#include <SimpleIni.h>

CSimpleIniA ini;
ini.SetUnicode();
ini.LoadFile("Data/SKSE/Plugins/MyPlugin.ini");

// Reading
bool enabled = ini.GetBoolValue("General", "bEnabled", true);
float distance = static_cast<float>(ini.GetDoubleValue("General", "fDistance", 3000.0));
const char* name = ini.GetValue("General", "sName", "default");

// Writing
ini.SetBoolValue("General", "bEnabled", false);
ini.SaveFile("Data/SKSE/Plugins/MyPlugin.ini");
```

SimpleIni preserves comments and section ordering on save. The custom approach above does not (it regenerates the file).

## Thread Safety

If settings can be modified at runtime (e.g., via MCM), protect reads/writes with a mutex:

```cpp
std::mutex m_settingsMutex;

bool GetEnabled() {
    std::lock_guard lock(m_settingsMutex);
    return m_enabled;
}

void SetEnabled(bool value) {
    std::lock_guard lock(m_settingsMutex);
    m_enabled = value;
    Save();  // Persist immediately
}
```

## Naming Conventions

Bethesda-style INI keys use Hungarian notation prefixes:
- `b` = bool (`bEnabled`, `bDebugLogging`)
- `f` = float (`fMaxDistance`, `fScale`)
- `i` = integer (`iMaxCount`)
- `s` = string (`sLanguage`)

This isn't required but matches what users expect from Skyrim INI files.

## When to Save

- **On MCM change**: Save immediately when the user changes a setting via MCM
- **On game save**: Not necessary if you save on change (INI is independent of save games)
- **Never on load**: INI is read-only at load time; the user or MCM writes it

Don't conflate INI settings with cosave data. INI stores user preferences (persistent across saves). Cosave stores per-save state (container toggles, quest progress).
