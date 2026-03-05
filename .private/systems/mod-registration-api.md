# Mod Integration Model

## Principle

OHUI is a UI framework. It renders what the engine and game data
provide. Mods should not need to register with OHUI for their content
to appear correctly — OHUI reads game data directly.

There are three categories of mod integration:

---

## Engine Data (no mod action needed)

OHUI reads game data from the engine. This covers the vast majority
of mod content automatically:

- Spells and spell schools — read from magic effect data
- Perks and skill trees — read perk tree structure, including
  mod-added perks (Ordinator, Vokrii, etc.)
- Recipes — read from the engine, including overhaul-added recipes
  (CACO, CCOR, etc.)
- Dialogue options — read from the engine, including mod-added dialogue
- Stats — engine-provided values
- Items, enchantments, ingredients — engine data
- Quest data — engine-provided quest stages and objectives

Mods that add content through standard engine mechanisms (ESP/ESM)
appear in OHUI's screens without any awareness of OHUI.

---

## Frozen Giants (baked-in native support)

Some mods are so widely installed and so inactive that OHUI ships
with specific knowledge of their data baked in. Their data appears
in OHUI correctly, styled consistently, without any mod author action.

This is a permanent maintenance commitment per mod. The list is kept
deliberately short. Each addition is decided case by case based on
install count, mod activity level, and whether OHUI surfaces genuinely
benefit from native support.

The current list and per-mod integration details are documented in
`compatibility.md`.

---

## Custom Widgets (registration required)

Mod authors who want to add new HUD widgets to the canvas must
register them with OHUI. This is the only integration point where
a mod needs to actively communicate with OHUI — the engine has no
concept of HUD widgets, so there is nothing for OHUI to discover
automatically.

A widget registration provides the manifest (ID, display name,
default position/size, min/max constraints, default visibility) and
the widget's rendering implementation. Once registered, the widget
participates fully in the canvas — edit mode, layout profiles,
skin support.

---

## Custom Data Bindings

Mods that expose game state values OHUI doesn't know about can
register custom bindings into the data binding schema. A survival
mod registers `mymod.player.hunger` as a Float. Any widget can then
bind to it.

This is not a mandatory registration — a mod that doesn't register
bindings simply has no custom data available to widgets. The game
works fine. OHUI's built-in bindings cover all engine data.

Custom bindings are the mechanism by which frozen giant integrations
provide their data — OHUI's baked-in support registers the bindings
on behalf of the frozen giant mod.

---

## Soft Dependency Guarantee

Every integration point degrades gracefully when a mod is absent.
OHUI never hard-requires any mod. It checks, uses what it finds,
and backs down cleanly when something is missing. The full soft
dependency philosophy is documented in `compatibility.md`.
