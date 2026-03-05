# Papyrus Native Functions

## Overview

SKSE lets you expose C++ functions to Papyrus scripts. This is how MCM toggles, status queries, and other script-accessible features work.

## Declaration (Papyrus Side)

```papyrus
ScriptName MyPlugin_NativeFunctions

; Native functions are declared with the 'native' keyword
; The 'global' keyword means they don't need an object instance
bool Function GetEnabled() global native
Function SetEnabled(bool abEnabled) global native
string Function GetVersion() global native
```

## Registration (C++ Side)

Register during `kDataLoaded`:

```cpp
namespace Papyrus {
    // Function implementations
    bool GetEnabled(RE::StaticFunctionTag*) {
        return MySettings::GetSingleton()->IsEnabled();
    }

    void SetEnabled(RE::StaticFunctionTag*, bool a_enabled) {
        MySettings::GetSingleton()->SetEnabled(a_enabled);
    }

    std::string GetVersion(RE::StaticFunctionTag*) {
        return fmt::format("{}.{}.{}", Version::MAJOR, Version::MINOR, Version::PATCH);
    }

    // Registration function
    bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm) {
        const auto className = "MyPlugin_NativeFunctions"sv;

        a_vm->RegisterFunction("GetEnabled"sv, className, GetEnabled);
        a_vm->RegisterFunction("SetEnabled"sv, className, SetEnabled);
        a_vm->RegisterFunction("GetVersion"sv, className, GetVersion);

        logger::info("Registered {} Papyrus native functions", 3);
        return true;
    }
}

// In kDataLoaded handler:
if (auto* papyrus = SKSE::GetPapyrusInterface()) {
    papyrus->Register(Papyrus::RegisterFunctions);
}
```

## Type Marshalling

| Papyrus Type | C++ Type |
|-------------|----------|
| `bool` | `bool` |
| `int` | `int32_t` |
| `float` | `float` |
| `string` | `std::string` or `RE::BSFixedString` |
| `ObjectReference` | `RE::TESObjectREFR*` |
| `Form` | `RE::TESForm*` |
| `Actor` | `RE::Actor*` |
| `int[]` | `RE::BSScript::VMArray<int32_t>` (see below) |

The first parameter is always `RE::StaticFunctionTag*` for global functions or the script's attached object type for member functions. This parameter is not visible on the Papyrus side.

## Array Parameters

```cpp
// Papyrus: int[] Function GetFormIDs() global native
std::vector<int32_t> GetFormIDs(RE::StaticFunctionTag*) {
    std::vector<int32_t> result;
    // ... populate ...
    return result;
}

// Papyrus: Function ProcessItems(int[] aiFormIDs) global native
void ProcessItems(RE::StaticFunctionTag*, std::vector<int32_t> a_formIDs) {
    for (auto id : a_formIDs) {
        // ...
    }
}
```

## Threading

Papyrus functions run on script threads, not the game's main thread. If your native function needs to call game APIs:

```cpp
void DoSomethingUnsafe(RE::StaticFunctionTag*) {
    // This runs on a Papyrus script thread
    SKSE::GetTaskInterface()->AddTask([]() {
        // This runs on the game thread - safe to call game APIs
        auto* player = RE::PlayerCharacter::GetSingleton();
        player->AddSpell(someSpell);
    });
}
```

Simple reads (checking a bool, reading a setting) are generally safe without the task interface. Writes to game state should go through `AddTask`.

## Script Name Must Match

The Papyrus script name (`ScriptName MyPlugin_NativeFunctions`) must exactly match the class name used in `RegisterFunction`. The compiled `.pex` filename must also match. Mismatches silently fail -- functions will appear to not exist.

## Compilation

Papyrus sources go in `scripts/papyrus/source/`. Compile with the Creation Kit's Papyrus compiler:

```cmd
PapyrusCompiler.exe "MyPlugin_NativeFunctions.psc" -f="TESV_Papyrus_Flags.flg" -i="path/to/imports" -o="output/dir"
```

Import paths must include vanilla script sources and (if using MCM) SkyUI SDK sources.
