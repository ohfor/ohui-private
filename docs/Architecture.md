# OHUI Architecture

## Layer Model

OHUI uses strict layer separation. Dependencies flow downward only.
Core never imports Bridge. Bridge imports Core to wire implementations
to interfaces. Presentation is data consumed by Core.

```
┌─────────────────────────────────────────────────┐
│               Game Engine                        │
│   Skyrim + SKSE + CommonLibSSE-NG                │
├─────────────────────────────────────────────────┤
│               Bridge                             │
│   GameBridge.cpp, main.cpp                       │
│   Concrete implementations of narrow interfaces  │
│   SKSE lifecycle, logging, hook installation      │
├─────────────────────────────────────────────────┤
│               Core (ohui_core)                   │
│   All logic. No game engine dependencies.         │
│   Testable offline via narrow interface mocks.    │
├─────────────────────────────────────────────────┤
│               Presentation                       │
│   Scaleform SWFs, USS skin files, DSL definitions │
│   Data consumed by Core, not code.                │
└─────────────────────────────────────────────────┘
```

## Module Map

All source lives under `src/` with headers in `include/ohui/`.

| Module | Purpose |
|--------|---------|
| `core/` | Shared types, error handling (`Result<T>`), logging (`Log.h`), filesystem interface, game bridge |
| `cosave/` | Cosave file format, read/write, persistence API |
| `dsl/` | USS parser, design token store, DSL parser (Phase 3) |
| `layout/` | Yoga layout engine wrapper |
| `widget/` | Widget registry, data binding engine, edit mode (Phase 1) |
| `input/` | Input context stack, action routing (Phase 2) |
| `render/` | Scaleform bridge, compositor, component renderer (Phase 3+) |
| `mcm/` | MCM compatibility layer, MCM2 native API (Phase 4-5) |
| `screens/` | Individual screen implementations (Phase 7+) |
| `integration/` | Mod integration, frozen giant support (Phase 9+) |

Only `core/`, `cosave/`, `dsl/`, and `layout/` exist at project scaffold.
Other modules are created by their respective phases.

## Build Targets

| Target | Type | PCH | Links | Purpose |
|--------|------|-----|-------|---------|
| `ohui_core` | Static library | `CorePCH.h` | spdlog | All logic, no game engine. Tests link against this. |
| `OHUI` | Shared library (DLL) | `PCH.h` | ohui_core, CommonLibSSE | Ships to users. Entry point + bridge. |
| `ohui_tests` | Executable | Reuses ohui_core | ohui_core, Catch2 | Unit and contract tests. |

`ohui_compile_options` is an INTERFACE library providing shared MSVC
flags (`/W4 /permissive- /Zc:__cplusplus /Zc:preprocessor`) to all
three targets.

## Data Flow

How a game state change flows through the system (placeholder, grows
with Phase 1):

```
Game Engine
  → IPlayerState (narrow interface)
    → DataBindingEngine (detects change, notifies subscribers)
      → Widget (updates bound properties)
        → Compositor (layout + draw calls)
          → IScaleformBridge (renders to screen)
```

## Extension Points

Where mod authors integrate with OHUI (placeholder, grows with
Phase 1+):

| Extension | Mechanism |
|-----------|-----------|
| Custom HUD widgets | Widget registration API (Phase 9) |
| Custom data bindings | Binding registration API (Phase 9) |
| Visual skins | USS skin files + token overrides |
| MCM2 config | Declarative `.mcm2` definition files (Phase 5) |
| Persistent data | Persistence API key-value store (Phase 0) |

## Subsystem Dependency Graph

Matches the phase dependency graph in `TASKS.md`:

```
Phase 0 (Foundation: layout, DSL, cosave, core)
  ├── Phase 1 (Widget Runtime)
  │     ├── Phase 2 (Input System)
  │     │     └── Phase 6 (Component Library)
  │     │           └── Phase 7 (Screens)
  │     └── Phase 9 (Mod Integration)
  ├── Phase 3 (DSL Runtime)
  │     ├── Phase 6 (merges here)
  │     ├── Phase 8 (Message Log)
  │     └── Phase 14 (HUD Widgets)
  ├── Phase 4 (MCM Compat) → Phase 5 (MCM2)
  ├── Phase 10 (Localisation, parallel)
  └── Phase 11 (Compatibility Layers)
```

## Compatibility

### Requirements
- SKSE64 (matching game version)
- Address Library for SKSE Plugins
- (VR) VR Address Library + Skyrim VR ESL Support (if ESPs are ESL-flagged)

### Known Incompatibilities

None yet. See `.private/compatibility.md` for the full compatibility
model and `.private/mod-compatibility-checklist.md` for specific mod
evaluation status.
