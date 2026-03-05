# Hooking Patterns for SKSE Plugins

## MinHook Function Detours

MinHook is the most flexible approach for intercepting game functions.

### Setup

Add to `vcpkg.json`:
```json
"dependencies": ["minhook"]
```

Add to `CMakeLists.txt`:
```cmake
find_package(minhook CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE minhook::minhook)
```

### Basic Pattern

```cpp
#include <MinHook.h>

// Original function pointer (trampoline)
using MyFunc_t = int32_t(*)(RE::TESObjectREFR*, bool);
MyFunc_t _originalMyFunc = nullptr;

// Detour
int32_t Hook_MyFunc(RE::TESObjectREFR* a_ref, bool a_flag) {
    // Your logic here
    return _originalMyFunc(a_ref, a_flag);  // Call original
}

bool InstallHooks() {
    if (MH_Initialize() != MH_OK) return false;

    // Resolve address via Address Library
    auto addr = REL::VariantID(seID, aeID, vrOffset).address();

    if (MH_CreateHook(reinterpret_cast<void*>(addr),
                      reinterpret_cast<void*>(&Hook_MyFunc),
                      reinterpret_cast<void**>(&_originalMyFunc)) != MH_OK) {
        return false;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return false;

    return true;
}
```

### Address Resolution

- **SE + AE**: `REL::RelocationID(seID, aeID)` -- uses Address Library CSV
- **SE + AE + VR**: `REL::VariantID(seID, aeID, vrOffset)` -- hardcoded VR offset
- **VR offsets**: Look up in the [skyrim_vr_address_library database](https://github.com/alandtse/skyrim_vr_address_library). Some IDs are missing from the shipped VR Address Library CSV -- hardcoding is the workaround.

Use `REL::VariantID` by default if you support VR. It's a superset of `REL::RelocationID`.

## VTable Hooks

For virtual function overrides (e.g., `RemoveItem`):

```cpp
// Define your replacement
static RE::ObjectRefHandle* Hook_RemoveItem(RE::TESObjectREFR* a_this, ...) {
    // Your logic
    return _originalRemoveItem(a_this, ...);
}

// Install via stl::write_vfunc
void InstallVTableHook() {
    stl::write_vfunc<RE::TESObjectREFR, 0x56>(Hook_RemoveItem);
}
```

VTable offsets are consistent across SE/AE/VR runtimes (per CommonLibSSE-NG).

## ABI Gotchas

### Hidden Result Pointer (x64 MSVC struct return)

When a function returns a struct by value on x64 MSVC, the compiler inserts a **hidden first parameter** (after `this`) that is a pointer to the return value. The actual function signature differs from what the source code suggests.

Example -- `RemoveItem` returns `ObjectRefHandle` by value:

```cpp
// What the source code looks like:
ObjectRefHandle RemoveItem(TESBoundObject* item, int32_t count, ...);

// What the actual calling convention is:
ObjectRefHandle* RemoveItem(TESObjectREFR* this, ObjectRefHandle* result,
    TESBoundObject* item, int32_t count, ...);
```

If your hook's parameter order is wrong, you'll get garbage values and crashes. Always verify against the actual disassembly or known good signatures.

### Identifying This Pattern

If a function returns a type larger than 8 bytes (or a non-trivial struct), suspect a hidden result pointer. Check IDA/Ghidra for `lea rcx` or `mov rcx, rsp+XX` before the call.

## Hook Installation Timing

Install hooks in `SKSEPluginLoad` (before `kDataLoaded`). Register event handlers in `kDataLoaded`.

**Never** return `false` from `SKSEPluginLoad` if hooks fail. This shows a scary SKSE error dialog. Instead:

```cpp
if (!MyHooks::Install()) {
    logger::critical("{} DISABLED - hooks failed", Version::NAME);
    return true;  // Graceful degradation
}
```

## State Management in Hooks

### Lazy Initialization

Game events don't always fire in the order you expect. Menu open events may fire AFTER the first inventory query. Initialize state lazily in the hook itself:

```cpp
bool EnsureSessionActive() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto furnitureHandle = player->GetOccupiedFurniture();
    if (furnitureHandle.native_handle() == 0) {
        if (g_session.active) g_session.Reset();
        return false;
    }
    if (g_session.active) return true;  // Already initialized
    // Initialize now
    g_session.active = true;
    // ... populate state
    return true;
}
```

### Loading Menu Guard

Save/load invalidates all container and form references. Guard every hook that accesses game state:

```cpp
auto* ui = RE::UI::GetSingleton();
if (!ui || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
    if (g_session.active) {
        logger::info("Loading detected, ending session");
        g_session.Reset();
    }
    return originalFunc(args...);
}
```

Without this, you get heap corruption from stale pointers that manifests as CTD on the next save reload.

## Anti-Patterns (Known Dead Ends)

These approaches have been tried and don't work. Save yourself the time. Note: these are specific to crafting inventory interception -- your domain will have different dead ends. The pattern of documenting them is what matters.

- **Entry-point hooks on GetItemCount** (Address Library IDs 16565/50372, 19275/19701): Either never called during crafting, or CTD immediately.
- **Call-site hooks in crafting functions**: Internal structures use indices and offsets, not TESForm pointers. You can't intercept at the call site meaningfully.
- **Most ConstructibleObjectMenu VTable entries**: Stubs or early-exit functions. The menu doesn't expose useful virtual functions for interception.
- **Hooking `GetItemCount` condition evaluation**: The code path for COBJ recipe conditions is different from the inventory query path. Hooks on one don't affect the other.

## Performance Considerations

Hooks on inventory query functions can be called hundreds of times per frame during menu rendering. Keep hot-path hooks lean:

- Cache results aggressively (invalidate on item add/remove)
- Avoid allocations in the hook body
- Move complex logic (container scanning, filtering) to session initialization
- Profile with `std::chrono::high_resolution_clock` scope guards
