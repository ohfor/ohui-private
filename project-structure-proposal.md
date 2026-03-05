# OHUI — Project Structure & Test Infrastructure Proposal

This document proposes the directory restructuring, source module layout,
test infrastructure, conventions, and mock boundary layer needed before
Phase 0 tasks begin executing.

---

## 1. Directory Restructuring

### Moves

| Current Location | New Location | Reason |
|---|---|---|
| `screens/` | `.private/screens/` | Design docs, not public content |
| `systems/` | `.private/systems/` | Design docs, not public content |
| `scripts/` | `.private/tools/` | Internal build/deploy tooling |

### Updated CLAUDE.md references

Every reference to `screens/`, `systems/`, `scripts/` in CLAUDE.md,
TASKS.md, and DESIGN.md must be updated to the new paths. The
`deploy.ps1` and `build_package.ps1` self-locate via `$PSScriptRoot`
so they'll work from `.private/tools/` without code changes. The
CMake post-build step and `check_versions.py` need their paths
updated.

### After restructuring

```
ohui-private/
├── .claude/
├── .github/workflows/
├── .private/
│   ├── decisions/
│   ├── notes/
│   ├── reference/              ← template-owned guides
│   ├── reference-overrides/    ← project divergences
│   ├── research/
│   ├── screens/                ← 17 screen design docs (MOVED)
│   ├── systems/                ← 16 system architecture docs (MOVED)
│   └── tools/                  ← build, deploy, utility scripts (MOVED)
│       ├── deploy.ps1
│       ├── build_package.ps1
│       └── check_versions.py
├── assets/
├── dist/
├── docs/
│   ├── Architecture.md
│   ├── Build.md
│   └── CHANGELOG.md
├── include/
│   └── ohui/                   ← NEW (see §2)
├── src/                        ← NEW module structure (see §2)
├── tests/                      ← NEW (see §3)
├── CLAUDE.md
├── CMakeLists.txt
├── CMakePresets.json
├── compatibility.md
├── DESIGN.md
├── LICENSE
├── mod-compatibility-checklist.md
├── TASKS.md
├── vcpkg.json
└── version.rc
```

---

## 2. Source Module Layout

### Principles

Every module maps to a phase or major subsystem in TASKS.md. Headers
and sources mirror each other. A task knows exactly where its code
lands by looking at this map. No task creates new top-level
directories in `src/` without amending this document.

### Header structure: `include/ohui/`

```
include/
├── PCH.h
├── Version.h
└── ohui/
    ├── core/
    │   ├── GameBridge.h        ← abstract interface to SKSE/game APIs
    │   ├── Log.h               ← project logging macros/helpers
    │   ├── Result.h            ← error handling types
    │   └── Types.h             ← common typedefs, forward declarations
    ├── cosave/
    │   ├── CosaveManager.h
    │   ├── BlockTypes.h
    │   └── PersistenceAPI.h
    ├── dsl/
    │   ├── USSParser.h
    │   ├── TokenStore.h
    │   ├── StyleResolver.h
    │   └── SkinManager.h
    ├── layout/
    │   ├── YogaWrapper.h
    │   └── LayoutEngine.h
    ├── widget/
    │   ├── WidgetRegistry.h
    │   ├── WidgetManifest.h
    │   ├── DataBindingEngine.h
    │   ├── Compositor.h
    │   └── EditMode.h
    ├── input/
    │   ├── InputContextStack.h
    │   ├── InputRouter.h
    │   └── KeybindManager.h
    ├── mcm/
    │   ├── MCMCompat.h         ← SkyUI backwards compat layer
    │   └── MCM2.h              ← native MCM2
    ├── screens/
    │   └── (one header per screen, added by screen tasks)
    ├── render/
    │   ├── ScaleformBridge.h
    │   └── ComponentRenderer.h
    └── integration/
        ├── ModRegistrationAPI.h
        └── FrozenGiants.h
```

### Source structure: `src/`

```
src/
├── main.cpp                    ← plugin entry point (exists)
├── core/
│   ├── GameBridge.cpp          ← concrete SKSE/Scaleform impl
│   └── Log.cpp
├── cosave/
│   ├── CosaveManager.cpp
│   └── PersistenceAPI.cpp
├── dsl/
│   ├── USSParser.cpp
│   ├── TokenStore.cpp
│   ├── StyleResolver.cpp
│   └── SkinManager.cpp
├── layout/
│   ├── YogaWrapper.cpp
│   └── LayoutEngine.cpp
├── widget/
│   ├── WidgetRegistry.cpp
│   ├── DataBindingEngine.cpp
│   ├── Compositor.cpp
│   └── EditMode.cpp
├── input/
│   ├── InputContextStack.cpp
│   ├── InputRouter.cpp
│   └── KeybindManager.cpp
├── mcm/
│   ├── MCMCompat.cpp
│   └── MCM2.cpp
├── screens/
│   └── (one .cpp per screen)
├── render/
│   ├── ScaleformBridge.cpp
│   └── ComponentRenderer.cpp
└── integration/
    ├── ModRegistrationAPI.cpp
    └── FrozenGiants.cpp
```

### Task → module map

| Phase | Module(s) | Key files |
|---|---|---|
| 0 — Foundation | `layout/`, `dsl/`, `cosave/` | YogaWrapper, USSParser, TokenStore, CosaveManager, PersistenceAPI |
| 1 — Widget Runtime | `widget/` | WidgetRegistry, DataBindingEngine, EditMode |
| 2 — Input System | `input/` | InputContextStack, InputRouter, KeybindManager |
| 3 — DSL Runtime | `dsl/`, `render/` | StyleResolver, SkinManager, Compositor, ComponentRenderer |
| 4 — MCM Compat | `mcm/` | MCMCompat |
| 5 — MCM2 Native | `mcm/` | MCM2 |
| 6 — Components | `render/` | ComponentRenderer (extended) |
| 7–8 — Screens | `screens/` | One file per screen |
| 9 — Mod Integration | `integration/` | ModRegistrationAPI |
| 10 — Localisation | `core/` or `dsl/` | (extends existing) |
| 12 — Frozen Giants | `integration/` | FrozenGiants |
| 13 — Outfit System | `screens/` or new `outfit/` | TBD |
| 14 — HUD Widgets | `widget/` + DSL definitions | (data-driven, minimal C++) |

### CMakeLists.txt approach

Use a `SOURCES` manifest (not a glob) so that every new file is an
explicit, reviewable addition:

```cmake
set(SOURCES
    src/main.cpp
    # core
    src/core/GameBridge.cpp
    src/core/Log.cpp
    # cosave
    src/cosave/CosaveManager.cpp
    src/cosave/PersistenceAPI.cpp
    # ... each task appends here
)

set(HEADERS
    include/PCH.h
    include/Version.h
    # core
    include/ohui/core/GameBridge.h
    include/ohui/core/Log.h
    include/ohui/core/Result.h
    include/ohui/core/Types.h
    # ... each task appends here
)
```

Each task's commit includes its additions to SOURCES and HEADERS.
This makes diffs reviewable and prevents orphaned files.

---

## 3. Test Infrastructure

### Framework choice: Catch2

Catch2 v3 via vcpkg. Single-header convenience, good CMake integration,
expression-based assertions, sections for test organisation, BDD macros
available if wanted. Already battle-proven in C++23 projects.

### vcpkg.json addition

```json
{
    "dependencies": [
        "spdlog",
        "xbyak",
        "catch2"
    ]
}
```

### Test directory structure

```
tests/
├── CMakeLists.txt              ← test executable target
├── main.cpp                    ← Catch2 main (one line)
├── core/
│   └── ResultTests.cpp
├── cosave/
│   ├── CosaveRoundtripTests.cpp
│   ├── PersistenceAPITests.cpp
│   └── CosaveContractTests.cpp     ← interface contracts
├── dsl/
│   ├── USSParserTests.cpp
│   ├── TokenStoreTests.cpp
│   └── StyleResolverTests.cpp
├── layout/
│   ├── YogaWrapperTests.cpp
│   └── LayoutEngineTests.cpp
├── widget/
│   ├── WidgetRegistryTests.cpp
│   ├── DataBindingEngineTests.cpp
│   └── EditModeTests.cpp
├── input/
│   └── InputContextStackTests.cpp
├── fixtures/                   ← test data files
│   ├── valid.uss
│   ├── malformed.uss
│   ├── tokens-base.uss
│   ├── tokens-override.uss
│   └── test-cosave.ohui
└── mocks/
    ├── MockGameBridge.h
    ├── MockScaleformBridge.h
    └── MockEventSource.h
```

### tests/CMakeLists.txt

```cmake
find_package(Catch2 3 CONFIG REQUIRED)

# Collect test sources (explicit manifest, same as main project)
set(TEST_SOURCES
    main.cpp
    # Added per-task as tests are written
)

add_executable(ohui_tests ${TEST_SOURCES})

target_include_directories(ohui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

# Link against project source as a static lib (not the DLL)
# This requires a separate STATIC library target for testable code.
target_link_libraries(ohui_tests PRIVATE
    ohui_core      # static lib of all non-DLL-entry-point code
    Catch2::Catch2WithMain
)

target_compile_features(ohui_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(ohui_tests)
```

### Splitting the build: ohui_core static lib + ohui DLL

This is the key architectural decision for testability. The main
CMakeLists.txt needs to produce two targets:

1. **`ohui_core`** — a STATIC library containing all logic. Everything
   except `main.cpp` (the SKSE entry point). This is what tests link
   against.

2. **`OHUI`** — the SHARED library (DLL) that links `ohui_core` +
   `main.cpp` + CommonLibSSE. This is what ships.

```cmake
# --- Static library of all OHUI logic (testable without SKSE) ---
add_library(ohui_core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(ohui_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ohui_core does NOT link CommonLibSSE.
# Code that needs game APIs goes through GameBridge interface.
# Tests substitute MockGameBridge.

# --- DLL (ships to users) ---
add_library(OHUI SHARED
    src/main.cpp
    src/core/GameBridge.cpp   # concrete SKSE implementation
    version.rc
)

target_link_libraries(OHUI PRIVATE
    ohui_core
    CommonLibSSE::CommonLibSSE
)
```

This split is the enabler for the entire test strategy. Without it,
every test needs CommonLibSSE linked, which means every test needs
SKSE headers, which makes them fragile and slow.

### Test levels

**Level 1 — Unit tests (ohui_core, no game dependencies)**

Pure logic. Runs on any machine, no Skyrim install needed. This is
the backbone. CI runs these on every push.

Covers: USS parsing, token resolution, layout computation, cosave
serialization, widget state machines, input context stack logic,
edit mode undo/redo, persistence API key-value operations, style
cascade resolution.

**Level 2 — Contract tests (interface invariant enforcement)**

When Task A defines an interface consumed by Tasks B, C, D, the
contract tests live alongside Task A's tests and verify the
interface's behavioural guarantees. If a later task modifies
something in A's domain, these break first.

Example contracts:
- `WidgetRegistry` always returns `false` for duplicate registration
  (never throws, never silently succeeds)
- `CosaveManager::Read` after `Write` produces identical data
- `USSParser` never crashes on malformed input (returns error result)
- `DataBindingEngine` never fires update callbacks when value unchanged
- `PersistenceAPI` enforces namespace isolation (cross-mod read
  returns default)

These are tagged `[contract]` in Catch2 so they can be run separately:
```
ohui_tests [contract]
```

**Level 3 — In-game smoke tests (console commands)**

Registered as SKSE console commands in debug builds only. Not
automated, but repeatable and scriptable.

```cpp
#ifdef _DEBUG
namespace DebugConsole {
    bool Register(RE::BSScript::IVirtualMachine*);
}
// Registers: ohui.test.widget_lifecycle, ohui.test.skin_swap, etc.
#endif
```

Output goes to the SKSE log. Format:
```
[TEST] widget_lifecycle: PASS (3 checks, 0 failures)
[TEST] skin_swap: FAIL — expected "dark" got "default" at step 4
```

### deploy.ps1 integration

The deploy script gains a test gate. Tests run before deployment.
Failed tests block the deploy unless explicitly overridden.

```powershell
# In deploy.ps1, after build, before deploy:
if (-not $SkipTests) {
    Write-Host "`n=== Running Tests ===" -ForegroundColor Cyan
    ctest --test-dir $BuildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        Write-Host "TESTS FAILED — deploy blocked" -ForegroundColor Red
        Write-Host "Use -SkipTests to deploy anyway" -ForegroundColor Yellow
        exit 1
    }
}
```

### CI integration

Add a test step to the release workflow between Build and Package:

```yaml
- name: Run tests
  run: |
    ctest --test-dir build/release --build-config Release --output-on-failure
```

### Task definition of done — test requirements

Every task's definition of done implicitly includes:

1. Unit tests for all new public interfaces
2. Contract tests for any interface consumed by downstream tasks
3. All existing tests still pass (zero regressions)
4. `ctest` green before commit

This doesn't need to be restated in every task. It's a project-level
invariant enforced by the deploy gate.

---

## 4. Mock Boundary Layer

### The problem

OHUI's core logic (DSL parsing, layout, widget state, cosave,
persistence, input routing) doesn't need Skyrim running. But if
it calls `RE::UI::GetSingleton()` or `SKSE::GetSerializationInterface()`
directly, it can't be tested without the game.

### The solution: GameBridge interface

A thin abstract interface at the SKSE/Scaleform boundary. The core
systems depend on the interface. The DLL provides the real
implementation. Tests provide mocks.

```cpp
// include/ohui/core/GameBridge.h

namespace ohui {

/// Abstract interface to game engine services.
/// Core systems depend on this; never on RE:: or SKSE:: directly.
class IGameBridge {
public:
    virtual ~IGameBridge() = default;

    // --- Player state ---
    virtual float GetPlayerHealth() const = 0;
    virtual float GetPlayerMaxHealth() const = 0;
    virtual float GetPlayerStamina() const = 0;
    virtual float GetPlayerMagicka() const = 0;
    virtual int32_t GetPlayerLevel() const = 0;

    // --- UI state ---
    virtual bool IsMenuOpen(std::string_view menuName) const = 0;
    virtual bool IsInLoadingScreen() const = 0;

    // --- Time ---
    virtual float GetGameHour() const = 0;

    // --- File I/O (for cosave, USS files) ---
    virtual std::vector<uint8_t> ReadFile(const std::filesystem::path& path) const = 0;
    virtual bool WriteFile(const std::filesystem::path& path,
                           std::span<const uint8_t> data) const = 0;

    // Extend as subsystems need more game access.
    // Each addition is a deliberate decision, not a leak.
};

}  // namespace ohui
```

```cpp
// tests/mocks/MockGameBridge.h

namespace ohui::test {

class MockGameBridge : public IGameBridge {
public:
    // Scriptable state for tests
    float playerHealth = 100.0f;
    float playerMaxHealth = 100.0f;
    float playerStamina = 100.0f;
    float playerMagicka = 100.0f;
    int32_t playerLevel = 1;
    std::unordered_set<std::string> openMenus;
    bool loading = false;
    float gameHour = 12.0f;

    // Written files captured for verification
    std::unordered_map<std::string, std::vector<uint8_t>> writtenFiles;
    std::unordered_map<std::string, std::vector<uint8_t>> fileContents;

    float GetPlayerHealth() const override { return playerHealth; }
    float GetPlayerMaxHealth() const override { return playerMaxHealth; }
    float GetPlayerStamina() const override { return playerStamina; }
    float GetPlayerMagicka() const override { return playerMagicka; }
    int32_t GetPlayerLevel() const override { return playerLevel; }

    bool IsMenuOpen(std::string_view name) const override {
        return openMenus.contains(std::string(name));
    }

    bool IsInLoadingScreen() const override { return loading; }
    float GetGameHour() const override { return gameHour; }

    std::vector<uint8_t> ReadFile(const std::filesystem::path& path) const override {
        auto it = fileContents.find(path.string());
        if (it != fileContents.end()) return it->second;
        return {};
    }

    bool WriteFile(const std::filesystem::path& path,
                   std::span<const uint8_t> data) override {
        writtenFiles[path.string()] = {data.begin(), data.end()};
        return true;
    }
};

}  // namespace ohui::test
```

### Scaleform bridge (Phase 3+)

Same pattern for the rendering boundary. Doesn't need to exist
until Phase 3 (DSL runtime / compositor). Sketch:

```cpp
class IScaleformBridge {
public:
    virtual ~IScaleformBridge() = default;
    virtual void InvokeMethod(std::string_view movieClip,
                              std::string_view method,
                              std::span<const ScaleformValue> args) = 0;
    virtual void SetVariable(std::string_view path, const ScaleformValue& value) = 0;
    virtual ScaleformValue GetVariable(std::string_view path) const = 0;
};
```

### Rule

**No `RE::` or `SKSE::` calls in any file under `src/` except
`src/core/GameBridge.cpp` and `src/main.cpp`.** Everything else
goes through the interface. This is enforced by the static library
split: `ohui_core` does not link CommonLibSSE, so any direct
game API call is a linker error.

This is strict but it's what makes the test pyramid work. The
concrete `GameBridge.cpp` implementation is thin glue — it calls
through to `RE::` and `SKSE::` APIs and nothing else. All logic
lives in the testable core.

---

## 5. Conventions

This should live as `.private/reference-overrides/ohui-conventions.md`
and be consulted by Code at the start of every session.

### Error handling

Use `ohui::Result<T>` (a type alias for `std::expected<T, Error>`).
No exceptions in core logic. No silent failures. Every function that
can fail returns a Result. The Error type carries a message and an
error code enum.

```cpp
namespace ohui {

enum class ErrorCode {
    ParseError,
    FileNotFound,
    InvalidFormat,
    SizeLimitExceeded,
    DuplicateRegistration,
    // ... extend per subsystem
};

struct Error {
    ErrorCode code;
    std::string message;
};

template<typename T>
using Result = std::expected<T, Error>;

}
```

At the DLL boundary (`main.cpp`, `GameBridge.cpp`), Results are
unwrapped, errors are logged, and sensible defaults are used.
Core code never calls `logger::` directly — it returns errors.
The boundary layer logs them.

### Logging

- `info` — plugin lifecycle events (load, data loaded, game loaded/saved)
- `warn` — recoverable issues (unknown USS property, missing token, stale cosave block)
- `error` — unrecoverable issues that disable a subsystem (hook failed, cosave corrupt)
- `debug` — development diagnostics (parsed N rules, resolved N tokens)
- `trace` — per-frame or per-item detail (NEVER in release builds)

Guard trace-level output behind `#ifdef _DEBUG` or a runtime flag.
Never log in hot paths (see `.private/reference/performance-patterns.md`).

### Memory ownership

- **Widget instances** are owned by `WidgetRegistry`. Subsystems hold
  non-owning references (raw pointers or `std::reference_wrapper`).
- **USS rule sets** are owned by `SkinManager`. Widgets hold a
  `const&` to their resolved style.
- **Cosave blocks** are owned by `CosaveManager`. Subsystems read
  via accessor, write via `CosaveManager::QueueWrite`.
- **Input contexts** are owned by `InputContextStack`. Screens push/pop
  but don't own.

General rule: `std::unique_ptr` for owning, raw pointer for
non-owning. `std::shared_ptr` only when lifetime genuinely cannot
be determined statically (expected to be rare).

### Naming

- Namespaces: `ohui`, `ohui::dsl`, `ohui::widget`, etc.
- Files: PascalCase for classes (`USSParser.h`), lowercase for
  non-class headers (`Types.h`)
- Classes: PascalCase (`WidgetRegistry`)
- Methods: PascalCase (`GetWidget`, `ParseFile`)
- Member variables: `m_` prefix (`m_widgets`, `m_tokenStore`)
- Local variables: camelCase (`parsedRules`, `blockData`)
- Constants: `k` prefix (`kMaxModDataSize`, `kDefaultUpdateRate`)
- Enums: PascalCase type, PascalCase values (`ErrorCode::ParseError`)
- Test files: `{Class}Tests.cpp`

### String handling

- `std::string` and `std::string_view` for all internal strings
- `RE::BSFixedString` only at the GameBridge boundary
- USS property names stored as `std::string`
- Binding keys stored as `std::string`
- Never hold raw `const char*` from BSFixedString (see
  `.private/reference/commonlibsse-ng-gotchas.md`)

### Include order

1. Corresponding header (`#include "ohui/dsl/USSParser.h"`)
2. Project headers (`#include "ohui/core/Result.h"`)
3. Third-party headers (`#include <yoga/Yoga.h>`)
4. Standard library headers (`#include <vector>`)

Blank line between each group. PCH covers standard library and
spdlog; don't re-include those in individual files.

---

## 6. Architecture.md — Skeleton

Replace the current template placeholder with a real document that
covers the module dependency graph and subsystem boundaries. This
grows with each phase but the skeleton should exist before Phase 0.

Key sections:

1. **Layer model** — four layers (Game Engine → Bridge → Core → Presentation),
   what lives in each, dependency direction (Core never depends on Bridge,
   Bridge never depends on Presentation)
2. **Module map** — the `src/` structure from §2 with one-line descriptions
3. **Subsystem dependency graph** — which modules depend on which (matches
   the TASKS.md dependency graph but at code level)
4. **Data flow** — how a game state change (e.g., player takes damage) flows
   from game engine → GameBridge → DataBindingEngine → Widget → Compositor
   → Scaleform
5. **Build targets** — `ohui_core` (static, testable), `OHUI` (DLL, ships),
   `ohui_tests` (test executable)
6. **Extension points** — where mod authors plug in (widget registration,
   binding registration, skin files, MCM2 API)

---

## 7. Pre-Phase-0 Task: TASK-000 · Project Scaffold

Everything in this document should land as TASK-000, executed before
any Phase 0 work begins. Definition of done:

- Directory restructuring complete (screens, systems, tools moved)
- All path references in CLAUDE.md, TASKS.md, DESIGN.md updated
- `include/ohui/core/` exists with GameBridge.h, Result.h, Types.h
- `src/core/` exists with stubs
- `tests/` exists with Catch2 wired, one trivial passing test
- `ohui_core` static library target builds
- `ohui_tests` executable builds and runs with `ctest`
- `tests/mocks/MockGameBridge.h` exists
- `.private/reference-overrides/ohui-conventions.md` exists
- `docs/Architecture.md` has real skeleton content
- `deploy.ps1` updated: new path, test gate added
- CI workflow updated: test step added
- `ctest` passes (trivial test confirms infrastructure works)

After this task, every subsequent task has: a known place to put
its code, a test framework to write against, conventions to follow,
and a mock boundary to test through. No task needs to invent
structure.
