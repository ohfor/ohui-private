# OHUI — Design Document

> A complete UI framework for Skyrim SE/AE/VR. Not a SkyUI reskin. Not a parallel ecosystem.
> A proper foundation that should have existed from the start.

---

## The Problem

Skyrim's UI modding is broken at a structural level. The root cause is that every UI mod is a
monolithic file that wholesale replaces a vanilla menu. Two mods touching the same menu
conflict. There is no composition. The result is that players who want a complete, modern UI
experience must maintain a fragile patchwork of 10–15 independent mods — TrueHUD, SkyHUD,
moreHUD, Wheeler, QuickLoot, Compass Navigation Overhaul, STB Widgets, Detection Meter — none
of which were designed to coexist, all of which have their own visual language, configuration
system, and update cadence.

This is not a problem that can be solved by yet another reskin of SkyUI, or by a parallel
framework that requires 100% ecosystem adoption to be useful. It requires replacing the single
thing everything depends on, while honouring the contract that everything has been built against.

**SkyUI is infrastructure. OHUI replaces the infrastructure.**

---

## Vision

A single framework that owns the entire Skyrim UI surface area:

- **For players**: a canvas of independent widgets that can be dragged, resized, shown, hidden,
  and reskinned in a first-class edit mode. Visual consistency guaranteed, not negotiated.
- **For mod authors**: a declarative DSL and clean C++ API. No legacy tooling. No monolithic
  file conflicts.
- **For the ecosystem**: full backwards compatibility with existing SkyUI-dependent mods from
  day one. Users can switch without waiting for mod authors to update.

---

## Design Decisions

### Design Feature 1 — SkyUI Backwards Compatibility

OHUI must support the majority of existing SkyUI-dependent mods, especially MCM integrations,
without requiring mod authors to update their mods. Users must be able to drop OHUI in as a
replacement and have their existing mod list continue to work.

The MCM contract is well-defined and finite — a limited set of control types, a registration
system, and a set of callbacks that fire at predictable moments. It can be mapped exhaustively
and honoured completely. SkyUI's source is available and the contract can be reverse-engineered
without ambiguity.

This is not optional. It is a launch requirement.

### Design Feature 2 — Widget Runtime

The gameplay HUD is not a fixed layout. It is a **canvas of independent widgets**. Each widget
is self-contained, independently positioned, and independently skinnable.

Players can enter an edit mode (WoW-style) to:
- Drag widgets to any position
- Resize widgets
- Show or hide individual widgets
- Swap widget skins (e.g. swap "Health Bar — Default" for "Health Bar — Nordic UI")
- Save and load layout profiles

**Skins are strictly decoupled from widget data contracts.** A skin is a visual implementation
of a widget's data interface. The widget defines what data it needs. The skin defines how to
present it. These are separate concerns and separate artifacts.

### Design Principle — Strict Layer Separation

Layers must not leak into each other. This is a non-negotiable architectural discipline,
enforced by constraint not convention.

```
┌─────────────────────────────────────────────────────────┐
│                    Player / User Layer                   │
│         Edit mode, layout profiles, skin selection       │
├─────────────────────────────────────────────────────────┤
│                      Widget Layer                        │
│    Independent widgets with defined data contracts.      │
│    Skins implement the visual side of those contracts.   │
├─────────────────────────────────────────────────────────┤
│                  Widget DSL / Skin Layer                 │
│    Declarative, game-agnostic DSL.                       │
│    Describes widget structure, properties, bindings,     │
│    states and layout. Contains zero game-specific logic. │
├─────────────────────────────────────────────────────────┤
│               Data Binding Schema Layer                  │
│    Game-specific property manifest. Declares what the    │
│    runtime exposes to widgets (player.health.current,    │
│    player.shout.cooldown, etc.). One schema per game.    │
│    Skyrim schema, FO4 schema, Starfield schema, ESVI     │
│    schema. The DSL itself is unaware of these.           │
├─────────────────────────────────────────────────────────┤
│                    Runtime Layer (C++)                   │
│    CommonLib/SKSE plugin. Widget registry, data          │
│    binding engine, layout persistence, game data         │
│    polling, renderer bridge, Papyrus interface,          │
│    MCM compatibility shim.                               │
├─────────────────────────────────────────────────────────┤
│                      Game Engine                         │
│              Skyrim / FO4 / Starfield / ESVI             │
└─────────────────────────────────────────────────────────┘
```

The most important consequence: **the DSL must contain no Skyrim-specific concepts.** A shout
cooldown meter, a radiation bar, an oxygen gauge — these are data binding schema concerns, not
DSL concerns. When in doubt about where something belongs: if it would need to change when
porting to a different Bethesda game, it belongs in the data binding schema layer, not the DSL.

---

## Architecture Overview

OHUI has two distinct top-level surfaces — the **HUD** and the **UI** — which differ in
lifecycle and input ownership but share the same rendering substrate.

**HUD** — persistent, non-modal, always present during gameplay. Input passes through to the
game. The widget runtime, edit mode, and layout profiles belong here.

**UI** — modal, stacked, input-capturing. When a menu opens, input is owned entirely by that
menu. All menu screens belong here.

```
┌────────────────────────────────┬────────────────────────────────┐
│              HUD               │               UI               │
│  Persistent. Non-modal.        │  Modal. Stacked. Input-        │
│  Pass-through input.           │  capturing. Dismissible.       │
│  Widget runtime + edit mode.   │  All menu screens. MCM.        │
├────────────────────────────────┴────────────────────────────────┤
│                       Component Library                         │
│        Shared by HUD and UI. One component set,                 │
│        consistent design token system, usable anywhere.         │
├─────────────────────────────────────────────────────────────────┤
│                    USS + Yoga (Style / Layout)                   │
│   USS for visual properties and design tokens.                  │
│   Yoga (MIT) for Flexbox layout geometry.                       │
├─────────────────────────────────────────────────────────────────┤
│                     Widget Rendering                            │
│   DSL Runtime (primary) · Native C++ (specialist)               │
│   Third-party viewport contract (open)                          │
├─────────────────────────────────────────────────────────────────┤
│                    Data Binding Engine                          │
│        Routes live game state to widgets via schema.            │
├─────────────────────────────────────────────────────────────────┤
│                    Data Binding Schema                          │
│        Game-specific property manifest. One per game.           │
├─────────────────────────────────────────────────────────────────┤
│                   Runtime / SKSE Plugin                         │
│   Widget API · MCM compat · MCM2 · Input · Persistence          │
├─────────────────────────────────────────────────────────────────┤
│                 Rendering Backend (Scaleform)                   │
│   Swappable. Invisible above the renderer abstraction layer.    │
└─────────────────────────────────────────────────────────────────┘
```

Full detail on each layer is in the relevant systems documents.

---

## Portability

The layer separation principle enables portability across Bethesda games.

| Layer | Skyrim | FO4 | Starfield | ESVI |
|-------|--------|-----|-----------|------|
| Widget DSL | ✅ | ✅ | ✅ | ✅ |
| Widget definitions | ✅ | ✅ | ✅ | ✅ |
| Widget skins | ✅ | ✅* | ✅* | ✅* |
| Data binding schema | Skyrim | FO4 | Starfield | ESVI |
| Runtime | Port required | Port required | Port required | Port required |

*Skins port when the target game's schema exposes compatible property names.

Porting to a new game requires a new runtime target and a new data binding schema.
The DSL, widget definitions, and skin ecosystem travel unchanged.

---

## Versioning and Compatibility

OHUI's public contracts — the DSL, the data binding schema interface, the widget viewport
contract, the input action API — are stable by default. Backwards compatibility is a
first-class design commitment. Existing widgets and skins must not break when OHUI updates.

When a breaking change is unavoidable the cosave versioned migration pattern applies.
Widget and skin authors declare minimum OHUI version requirements explicitly:

```
widget MyWidget {
    requires ohui >= 1.3;
    ...
}
```

---

## Update Resilience

**OHUI depends on SKSE and nothing else.**

SKSE is an unavoidable dependency. That dependency is accepted. What is not accepted is
expanding the surface area beyond it. No additional native hooks outside of SKSE. No
dependency on other mods with fragile update surfaces. OHUI's vulnerability window on a
game update is exactly SKSE's vulnerability window — no larger.

When evaluating any implementation decision that could be handled via SKSE or via an
additional native dependency, SKSE wins. Always.

---

## Public API

The public API is a projection of the DSL — the same concepts the DSL expresses, made
accessible programmatically. It is documented once the DSL design has stabilised.

The known integration seams are covered in their respective systems documents:
- Widget viewport contract → `.private/systems/hud-widget-runtime.md`
- Data binding schema interface → `.private/systems/dsl-and-skinning.md`
- Input action registration → `.private/systems/input-handling.md`
- MCM compatibility shim → `.private/systems/mcm2.md`
- MCM2 declarative config → `.private/systems/mcm2.md`
- Mod integration model → `.private/systems/mod-registration-api.md`
- Cosave / persistence → `.private/systems/cosave-persistence.md`

---

## Relationship to SCIE and STOR

OHUI is independent infrastructure. Both projects benefit directly:

- **SCIE**: MCM2 and the inventory menu replacement handle the UI pain points directly.
- **STOR**: tree widgets and proper panel UI replace the current MessageBox chain workarounds.
- **Both**: one shared dependency covers all UI needs for the ohfor mod suite.

---

## Document Map

**Top-level:**
- `DESIGN.md` — this document. Vision, principles, architecture overview.
- `.private/compatibility.md` — soft dependency philosophy, graceful degradation, frozen giants.
- `.private/mod-compatibility-checklist.md` — specific mod compatibility status.

**Systems:**
- `.private/systems/hud-widget-runtime.md` — widget canvas, edit mode, widget contract.
- `.private/systems/component-library.md` — full component and token library.
- `.private/systems/dsl-and-skinning.md` — DSL, USS/Yoga, rendering paths, skin system.
- `.private/systems/data-binding-schema.md` — named binding catalogue, poll sources, custom bindings.
- `.private/systems/input-handling.md` — context stack, action binding, button prompts.
- `.private/systems/localisation.md` — string format, pluralisation, RTL, font system.
- `.private/systems/performance-model.md` — update modes, dirty flagging.
- `.private/systems/mcm2.md` — MCM compatibility layer and declarative native API.
- `.private/systems/icon-system.md` — item/spell/perk/status icon resolution and atlases.
- `.private/systems/mod-registration-api.md` — mod integration model (engine reads, frozen giants, widget registration).
- `.private/systems/cosave-persistence.md` — cosave format, persistence API, lifecycle.
- `.private/systems/message-log.md` — unified message log system.
- `.private/systems/actor-viewport.md` — clone-based actor rendering for dialogue, inventory, and load game.
- `.private/systems/outfit-system.md` — named equipment sets for player and followers.
- `.private/systems/new-game-plus.md` — PARKED. New Game+ feature, isolated pending further design.

**Screens:**
- `.private/screens/inventory-barter-container.md`
- `.private/screens/magic-spells-shouts.md`
- `.private/screens/crafting.md`
- `.private/screens/map.md`
- `.private/screens/quest-journal.md`
- `.private/screens/skills.md`
- `.private/screens/dialogue.md`
- `.private/screens/wait-sleep.md`
- `.private/screens/level-up.md`
- `.private/screens/stats.md`
- `.private/screens/tween-menu.md`
- `.private/screens/loading-screen.md`
- `.private/screens/main-menu.md`
- `.private/screens/load-game.md`
- `.private/screens/gift-menu.md`
- `.private/screens/favourites-radial.md`
- `.private/screens/edit-mode.md`
