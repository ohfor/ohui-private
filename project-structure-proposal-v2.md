# OHUI — Project Structure & Test Infrastructure Proposal (v2)

> Revised after Code review. Changes from v1 marked with (v2).

This document defines the directory restructuring, source module layout,
test infrastructure, conventions, and mock boundary layer needed before
Phase 0 tasks begin executing.

---

## 1. Directory Restructuring

### Moves

| Current Location | New Location | Reason |
|---|---|---|
| `screens/` | `.private/screens/` | Design docs, not public content |
| `systems/` | `.private/systems/` | Design docs, not public content |

**(v2)** `scripts/` stays at project root. It contains both internal
build tooling (`deploy.ps1`, `build_package.ps1`, `check_versions.py`)
and public Papyrus content (`scripts/papyrus/`). This matches the
proven SLID pattern where the sync workflow explicitly copies
`scripts/papyrus/` to the public repo. Build tooling being publicly
visible is harmless.

### Path reference updates

Every reference to `screens/` and `systems/` in CLAUDE.md, TASKS.md,
DESIGN.md, and any other documents must be updated to `.private/screens/`
and `.private/systems/` respectively.

### After restructuring

```
ohui-private/
├── .claude/
├── .github/workflows/
├── .private/
│   ├── decisions/
│   ├── notes/
│   ├── reference/              ← template-owned guides
│   ├── reference-overrides/
│   │   └── ohui-conventions.md ← NEW (see §5)
│   ├── research/
│   ├── screens/                ← 17 screen design docs (MOVED)
│   └── systems/                ← 16 system architecture docs (MOVED)
├── assets/
├── dist/
├── docs/
│   ├── Architecture.md         ← rewritten (see §6)
│   ├── Build.md
│   └── CHANGELOG.md
├── include/
│   ├── CorePCH.h               ← NEW (see §3)
│   ├── PCH.h
│   ├── Version.h
│   └── ohui/                   ← NEW (see §2)
├── scripts/                    ← (v2) STAYS at root
│   ├── build_package.ps1
│   ├── check_versions.py
│   └── deploy.ps1
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
and sources mirror each other. A task knows where its code lands by
looking at the module map below.

**(v2)** The full module map below is **illustrative guidance**, not a
contract. Only Phase 0 directories and files are created at TASK-000.
Subsequent phases create their own directories and files as they
execute. No task creates new top-level directories in `src/` without
amending this document.

### Header structure: `include/ohui/`

The nested namespace means includes become `#include "ohui/core/Result.h"`
etc. `PCH.h`, `CorePCH.h`, and `Version.h` remain at `include/` root
— they serve the SKSE/build contract, not the OHUI architecture.

**Created at TASK-000 (Phase 0 needs these):**

```
include/
├── CorePCH.h                   ← (v2) static lib precompiled header
├── PCH.h                       ← existing, DLL-only PCH
├── Version.h                   ← existing
└── ohui/
    ├── core/
    │   ├── IFileSystem.h       ← (v2) narrow interface: file I/O
    │   ├── Log.h               ← project logging helpers
    │   ├── Result.h            ← error handling types
    │   └── Types.h             ← common typedefs, forward declarations
    ├── cosave/
    │   ├── CosaveManager.h
    │   └── PersistenceAPI.h
    ├── dsl/
    │   ├── USSParser.h
    │   └── TokenStore.h
    └── layout/
        └── YogaWrapper.h
```

**Created later by their respective phases (illustrative, not prescriptive):**

```
    ├── core/
    │   ├── IPlayerState.h      ← Phase 1 (data binding needs it)
    │   ├── IUIState.h          ← Phase 2 or 3 (when screens need it)
    │   └── IGameTime.h         ← Phase 1 (time bindings)
    ├── widget/
    │   ├── WidgetRegistry.h    ← Phase 1
    │   ├── DataBindingEngine.h ← Phase 1
    │   └── EditMode.h          ← Phase 1
    ├── input/
    │   ├── InputContextStack.h ← Phase 2
    │   └── InputRouter.h       ← Phase 2
    ├── render/
    │   ├── IScaleformBridge.h  ← Phase 3
    │   ├── Compositor.h        ← Phase 3
    │   └── ComponentRenderer.h ← Phase 6
    ├── mcm/
    │   ├── MCMCompat.h         ← Phase 4
    │   └── MCM2.h              ← Phase 5
    ├── screens/                ← Phase 7+ (one header per screen)
    └── integration/            ← Phase 9, 12
```

### Source structure: `src/`

Mirrors `include/ohui/`. **Only Phase 0 modules created at TASK-000:**

```
src/
├── main.cpp                    ← existing, DLL entry point
├── core/
│   ├── GameBridge.cpp          ← concrete SKSE implementations of interfaces
│   └── Log.cpp
├── cosave/
│   ├── CosaveManager.cpp
│   └── PersistenceAPI.cpp
├── dsl/
│   ├── USSParser.cpp
│   └── TokenStore.cpp
└── layout/
    └── YogaWrapper.cpp
```

Subsequent phases add their directories and files. Each task's commit
includes its additions to the CMake SOURCES and HEADERS manifests.

### Task → module map

| Phase | Module(s) | Notes |
|---|---|---|
| 0 — Foundation | `layout/`, `dsl/`, `cosave/`, `core/` | Created at TASK-000 |
| 1 — Widget Runtime | `widget/`, `core/` (adds IPlayerState, IGameTime) | Created at Phase 1 start |
| 2 — Input System | `input/`, `core/` (adds IUIState if needed) | Created at Phase 2 start |
| 3 — DSL Runtime | `dsl/` (extends), `render/` (adds IScaleformBridge) | Created at Phase 3 start |
| 4–5 — MCM | `mcm/` | Created at Phase 4 start |
| 6 — Components | `render/` (extends) | Extends Phase 3 files |
| 7+ — Screens | `screens/` | One file per screen |
| 9 — Mod Integration | `integration/` | Created at Phase 9 start |
| Later phases | TBD | Determined when phase begins |

### CMakeLists.txt approach

Explicit SOURCES manifest (not a glob). Every new file is a reviewable
addition:

```cmake
set(CORE_SOURCES
    src/core/Log.cpp
    src/cosave/CosaveManager.cpp
    src/cosave/PersistenceAPI.cpp
    src/dsl/USSParser.cpp
    src/dsl/TokenStore.cpp
    src/layout/YogaWrapper.cpp
    # Each task appends here
)

set(CORE_HEADERS
    include/ohui/core/IFileSystem.h
    include/ohui/core/Log.h
    include/ohui/core/Result.h
    include/ohui/core/Types.h
    include/ohui/cosave/CosaveManager.h
    include/ohui/cosave/PersistenceAPI.h
    include/ohui/dsl/USSParser.h
    include/ohui/dsl/TokenStore.h
    include/ohui/layout/YogaWrapper.h
    # Each task appends here
)
```

---

## 3. Test Infrastructure

### Framework: Catch2 v3

Via vcpkg. Expression-based assertions, sections for test organisation,
tag-based filtering for contract tests.

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

### The ohui_core static lib split

This is the key architectural decision for testability. The CMakeLists.txt
produces three targets:

1. **`ohui_core`** — STATIC library. All logic except the SKSE entry
   point and concrete game bridge implementations. This is what tests
   link against. **Uses `CorePCH.h` (no CommonLibSSE).**

2. **`OHUI`** — SHARED library (DLL). Links `ohui_core` + `main.cpp` +
   `GameBridge.cpp` + CommonLibSSE. This ships to users. **Uses `PCH.h`
   (full CommonLibSSE).**

3. **`ohui_tests`** — test executable. Links `ohui_core` + Catch2.
   **Uses `CorePCH.h`.**

### (v2) CorePCH.h — the separate precompiled header

The existing `PCH.h` includes `RE/Skyrim.h`, `REL/Relocation.h`, and
`SKSE/SKSE.h`. If `ohui_core` used this PCH, it would transitively
depend on CommonLibSSE headers, defeating the isolation.

`CorePCH.h` includes standard library and spdlog only:

```cpp
// include/CorePCH.h
// Precompiled header for ohui_core (no game engine dependencies)

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using namespace std::literals;
```

The full `PCH.h` stays unchanged and is used only by the DLL target
(which links CommonLibSSE).

CMake wiring:

```cmake
# --- ohui_core: testable logic, no game engine ---
add_library(ohui_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(ohui_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_precompile_headers(ohui_core PRIVATE include/CorePCH.h)

target_link_libraries(ohui_core PRIVATE
    spdlog::spdlog
    # Yoga, other non-game dependencies added by their tasks
)

target_compile_features(ohui_core PRIVATE cxx_std_23)

# --- OHUI DLL: ships to users ---
add_library(OHUI SHARED
    src/main.cpp
    src/core/GameBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
)

target_precompile_headers(OHUI PRIVATE include/PCH.h)

target_link_libraries(OHUI PRIVATE
    ohui_core
    CommonLibSSE::CommonLibSSE
)

# --- ohui_tests: test executable ---
# (see tests/CMakeLists.txt below)
```

### Test directory structure

**Created at TASK-000 (minimal scaffold):**

```
tests/
├── CMakeLists.txt
├── main.cpp                    ← Catch2 main
├── mocks/
│   └── MockFileSystem.h        ← (v2) narrow mock for IFileSystem
├── fixtures/                   ← test data files (populated by tasks)
│   └── .gitkeep
└── SanityTests.cpp             ← trivial test proving infrastructure works
```

**Populated by Phase 0 tasks as they execute:**

```
tests/
├── cosave/
│   ├── CosaveRoundtripTests.cpp
│   ├── PersistenceAPITests.cpp
│   └── CosaveContractTests.cpp
├── dsl/
│   ├── USSParserTests.cpp
│   └── TokenStoreTests.cpp
├── layout/
│   └── YogaWrapperTests.cpp
└── fixtures/
    ├── valid.uss
    ├── malformed.uss
    ├── tokens-base.uss
    ├── tokens-override.uss
    └── test-cosave.ohui
```

**Populated by later phases:**

```
tests/
├── widget/
│   ├── WidgetRegistryTests.cpp
│   ├── DataBindingEngineTests.cpp
│   └── EditModeTests.cpp
├── input/
│   └── InputContextStackTests.cpp
├── mocks/
│   ├── MockFileSystem.h
│   ├── MockPlayerState.h      ← added Phase 1
│   ├── MockUIState.h          ← added Phase 2/3
│   ├── MockGameTime.h         ← added Phase 1
│   └── MockScaleformBridge.h  ← added Phase 3
└── ...
```

### tests/CMakeLists.txt

```cmake
find_package(Catch2 3 CONFIG REQUIRED)

set(TEST_SOURCES
    main.cpp
    SanityTests.cpp
    # Each task appends here
)

add_executable(ohui_tests ${TEST_SOURCES})

target_include_directories(ohui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(ohui_tests PRIVATE
    ohui_core
    Catch2::Catch2WithMain
)

target_precompile_headers(ohui_tests REUSE_FROM ohui_core)

target_compile_features(ohui_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(ohui_tests)
```

### Test levels

**Level 1 — Unit tests (ohui_core, no game dependencies)**

Pure logic. Runs on any machine, no Skyrim install. CI runs these on
every push. Covers: USS parsing, token resolution, layout computation,
cosave serialization, widget state machines, input context stack logic,
edit mode undo/redo, persistence API key-value operations, style
cascade resolution.

**Level 2 — Contract tests (interface invariant enforcement)**

When Task A defines an interface consumed by Tasks B, C, D, contract
tests live alongside Task A's test file and verify behavioural
guarantees. If a later task modifies something in A's domain, these
break first.

Example contracts:
- `WidgetRegistry` returns `false` for duplicate registration
  (never throws, never silently succeeds)
- `CosaveManager::Read` after `Write` produces identical data
- `USSParser` never crashes on malformed input (returns error Result)
- `DataBindingEngine` never fires update callbacks when value unchanged
- `PersistenceAPI` enforces namespace isolation

Tagged `[contract]` in Catch2 for selective execution:
```
ohui_tests [contract]
```

**Level 3 — In-game smoke tests (console commands, debug builds only)**

Registered as SKSE console commands, gated behind `#ifdef _DEBUG`.
Not automated but repeatable.

```cpp
#ifdef _DEBUG
namespace DebugConsole {
    bool Register(RE::BSScript::IVirtualMachine*);
}
#endif
```

Output to SKSE log:
```
[TEST] widget_lifecycle: PASS (3 checks, 0 failures)
[TEST] skin_swap: FAIL — expected "dark" got "default" at step 4
```

### (v2) deploy.ps1 integration — resilient test gate

Tests run before deployment when available. Missing test binary
is a warning, not a failure. Failed tests block deploy unless
explicitly overridden.

```powershell
param(
    # ... existing params ...
    [switch]$SkipTests
)

# --- Test gate (after build, before deploy) ---
if (-not $PapyrusOnly -and -not $SkipTests) {
    $testExe = "$BuildDir/Release/ohui_tests.exe"
    if (Test-Path $testExe) {
        Write-Host "`n=== Running Tests ===" -ForegroundColor Cyan
        ctest --test-dir $BuildDir --build-config Release --output-on-failure
        if ($LASTEXITCODE -ne 0) {
            Write-Host "TESTS FAILED — deploy blocked" -ForegroundColor Red
            Write-Host "Use -SkipTests to deploy anyway" -ForegroundColor Yellow
            exit 1
        }
        Write-Host "  All tests passed" -ForegroundColor Green
    } else {
        Write-Host "`n=== Tests not built — skipping ===" -ForegroundColor Yellow
    }
}
```

### CI integration

Add test step to release workflow between Build and Package:

```yaml
- name: Run tests
  run: |
    ctest --test-dir build/release --build-config Release --output-on-failure
```

### Task definition of done — test requirements

Every task's definition of done implicitly includes:

1. Unit tests for all new public interfaces
2. Contract tests for any interface consumed by downstream tasks
3. All existing tests pass (zero regressions)
4. `ctest` green before commit

This is a project-level invariant enforced by the deploy gate, not
restated in every task.

---

## 4. Mock Boundary Layer

### The problem

OHUI's core logic doesn't need Skyrim running. But if it calls
`RE::UI::GetSingleton()` or `SKSE::GetSerializationInterface()`
directly, it can't be tested without the game.

### (v2) The solution: narrow interfaces per concern

Instead of a single `IGameBridge` god-interface, define small
interfaces per concern following Interface Segregation. A concrete
`GameBridge` class implements all of them. Test code depends only
on the slice it needs.

**Created at TASK-000 (Phase 0 needs file I/O for cosave):**

```cpp
// include/ohui/core/IFileSystem.h

namespace ohui {

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual std::vector<uint8_t> ReadFile(
        const std::filesystem::path& path) const = 0;
    virtual bool WriteFile(
        const std::filesystem::path& path,
        std::span<const uint8_t> data) const = 0;
    virtual bool FileExists(
        const std::filesystem::path& path) const = 0;
};

}  // namespace ohui
```

```cpp
// tests/mocks/MockFileSystem.h

namespace ohui::test {

class MockFileSystem : public IFileSystem {
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    std::unordered_map<std::string, std::vector<uint8_t>> written;

    std::vector<uint8_t> ReadFile(
            const std::filesystem::path& path) const override {
        auto it = files.find(path.string());
        if (it != files.end()) return it->second;
        return {};
    }

    bool WriteFile(
            const std::filesystem::path& path,
            std::span<const uint8_t> data) override {
        written[path.string()] = {data.begin(), data.end()};
        return true;
    }

    bool FileExists(
            const std::filesystem::path& path) const override {
        return files.contains(path.string());
    }
};

}  // namespace ohui::test
```

**Created by later phases as their consumers appear:**

| Interface | Created when | Consumer |
|---|---|---|
| `IFileSystem` | TASK-000 | CosaveManager, USSParser |
| `IPlayerState` | Phase 1 | DataBindingEngine |
| `IGameTime` | Phase 1 | DataBindingEngine (time bindings) |
| `IUIState` | Phase 2 or 3 | InputRouter, screen management |
| `IScaleformBridge` | Phase 3 | Compositor, ComponentRenderer |

Each interface gets a corresponding mock in `tests/mocks/`. The
concrete `GameBridge` class (in `src/core/GameBridge.cpp`, DLL-only)
implements all interfaces, dispatching to `RE::` and `SKSE::` APIs.

### (v2) RE:: types rule — pragmatic boundary

The boundary rule is:

- **RE:: value types are allowed in core headers.** `RE::FormID`
  (a `uint32_t` typedef), `RE::FormType` (an enum), and similar
  data-carrying types may appear in `ohui_core` data structures.
  These are just numbers/enums and don't prevent test construction.

- **RE:: pointer-to-game-object types are banned from core interfaces.**
  `RE::TESForm*`, `RE::Actor*`, `RE::TESObjectREFR*` etc. must not
  appear in `ohui_core` public APIs. At the bridge boundary, translate
  game pointers into OHUI-owned descriptors carrying the data core
  logic needs (FormID, name string, type enum) without requiring a
  live game object.

- **RE:: function calls (invocations on game singletons) are banned
  from ohui_core entirely.** No `RE::UI::GetSingleton()`,
  `RE::PlayerCharacter::GetSingleton()`, `RE::TESDataHandler::GetSingleton()`
  etc. All such calls live in `GameBridge.cpp` or `main.cpp`.

This is enforced structurally: `ohui_core` uses `CorePCH.h` which does
not include CommonLibSSE headers. Any `RE::` reference in core code
requires an explicit `#include` of the relevant CommonLibSSE header,
making it visible in code review. Value type includes (like
`RE/BSCoreTypes.h` for FormID) are permitted; game object includes
(like `RE/Actor.h`) are not.

**Note:** This rule is a Phase 1+ concern in practice. Phase 0 tasks
(USS parser, layout engine, cosave serialization) don't touch game
forms at all. The rule is documented now so it's established before
the first task that needs game data types.

---

## 5. Conventions

Lives as `.private/reference-overrides/ohui-conventions.md`. Consulted
by Code at the start of every session.

### Error handling

Use `ohui::Result<T>` — a type alias for `std::expected<T, Error>`.
No exceptions in core logic. No silent failures. Every function that
can fail returns a Result.

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

}  // namespace ohui
```

At the DLL boundary (`main.cpp`, `GameBridge.cpp`), Results are
unwrapped, errors are logged, and sensible defaults are used.
Core code never calls `logger::` directly — it returns errors.
The boundary layer logs them.

**Note:** `std::expected` requires C++23 (MSVC 17.6+, confirmed
available with our toolchain).

### Logging

- `info` — plugin lifecycle events (load, data loaded, game loaded/saved)
- `warn` — recoverable issues (unknown USS property, missing token, stale cosave block)
- `error` — unrecoverable issues that disable a subsystem (hook failed, cosave corrupt)
- `debug` — development diagnostics (parsed N rules, resolved N tokens)
- `trace` — per-frame or per-item detail (NEVER in release builds)

Guard trace output behind `#ifdef _DEBUG` or a runtime flag. Never
log in hot paths (see `.private/reference/performance-patterns.md`).

### Memory ownership

- **Widget instances** owned by `WidgetRegistry`. Subsystems hold
  non-owning references.
- **USS rule sets** owned by `SkinManager`. Widgets hold `const&`
  to resolved style.
- **Cosave blocks** owned by `CosaveManager`. Subsystems read via
  accessor, write via `CosaveManager::QueueWrite`.
- **Input contexts** owned by `InputContextStack`. Screens push/pop
  but don't own.

General rule: `std::unique_ptr` for owning, raw pointer for
non-owning. `std::shared_ptr` only when lifetime genuinely cannot
be determined statically (expected to be rare).

### Naming

- **Namespaces:** `ohui`, `ohui::dsl`, `ohui::widget`, etc.
- **Files:** PascalCase for classes (`USSParser.h`), lowercase for
  non-class headers (`Types.h`)
- **Classes:** PascalCase (`WidgetRegistry`)
- **Methods:** PascalCase (`GetWidget`, `ParseFile`)
- **Member variables:** `m_` prefix (`m_widgets`, `m_tokenStore`)
- **Local variables:** camelCase (`parsedRules`, `blockData`)
- **Constants:** `k` prefix (`kMaxModDataSize`, `kDefaultUpdateRate`)
- **Enums:** PascalCase type, PascalCase values (`ErrorCode::ParseError`)
- **Test files:** `{Class}Tests.cpp`

**Note:** The `m_` member prefix diverges from CommonLibSSE-NG's
bare-name convention. This is an intentional project-level choice
for readability in a large codebase. Documented here so it isn't
flagged as inconsistency.

### String handling

- `std::string` and `std::string_view` for all internal strings
- `RE::BSFixedString` only at the GameBridge boundary (DLL target)
- USS property names, binding keys stored as `std::string`
- Never hold raw `const char*` from BSFixedString
  (see `.private/reference/commonlibsse-ng-gotchas.md`)

### Include order

1. Corresponding header (`#include "ohui/dsl/USSParser.h"`)
2. Project headers (`#include "ohui/core/Result.h"`)
3. Third-party headers (`#include <yoga/Yoga.h>`)
4. Standard library headers (`#include <vector>`)

Blank line between each group. CorePCH/PCH covers standard library
and spdlog; don't re-include those in individual files unless needed
for clarity in headers that may be read standalone.

---

## 6. Architecture.md

Replace the current template placeholder with a real document. Key
sections (skeleton filled in at TASK-000, grows with each phase):

1. **Layer model** — four layers:
   - **Game Engine** — Skyrim + SKSE + CommonLibSSE-NG
   - **Bridge** — `GameBridge.cpp`, `main.cpp` (concrete implementations
     of narrow interfaces, SKSE lifecycle)
   - **Core** — `ohui_core` static library (all logic, no game dependencies)
   - **Presentation** — Scaleform SWFs, USS skin files, DSL definitions

   Dependency direction: Core never imports Bridge. Bridge imports
   Core (to wire implementations to interfaces). Presentation is
   data consumed by Core.

2. **Module map** — the `src/` and `include/ohui/` structure from §2
   with one-line descriptions per module

3. **Build targets** — `ohui_core` (static, testable), `OHUI` (DLL,
   ships), `ohui_tests` (test executable)

4. **Data flow** — how a game state change (e.g., player takes damage)
   flows: Game Engine → IPlayerState → DataBindingEngine → Widget →
   Compositor → IScaleformBridge → Scaleform render

5. **Extension points** — where mod authors plug in (widget registration,
   binding registration, skin files, MCM2 API, persistence API)

6. **Subsystem dependency graph** — matches TASKS.md dependency graph
   but at code/module level

---

## 7. TASK-000 · Project Scaffold

Everything in this document lands as TASK-000, executed before any
Phase 0 task begins. **(v2)** Execute in ordered sub-steps with
clean commits at each stage.

### Step 1: Directory restructuring

- Move `screens/` → `.private/screens/`
- Move `systems/` → `.private/systems/`
- Update all path references in CLAUDE.md, TASKS.md, DESIGN.md,
  compatibility.md, mod-compatibility-checklist.md
- Verify no broken references remain
- **Commit**

### Step 2: Source module scaffold

- Create `include/ohui/core/` with `IFileSystem.h`, `Result.h`,
  `Types.h`, `Log.h` (stubs with correct namespaces and include guards)
- Create `include/ohui/cosave/`, `include/ohui/dsl/`,
  `include/ohui/layout/` with stub headers
- Create matching `src/core/`, `src/cosave/`, `src/dsl/`,
  `src/layout/` with stub `.cpp` files
- Create `include/CorePCH.h`
- **Commit**

### Step 3: CMake restructuring

- Restructure CMakeLists.txt: `ohui_core` static lib target +
  `OHUI` shared lib target
- `ohui_core` uses `CorePCH.h`, links spdlog only
- `OHUI` uses `PCH.h`, links `ohui_core` + CommonLibSSE
- Explicit SOURCES/HEADERS manifests
- Verify both targets build successfully
- **Commit**

### Step 4: Test framework

- Add `catch2` to `vcpkg.json`
- Create `tests/CMakeLists.txt`
- Create `tests/main.cpp` (Catch2 main)
- Create `tests/SanityTests.cpp` (trivial test: `1 + 1 == 2`)
- Create `tests/mocks/MockFileSystem.h`
- Create `tests/fixtures/.gitkeep`
- Wire `add_subdirectory(tests)` into root CMakeLists.txt
- Verify `ctest` runs and passes
- **Commit**

### Step 5: Conventions and Architecture docs

- Create `.private/reference-overrides/ohui-conventions.md` (§5 content)
- Rewrite `docs/Architecture.md` with real skeleton (§6 content)
- **Commit**

### Step 6: Deploy script and CI updates

- Update `scripts/deploy.ps1` with resilient test gate (§3)
- Update `.github/workflows/release.yml` with test step
- Verify deploy script works with and without test binary present
- **Commit**

### Definition of done (TASK-000 complete)

- `screens/` and `systems/` live under `.private/`
- All document path references updated
- `include/ohui/core/` exists with `IFileSystem.h`, `Result.h`,
  `Types.h`, `Log.h`
- `include/CorePCH.h` exists (standard lib + spdlog, no CommonLibSSE)
- `ohui_core` static library target builds
- `OHUI` DLL target builds
- `ohui_tests` executable builds and `ctest` passes
- `tests/mocks/MockFileSystem.h` exists
- `.private/reference-overrides/ohui-conventions.md` exists
- `docs/Architecture.md` has real content
- `scripts/deploy.ps1` has resilient test gate
- CI workflow has test step

After TASK-000, every subsequent task has: a known directory for its
code, a test framework to write against, conventions to follow, mock
interfaces to test through, and a deploy gate that catches regressions.
No task invents structure.
