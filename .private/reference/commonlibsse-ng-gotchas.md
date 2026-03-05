# CommonLibSSE-NG Gotchas

## Version Pinning

Always pin to a specific release tag:

```cmake
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG v3.7.0       # Pin this
    GIT_SHALLOW TRUE
)
```

Using `main` branch risks breaking changes mid-development. Update deliberately.

## Plugin Metadata

```cpp
v.UsesAddressLibrary();  // Version-independent address resolution
v.UsesNoStructs();       // Don't depend on game struct memory layouts
```

`UsesNoStructs()` tells SKSE your plugin won't break if game struct layouts change between updates. Use this unless you're doing raw memory manipulation with hardcoded offsets.

## Form Type Checking

Three patterns available:

```cpp
// As<T>() - returns typed pointer or nullptr (safest, most common)
auto* npc = form->As<RE::TESNPC>();

// Is(FormType) - boolean check only
if (form->Is(RE::FormType::NPC)) { ... }

// static_cast - when you've already verified the type
// Use when dynamic_cast is unavailable (some game types lack RTTI)
auto* ref = static_cast<RE::TESObjectREFR*>(alias->GetReference());
```

Prefer `As<T>()`. Use `static_cast` only when RTTI is known to be unavailable for that type (e.g., quest alias types).

## BSFixedString Lifetime

`BSFixedString` uses reference-counted interning. Strings persist as long as at least one `BSFixedString` holds them.

Gotcha: Converting to `std::string` and letting the `BSFixedString` go out of scope is fine. But holding a raw `const char*` from `BSFixedString::c_str()` after the last `BSFixedString` is destroyed is undefined behavior.

```cpp
// SAFE
std::string name = form->GetName();

// DANGEROUS - raw pointer may dangle
const char* name = form->GetName();
// ... if the BSFixedString backing this gets destroyed, name is garbage
```

## Thread Safety

Game functions are generally **not thread-safe**. If you need to call game functions from a non-game thread:

```cpp
// For general game operations
SKSE::GetTaskInterface()->AddTask([]() {
    // Safe to call game functions here
});

// For UI operations specifically
SKSE::GetTaskInterface()->AddUITask([]() {
    // Safe to modify UI state here
});
```

Papyrus native functions can be called from script threads. If your native function calls game APIs, use the task interface.

## Form Lookups

```cpp
// By FormID + plugin name (VR-safe, recommended)
auto* dh = RE::TESDataHandler::GetSingleton();
auto* form = dh->LookupForm<RE::TESGlobal>(0x809, "MyPlugin.esp");

// By FormID only (load-order dependent, less safe)
auto* form = RE::TESForm::LookupByID<RE::TESGlobal>(0x05000809);

// By EditorID (BROKEN ON VR - see vr-compatibility.md)
auto* form = RE::TESForm::LookupByEditorID<RE::TESGlobal>("MyGlobalVar");
```

Always use `TESDataHandler::LookupForm` for VR compatibility.

## Event Sinks

Pattern for subscribing to game events:

```cpp
class MyHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    static MyHandler* GetSingleton() {
        static MyHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
    {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;
        // Handle event
        return RE::BSEventNotifyControl::kContinue;  // Always continue
    }

private:
    MyHandler() = default;
};

// Register in kDataLoaded handler:
RE::UI::GetSingleton()->AddEventSink(MyHandler::GetSingleton());
```

Always return `kContinue` unless you specifically want to block downstream handlers.

## Initialization Order

Wait for `kDataLoaded` before:
- Looking up forms
- Registering event sinks
- Accessing `TESDataHandler`, `UI`, `ScriptEventSourceHolder`

The `SKSEPluginLoad` function runs too early for most game-state operations. Use it only for:
- Logging setup
- Messaging registration
- Cosave registration
- Hook installation
