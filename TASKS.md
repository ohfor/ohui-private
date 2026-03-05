# OHUI — Implementation Task Breakdown

Each task is scoped to be handed to Code as a single session.
Every task has a defined start state, end state, and definition of done.
Tasks within a phase can be parallelised unless a dependency is stated.

---

## Phase 0 — Foundation

These tasks have no prerequisites. They establish the substrate
everything else builds on.

---

### TASK-001 · Yoga Integration

**What:** Embed the Yoga layout engine into the OHUI C++ project and
expose a thin wrapper that OHUI's layout pass calls.

**Start state:** Empty OHUI project with SKSE scaffold.

**End state:** A layout module that accepts a tree of nodes with
USS-derived flex properties, runs layout, and returns computed
rectangles for each node. No rendering. Layout only.

**Definition of done:**
- Unit tests: column stack, row stack, nested flex, min/max
  constraints, padding, margin all produce correct geometry
- MIT licence notice for Yoga present in project

**Reference:** `systems/dsl-and-skinning.md` § Yoga

---

### TASK-002 · USS Parser

**What:** A parser that reads a `.uss` file and produces a typed
property bag per selector rule.

**Start state:** TASK-001 complete (Yoga integration, so layout
properties have a consumer).

**End state:** A USS parser that returns a structured rule set. Rules
contain: selectors (type, class, id, pseudo-class), and a property
map of standard USS properties plus the defined `-ohui-` extensions.
Invalid properties are logged and skipped. No rendering. No layout.
Parsing only.

**Definition of done:**
- Parses all standard USS visual properties
- Parses all `-ohui-` extension properties listed in
  `systems/dsl-and-skinning.md`
- Rejects unknown properties with a warning log, continues parsing
- Unit tests cover: selector specificity, cascade order, `var()`
  token resolution, pseudo-class states

**Reference:** `systems/dsl-and-skinning.md` § USS

---

### TASK-003 · Design Token System

**What:** The CSS custom properties (`--token-name`) store that USS
`var()` calls resolve against. Tokens defined in one file cascade
to all widgets.

**Start state:** TASK-002 complete (USS parser exists).

**End state:** A token store that loads token definitions from a
`.uss` file containing only custom property declarations. Resolves
token keys to values. The USS parser resolves `var()` calls through
the token store. Skin token overrides replace base token values
without touching other tokens.

**Definition of done:**
- Base token file loads correctly
- Skin token file overrides only declared keys, leaves rest unchanged
- `var(--undefined-token)` logs a warning and returns empty string
- Unit tests: token resolution, override layering, undefined key handling

**Reference:** `systems/dsl-and-skinning.md` § WPF/XAML as Reference Model

---

### TASK-004 · Cosave Read/Write

**What:** The cosave file format — header, block directory, typed
data blocks — with read and write paths.

**Start state:** Empty OHUI project with SKSE scaffold. No other
tasks required.

**End state:** A cosave manager that on game load reads an `.ohui`
file alongside the Skyrim save, validates the header and block
checksums, and exposes typed block accessors. On save writes the
cosave atomically. Empty blocks for all defined block types are
created on new game.

**Definition of done:**
- Round-trip test: write a cosave, read it back, all values match
- Atomic write: interrupted write (simulated) leaves previous cosave intact
- Unknown blocks are skipped without error
- Block version mismatch is logged as a warning, block is read
  with current parser (extra fields ignored)
- Migration infrastructure present: versioned migration chain applied
  before reads

**Reference:** `systems/cosave-persistence.md`

---

### TASK-005 · Persistence API

**What:** The mod-facing key-value persistence API backed by the
cosave Mod Data block.

**Start state:** TASK-004 complete (cosave read/write works).

**End state:** A persistence API accessible from both native code and
Papyrus. Typed get/set operations for bool, int, float, and string
values. Has, delete, and clear operations. Writes buffered and
flushed on cosave write. Namespace isolation enforced — mod A cannot
read mod B's keys. Size limits enforced (64KB per entry, 1MB per mod).
Violations logged, write rejected, no crash.

**Definition of done:**
- Read returns default value for missing key (no throw)
- Namespace isolation: confirmed cross-mod read returns default
- Size limit: oversized write is rejected and logged
- Papyrus bindings functional and tested via SKSE console

**Reference:** `systems/cosave-persistence.md` § The Persistence API

---

## Phase 1 — Widget Runtime

Requires Phase 0 complete.

---

### TASK-010 · Widget Registry

**What:** The registry that owns all widget registrations and
exposes the canvas to the compositor.

**Start state:** TASK-001, TASK-002, TASK-003 complete.

**End state:** A widget registry that accepts widget manifests
(id, display name, default position, default size, min/max size,
default visibility), stores them, and exposes the full canvas state.
The three operations — Activate, Move, Resize — update the canvas
state. No rendering. No data binding. Canvas management only.

**Definition of done:**
- Register widget, activate it, move it, resize it — canvas state
  reflects all changes correctly
- Duplicate registration logs a warning, does not crash
- Widget not found on Move/Resize logs a warning, does not crash

**Reference:** `systems/hud-widget-runtime.md`

---

### TASK-011 · Layout Profile Persistence

**What:** Save and load named layout profiles via the cosave.

**Start state:** TASK-004, TASK-010 complete.

**End state:** Layout profiles (complete canvas snapshots) are written
to the Layout Profiles cosave block on save and restored on load.
Save, load, list, and delete operations for named profiles. Built-in
profiles (Default, Minimal, Controller) are regenerated from compiled
defaults when missing. Profile switching is instant — canvas state
updated synchronously.

**Definition of done:**
- Save profile, reload game, load profile — positions/sizes/visibility
  all match
- Built-in profiles regenerate correctly when cosave is empty
- Profile not found on load returns false, logs warning, canvas unchanged

**Reference:** `systems/hud-widget-runtime.md` § Layout Profiles,
`systems/cosave-persistence.md` § Layout Profiles Block

---

### TASK-012 · Data Binding Engine

**What:** Routes live Skyrim game state to widgets via named binding
keys declared in the Skyrim data binding schema.

**Start state:** TASK-010 complete (widget registry exists and has
active widgets to bind to).

**End state:** A data binding engine that polls a defined set of
Skyrim game state values (player health, stamina, magicka, level,
shout cooldown — initial set from schema), detects changes, and
notifies subscribed widgets. Reactive and frame-driven update modes
both functional. Throttled reactive enforces declared rate.

**Definition of done:**
- Reactive widget: re-renders only when bound value changes, confirmed
  by render call counter in test
- Frame-driven widget: re-renders every frame
- Throttled reactive at 15fps: renders no more than 15 times per
  second regardless of how frequently the value changes
- Polling only activates for values with active subscriptions

**Reference:** `systems/performance-model.md`,
`systems/dsl-and-skinning.md` § Data Binding Schema

---

### TASK-013 · Edit Mode

**What:** The HUD edit mode — enter/exit, bounding box chrome,
drag and resize, auto-save on exit.

**Start state:** TASK-010, TASK-011, TASK-012 complete.

**End state:** Edit mode triggered by a dedicated keybind. On entry:
game pauses (configurable), all widgets gain bounding box chrome
(dotted outline, label, resize handles). Player can drag widgets
to new positions and drag resize handles to new sizes. On exit:
layout auto-saved to active profile. Chrome removed. Game resumes.

**Definition of done:**
- Drag moves widget to new position, position persists after reload
- Resize changes widget size within min/max constraints, persists
- Snap-to-grid optional (off by default), when on widgets snap to
  nearest 8px grid
- Exit saves layout to active profile immediately
- Widget cannot be dragged outside the canvas bounds

**Reference:** `systems/hud-widget-runtime.md` § Edit Mode

---

## Phase 2 — Input System

Requires TASK-010 (widget registry) to exist.

---

### TASK-020 · Input Context Stack

**What:** The context stack — push, pop, routing, exclusive active
context.

**Start state:** SKSE input hook available. No other OHUI tasks required.

**End state:** An input context stack with push and pop operations.
Only the top context receives input events. Physical key/button
events are routed to named action handlers in the active context.
No two handlers in the same context fire for the same physical
input simultaneously. Context not found on pop logs a warning.

**Definition of done:**
- Push context A, push context B — only B's handlers fire
- Pop context B — only A's handlers fire
- Push A, push B, pop A (wrong order) — warning logged, stack
  unchanged
- Confirmed: gameplay context at bottom always present

**Reference:** `systems/input-handling.md` § The Input Context Stack

---

### TASK-021 · Action Registration and Rebinding

**What:** Named action registration within contexts, default bindings,
and the player rebinding store.

**Start state:** TASK-020 complete.

**End state:** Action registration accepts context ID, action ID,
display name, default keyboard key, default controller button, and
handler. Default bindings applied on first run. Player rebinding
stored per-character in cosave. Binding map applied at context push
— no re-registration required on load.

**Definition of done:**
- Register action, trigger default key — handler fires
- Rebind to new key, trigger new key — handler fires
- Trigger old key after rebind — handler does not fire
- Papyrus action registration functional

**Reference:** `systems/input-handling.md` § Action-Based Binding

---

### TASK-022 · Button Prompts

**What:** The button prompt component — renders the correct glyph
for the active input device for a named action.

**Start state:** TASK-021 complete (actions are registered and have
bindings).

**End state:** A button prompt component that takes an action ID and
a label string. Renders the appropriate glyph (keyboard key, Xbox
button, PlayStation button, Steam Deck button) for the active input
device. Device switches instantly on input event from a different
device type. All visible prompts update.

**Definition of done:**
- Connect Xbox controller — all prompts show Xbox glyphs
- Switch to keyboard input — all prompts switch to key labels
- Unknown action ID logs warning, renders placeholder

**Reference:** `systems/input-handling.md` § Button Prompts

---

## Phase 3 — MCM Compatibility Layer

Requires Phase 0 complete. Independent of Phases 1 and 2.

---

### TASK-030 · MCM Registration Shim

**What:** The SkyUI MCM Papyrus API surface — `RegisterMCM`, `AddPage`,
`OnConfigInit`, `OnConfigOpen`, `OnConfigClose`, `OnPageReset`.

**Start state:** SKSE scaffold. TASK-004 (cosave) optional but
desirable for state persistence.

**End state:** Papyrus scripts can call SkyUI's MCM registration
functions and OHUI intercepts them, registers the mod in the MCM
list, and stores page structure. Registration callbacks fire at the
correct moments. Timing matches SkyUI exactly.

**Definition of done:**
- A real SkyUI MCM mod (e.g. SkyUI itself) registers without error
- Registration callback fires at the correct moment during load
- Page structure stored and retrievable from Registration State
  Inspector

**Reference:** `systems/mcm2.md` § Compatibility Layer

---

### TASK-031 · MCM Control Rendering

**What:** OHUI renders an MCM screen from SkyUI-registered control
definitions. All 8 control types functional.

**Start state:** TASK-030 complete.

**End state:** When a player opens an MCM, OHUI renders it using the
OHUI component library. All 8 control types functional: Toggle,
Slider, Menu, Text, Color, KeyMap, Header, Empty. Player interactions
fire the correct 11 callbacks with correct timing. `SetToggleOptionValue`
and equivalent live-update functions work without `ForcePageReset`.

**Definition of done:**
- SkyUI's own MCM opens and is fully functional
- All callbacks fire within same frame as player action (timing test)
- A mod with all 8 control types renders all controls correctly
- Live update: `SetToggleOptionValue` updates rendered value without
  ForcePageReset

Note on naming: SkyUI uses "Menu" (dropdown), "KeyMap" (keybind), and
"Color" in its Papyrus API. OHUI's internal components use "Dropdown",
"KeyBind", and "ColourPicker". Both names refer to the same controls.
The compatibility shim maps SkyUI names to OHUI names transparently.
MCM2 native API uses OHUI names exclusively.

**Reference:** `systems/mcm2.md` § Control Types, § Callbacks,
§ Live Updates

---

### TASK-032 · MCM List Management

**What:** Hide, rename, and reorder of MCM mod entries. MenuMaid2
feature set.

**Start state:** TASK-031 complete.

**End state:** Player can hide an MCM via context menu (hidden entries
filterable via "Show hidden" toggle). Player can rename an MCM (original
name in tooltip). Player can reorder via drag-and-drop. All state
persisted per-character in cosave MCM List Config block. Alphabetical
and most-recently-used sort available as alternatives.

**Definition of done:**
- Hide mod, reload — still hidden
- Rename mod, reload — renamed, original in tooltip
- Reorder, reload — order preserved
- "Show hidden" toggle reveals hidden entries

**Reference:** `systems/mcm2.md` § MCM List Management

---

### TASK-033 · MCM Developer Tools

**What:** Registration State Inspector, Force Callback, Refresh
Registry, Papyrus log surface.

**Start state:** TASK-031 complete.

**End state:** Maintenance section in OHUI settings (visually
separated, requires deliberate navigation). Registration State
Inspector shows all registered mods, timestamps, page structure,
callback history. Force Callback triggers OnConfigOpen/Close/PageReset
on demand. Refresh Registry re-runs registration pass. Papyrus log
surfaced as filterable channel in the message log.

**Definition of done:**
- Inspector shows correct registration data for a known mod
- Force callback triggers correctly and fires the handler
- Refresh registry picks up a mod registered after initial load

**Reference:** `systems/mcm2.md` § Developer and Maintenance Tools

---

## Phase 4 — MCM2 Native API

Requires Phase 3 complete.

---

### TASK-040 · MCM2 Definition Parser

**What:** Parse a declarative MCM definition file (YAML/DSL format)
into a typed MCM definition structure.

**Start state:** TASK-030 (MCM registration infrastructure).

**End state:** A definition parser that returns a typed MCM definition
containing pages, sections, controls with their types, defaults,
descriptions, conditions, and callback names. Syntax errors are
logged, parse is rejected, existing definition remains active.

**Definition of done:**
- All control types parse correctly (toggle, slider, dropdown,
  keybind, colour, text, header, empty)
- Unknown fields are ignored (forward compatibility)
- Syntax error: logged, rejected, existing definition unchanged
- Round-trip: parse then serialise produces identical output

**Reference:** `systems/mcm2.md` § Definition Format

---

### TASK-041 · MCM2 Reactive Conditions

**What:** Condition evaluation — control visibility and availability
driven by other control values and game state, evaluated reactively.

**Start state:** TASK-040 complete.

**End state:** Conditions declared in the definition (`condition:
enableFeature == true`) are evaluated against current control values.
When a control's value changes, all conditions that reference it
are re-evaluated and affected controls update immediately. No
`ForcePageReset`. State provider API allows mods to register custom
condition sources (e.g. `player.skill.smithing >= 50`).

**Definition of done:**
- Toggle A controls visibility of Slider B — toggling A immediately
  shows/hides B without any explicit refresh call
- Custom state provider registered and queryed correctly
- Circular condition references detected and logged, not executed

**Reference:** `systems/mcm2.md` § Conditions

---

### TASK-042 · MCM2 Persistence

**What:** Automatic persistence of declarative MCM control values
via the cosave MCM Values block.

**Start state:** TASK-040, TASK-004 complete.

**End state:** Control values from declarative MCMs are read from
and written to the cosave MCM Values block automatically. No mod
author persistence code required. Typed get operations return current
values. Orphaned entries (mod uninstalled) retained
for N sessions then pruned.

**Definition of done:**
- Set a value, save game, reload — value restored from cosave
- New game: defaults applied, all reads return declared defaults
- Orphaned entry count decrements each session without matching
  registration, deleted after N sessions

**Reference:** `systems/mcm2.md` § Persistence

---

### TASK-043 · MCM2 Hot Reload

**What:** File watcher detects definition file changes and applies
a delta to the live MCM without restart.

**Start state:** TASK-040, TASK-041, TASK-042 complete.

**End state:** File watcher watches the Data directory for `.mcm2`
definition files (off by default, enabled in Maintenance). On
detected change: re-parses definition, validates, diffs against
current state, applies delta — new controls appear, removed
disappear, changed update in place. Destructive changes (id rename,
mcm id change) detected, logged, flagged in Inspector. No restart.

**Definition of done:**
- Add a control to definition file, save — control appears in
  running game within 3 seconds
- Remove a control — disappears in running game
- Change label — label updates in running game
- Change control id — logged as destructive, existing value lost,
  new control added with default

**Reference:** `systems/mcm2.md` § Hot Reload

---

## Phase 5 — Screens

Each screen is an independent task. All require Phase 1 (widget
runtime) and Phase 2 (input) complete. Screens that share the
FacetedList component should implement it once in the component
library first (see TASK-050).

---

### TASK-050 · FacetedList Component

**What:** The shared filtered list component used by inventory,
barter, container, magic, and crafting screens.

**Start state:** Phase 1 complete. Component library scaffold exists.

**End state:** FacetedList component. SearchField always visible.
PresetBar of named quick-filter combinations. FacetPanel (drawer on
controller, sidebar on mouse) with multi-select facet groups.
ActiveFilterChips below preset bar. Virtualised ScrollList — smooth
scroll at any list size. Facets registered at construction by the
host screen. External facet registration via mod API. Filter state
is additive — player starts with everything and narrows.

**Definition of done:**
- 1000-item list scrolls smoothly at 60fps
- Search field filters list in real time
- Multiple facets active simultaneously, chips show each active
  facet, tapping chip removes that facet
- External facet registered by a test mod appears in FacetPanel

**Reference:** `systems/component-library.md` § FacetedList,
`screens/inventory-barter-container.md`

---

### TASK-051 · Inventory Screen

**Start state:** TASK-050 complete.
**End state:** Inventory screen fully functional with FacetedList,
item detail panel, sort controls, all item categories, and all
item action verbs (equip, drop, favourite, inspect).
**Definition of done:** Player can open inventory, filter by type and
condition, select and equip an item, drop an item, mark favourite.
Provenance stamp visible in detail panel for stamped items.

---

### TASK-052 · Barter Screen

**Start state:** TASK-050 complete.
**End state:** Player inventory and merchant inventory side by side.
Offer panel. Gold/value display. Transaction confirm. Sentimental
item protection fires on attempt to add sentimental item to offer.
**Definition of done:** Complete a buy, complete a sell, attempt to
sell sentimental item and receive confirmation prompt.

---

### TASK-053 · Container Screen

**Start state:** TASK-050 complete.
**End state:** Player inventory and container inventory side by side.
Take All (skips sentimental). Transfer individual items. Search
across both lists.
**Definition of done:** Take All skips sentimental items. Individual
transfer works both directions.

---

### TASK-054 · Magic Screen

**Start state:** TASK-050 complete.
**End state:** Spells, powers, shouts and lesser powers in FacetedList.
Equip to left/right hand. Favourite. Shout section shows word unlock
and cooldown state.
**Definition of done:** Equip spell to both hands, favourite a shout,
filter by school.

---

### TASK-055 · Crafting Screens

**Start state:** TASK-050 complete.
**End state:** Smithing forge (craft), smithing workbench (temper),
alchemy lab, and enchanting table. All functional. Provenance stamp
written on confirmed craft action. Dedication field presented after
confirmation.
**Definition of done:** Craft an item, receive stamp with correct
skill level and date. Dedication entered and stored. Stamp visible
in inventory detail panel.

---

### TASK-056 · Quest Journal

**Start state:** Phase 1, Phase 2 complete.
**End state:** Active and completed quests. Quest stages and objectives.
Map marker integration. Sort by faction, type, date.
**Definition of done:** All active quests visible, objectives correct,
completed quests accessible.

---

### TASK-057 · Map Screen

**Start state:** Phase 1, Phase 2 complete.
**End state:** World map with markers, zoom, fast travel.
**Definition of done:** Open map, place custom marker, fast travel
to a known location.

---

### TASK-058 · Skills Screen

**Start state:** Phase 1, Phase 2 complete. Investigation into
constellation rendering approach complete.
**End state:** All 18 skill trees functional. Perk acquisition
functional. Level up perk points correctly tracked.
**Definition of done:** Spend a perk point, confirm tree updates.

---

### TASK-059 · Dialogue Screen

**Start state:** Phase 1, Phase 2 complete.
**End state:** Topic list, subtitles, speaker attribution, full
sentence preview option.
**Definition of done:** Complete a branching dialogue tree without
missing any response options.

---

### TASK-060 · Level Up Screen

**Start state:** Phase 1, Phase 2 complete.
**End state:** Attribute selection (Health/Magicka/Stamina), skill
summary for the level, perk point award notification.
**Definition of done:** Level up, select attribute, confirm point
awarded and attribute increased.

---

### TASK-061 · Main Menu

**Start state:** Phase 1, Phase 2 complete.
**End state:** Main menu, New Game, New Game+ flow, Settings entry
point.
**Definition of done:** New game starts, New Game+ configuration
screen accessible, settings opens.

---

### TASK-062 · Load Game Screen — Character Gallery

**Start state:** TASK-061 complete. TASK-004 (cosave) complete.
**End state:** Character card gallery. Cards sourced from cosave
and save file metadata (no save load required). Portrait from
captured render stored in cosave. Most recently played character
selected by default.
**Definition of done:** Two characters present in save folder, both
cards display with correct name/level/location/portrait. Most recent
is pre-selected.

---

### TASK-063 · Load Game Screen — Save Browser

**Start state:** TASK-062 complete.
**End state:** Per-character save list. Reverse chronological. Rich
entry metadata (location, date, level, active quest, playtime).
Mod list delta indicator. Filter bar. Context menu with rename and
delete (with confirmation).
**Definition of done:** Select character, see all saves. Mod list
delta shows correctly for a save with a missing mod. Manual save
rename persists after reload.

---

### TASK-064 · Portrait Capture

**Start state:** TASK-062 (load game screen) complete. TASK-012
(data binding, character state accessible) complete.
**End state:** At every save event, OHUI renders a 512×512 portrait
of the player character in a neutral pose with three-point lighting.
Captured texture compressed and stored in cosave. Load game screen
reads texture and displays on character card.
**Definition of done:** Save game, reload — character card shows
portrait matching character's current appearance and outfit.

---

### TASK-065 · Remaining Screens

**Start state:** Phase 1, Phase 2 complete.
Covers: Wait/Sleep screen, Loading screen, Stats screen, Tween menu,
Favourites radial, Gift menu, Edit mode chrome.
Each is small enough to batch into one session. Split further if any
proves larger than expected.

---

## Phase 6 — Provenance Stamps

Requires TASK-055 (crafting screens) and TASK-004 (cosave).

---

### TASK-070 · Stamp Write

**What:** Write provenance stamps at craft, temper, enchant, brew,
and cook trigger points.

**Start state:** TASK-055 complete. TASK-004 complete.

**End state:** A stamp manager writes a stamp to the cosave Stamped
Items block at each crafting trigger. Stamp contains all fields
defined in the schema. Item instance IDs generated at first stamp.
Sentimental flag settable via the API.

**Definition of done:**
- Craft item — stamp present in cosave with correct skill level,
  date, character name
- Temper item — second stamp added to history
- Sentimental flag set — item protected from bulk operations

**Reference:** `systems/provenance-stamps.md`

---

### TASK-071 · Stamp Read and API

**What:** The read API surface — internal OHUI reads and the
external mod-facing Papyrus/C++ API.

**Start state:** TASK-070 complete.

**End state:** Stamp read API functional from both native code and
Papyrus — query history, crafted status, sentimental status, and
dedication. Inventory detail panel reads stamp history and renders
it in the collapsible section.

**Definition of done:**
- Crafted item — detail panel shows stamp history
- Stamp API correctly identifies crafted vs looted items
- External mod reads dedication via API correctly

---

## Phase 7 — Localisation

Can begin as soon as Phase 0 is complete. Independent of all other
phases, but blocks release of any user-visible text.

---

### TASK-080 · String Definition and Resolution

**Start state:** Phase 0 complete.
**End state:** String definition format parsed. String resolution
returns translated string for active language, falling back to
definition default. Plural form selection correct for all CLDR rule
sets covering Skyrim's 13 languages.
**Definition of done:** English, Russian (complex plurals), and
German strings resolve correctly. Missing key returns default, not
raw key.

---

### TASK-081 · RTL Layout

**Start state:** TASK-001 (Yoga) complete.
**End state:** Yoga `direction: RTL` applied when active language
is RTL. Semantic directional properties (`inline-start/end`)
resolve correctly for both LTR and RTL. Visual test: a simple
two-column layout mirrors correctly in RTL.
**Definition of done:** LTR layout correct. RTL layout mirrors
correctly. No hardcoded `left`/`right` in any first-party component.

---

### TASK-082 · Font Stack

**Start state:** TASK-080 complete.
**End state:** Noto Sans (full) and Noto Emoji loaded. Three-level
font stack resolution: skin display font → Noto Sans → Noto Emoji.
No tofu boxes for any codepoint in Noto's coverage. Colour emoji
rendered where CBDT/COLRv1 supported, monochrome otherwise.
**Definition of done:** Arabic string renders without tofu. Japanese
string renders without tofu. Emoji codepoint renders. Skin with
Latin-only font falls back to Noto for Cyrillic.

---

## Dependency Graph (summary)

```
Phase 0 (TASK-001–005)
  └── Phase 1 (TASK-010–013)  ← widget runtime
        └── Phase 2 (TASK-020–022)  ← input
              └── Phase 5 (TASK-050–065)  ← screens
                    └── Phase 6 (TASK-070–071)  ← stamps

Phase 0
  └── Phase 3 (TASK-030–033)  ← MCM compat
        └── Phase 4 (TASK-040–043)  ← MCM2 native

Phase 0
  └── Phase 7 (TASK-080–082)  ← localisation (parallel to all)
```

Phases 3–4 (MCM) are fully independent of Phases 1–2 (widget runtime).
They can be built in parallel by separate Code sessions.
Phase 7 (localisation) is independent of everything and can run
alongside any phase.

---

## Not Yet Tasked

The following design documents exist but have not been broken into
implementation tasks. They depend on implementation decisions not
yet made or on prior phases being proven in production first.

- `systems/message-log.md` — unified message log
- `systems/mod-registration-api.md` — facet, widget, and HUD
  registration extension points
- DSL parser/runtime — the engine that reads widget definition files
  and drives rendering; architectural spine, needs a dedicated task
- First-party HUD widgets — 12+ widgets (health/stamina/magicka bars,
  compass, enemy health, sneak eye, shout meter, notification toast,
  etc.) each need a build task once the DSL runtime exists
- UIExtensions compatibility shim — fully specified in compatibility.md,
  no implementation task written
- Frozen giant native integrations — 9 mods, each needs a task
- New Game+ flow — depends on cosave and multiple screen phases
- DSL tooling — syntax highlighting, error reporting, validation
  toolchain for mod authors
- Data binding schema implementation — `systems/data-binding-schema.md`
  is now written; needs a corresponding TASK entry
