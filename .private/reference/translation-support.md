# Translation Support

## Overview

Skyrim mods can be fully translated using SkyUI's translation file system. A single file provides translations for MCM strings (via SkyUI's `$KEY` syntax) and DLL/Papyrus notification strings (via a custom `TranslationService` singleton that reads the same file).

## Translation File Format

### Location and Naming

```
Data/Interface/Translations/{ESPName}_LANGUAGE.txt
```

The filename must match your ESP name exactly (without `.esp`). SKSE loads translation files based on the ESP name, not the DLL name.

Examples:
- `CraftingInventoryExtender_ENGLISH.txt`
- `CraftingInventoryExtender_FRENCH.txt`
- `CraftingInventoryExtender_RUSSIAN.txt`

### Encoding

**UTF-16 LE with BOM** (byte order mark). This is mandatory -- SKSE and SkyUI will not read UTF-8 translation files.

In most editors, save as "UTF-16 LE" or "UCS-2 LE BOM". Notepad++ and VS Code both support this.

### Content Format

Tab-separated key-value pairs, one per line:

```
$MYPLUGIN_MsgEnabled	Enabled
$MYPLUGIN_MsgDisabled	Disabled
$MYPLUGIN_LabelVersion	Version
$MYPLUGIN_HeaderSettings	Settings
```

- Keys start with `$` and use SCREAMING_SNAKE_CASE by convention
- A single tab character separates key from value
- No quotes around values
- Blank lines and lines without a tab are ignored
- Prefix keys with your mod's abbreviation to avoid collisions (e.g., `$SCIE_`, `$MYPLUGIN_`)

## MCM Integration (SkyUI)

SkyUI's MCM automatically resolves `$KEY` references in option labels and headers.

### Static Keys

```papyrus
AddHeaderOption("$MYPLUGIN_HeaderSettings")           ; Resolved by SkyUI from translation file
AddToggleOption("$MYPLUGIN_ToggleEnabled", isEnabled)  ; Label from translation, value from variable
AddTextOption("$MYPLUGIN_LabelVersion", "1.0.0")       ; Label translated, value is literal
```

### Dynamic Keys with Nested Syntax

SkyUI supports `$KEY{value}` for dynamic strings where the value is embedded in the translation:

Translation file:
```
$MYPLUGIN_ContainerCount	Tracked Containers: {0}
```

Papyrus:
```papyrus
; SkyUI resolves $MYPLUGIN_ContainerCount and substitutes the nested value
AddTextOption("$MYPLUGIN_ContainerCount", containerCount)
```

This works because SkyUI evaluates `$KEY{value}` by looking up `$KEY` and inserting `value` at the placeholder position.

Limitation: SkyUI's nested syntax only supports a single substitution per key. For multiple arguments, use the DLL-side `TranslateFormat` approach below.

## DLL-Side Translation

For notification messages, toggle feedback, and other strings generated in C++, implement a `TranslationService` singleton that reads the same translation file.

### Language Detection

Auto-detect the game's language from `Skyrim.ini`:

```cpp
std::string DetectLanguage() {
    // Read sLanguage from [General] section of Skyrim.ini
    auto setting = RE::INISettingCollection::GetSingleton()->GetSetting("sLanguage:General");
    if (setting) {
        return setting->GetString();
    }
    return "ENGLISH";  // Fallback
}
```

### File Loading

```cpp
void TranslationService::Load() {
    auto language = DetectLanguage();

    // Try detected language first, fall back to ENGLISH
    auto path = fmt::format("Data/Interface/Translations/{}_{}.txt", espName, language);
    if (!std::filesystem::exists(path)) {
        path = fmt::format("Data/Interface/Translations/{}_ENGLISH.txt", espName);
    }

    // Read UTF-16 LE file
    std::wifstream file(path, std::ios::binary);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));

    // Skip BOM
    wchar_t bom;
    file.get(bom);

    // Parse tab-separated key=value pairs into map
    std::wstring line;
    while (std::getline(file, line)) {
        auto tabPos = line.find(L'\t');
        if (tabPos == std::wstring::npos) continue;

        auto key = WideToUtf8(line.substr(0, tabPos));
        auto value = WideToUtf8(line.substr(tabPos + 1));
        translations[key] = value;
    }
}
```

### Lookup

```cpp
std::string TranslationService::Translate(const std::string& key) {
    auto it = translations.find(key);
    if (it != translations.end()) return it->second;
    return key;  // Return key as-is if not found (visible debugging)
}
```

### Positional Placeholders

For strings with multiple arguments, use `{0}`, `{1}`, `{2}` positional placeholders. This allows translators to reorder arguments to match their language's grammar.

Translation file:
```
$MYPLUGIN_MsgTransferred	Transferred {0} {1} from {2}
```

C++ implementation:
```cpp
std::string TranslationService::TranslateFormat(const std::string& key,
    const std::string& arg0, const std::string& arg1, const std::string& arg2)
{
    auto result = Translate(key);
    // Replace positional placeholders
    if (!arg0.empty()) ReplaceAll(result, "{0}", arg0);
    if (!arg1.empty()) ReplaceAll(result, "{1}", arg1);
    if (!arg2.empty()) ReplaceAll(result, "{2}", arg2);
    return result;
}
```

### Papyrus API

Expose translation to Papyrus for notification scripts:

```papyrus
; In your native functions script:
string Function Translate(string asKey) global native
string Function TranslateFormat(string asKey, string arg0, string arg1, string arg2) global native
```

Usage in Papyrus:
```papyrus
Debug.Notification(MYPLUGIN_NativeFunctions.Translate("$MYPLUGIN_MsgEnabled"))
```

## What NOT to Translate

**ESP strings** (power names, spell descriptions, quest names) remain in English unless you implement full STRINGS/ILSTRINGS/DLSTRINGS localization. This requires:
- Setting the Localized flag in the ESP header
- Converting all string fields to string table indices
- Generating language-specific `.STRINGS`, `.ILSTRINGS`, `.DLSTRINGS` files

This is disproportionate tooling effort for a handful of short strings. The `$KEY` system covers MCM and notifications, which are the bulk of user-visible text.

## Community Translation Workflow

1. Ship `{ESPName}_ENGLISH.txt` as the canonical file
2. Translators copy it, rename to their language, translate the values
3. Translations can be distributed as separate Nexus files or bundled
4. The game auto-detects language -- no user configuration needed

## Checklist

- [ ] Translation file is UTF-16 LE with BOM
- [ ] Filename matches ESP name exactly
- [ ] All MCM strings use `$KEY` references (no hardcoded English in Papyrus)
- [ ] All DLL notification strings go through `TranslationService`
- [ ] Positional placeholders use `{0}/{1}/{2}` (not `%s` -- allows reordering)
- [ ] Fallback to ENGLISH if detected language file not found
- [ ] Keys prefixed with mod abbreviation to avoid collisions
