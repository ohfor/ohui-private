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

### TASK-014 · Data Binding Schema Implementation

**What:** Implement the full Skyrim data binding schema — all binding
IDs defined in `systems/data-binding-schema.md` — with their poll
sources, types, and poll rates.

**Start state:** TASK-012 complete (data binding engine exists).

**End state:** All built-in bindings from the schema are registered
and polling live game state via SKSE. Player vitals, stats, skills,
state, location, equipped items, enemy target, time, and world
bindings all functional. Poll rates per the schema table.

**Definition of done:**
- `player.health.current` updates in real time during combat
- `player.level` updates on level-up event
- `enemy.name` returns target name when crosshair is on an NPC,
  returns empty string when no target
- `time.hour` tracks game time correctly
- All 18 skill bindings return correct values
- Custom binding registration API functional: a test mod registers
  a custom binding and a test widget reads it

**Reference:** `systems/data-binding-schema.md`

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

## Phase 3 — DSL Runtime

Requires Phase 0 (TASK-001–003) and TASK-012 (data binding engine).
This is the architectural spine — blocks all DSL-defined widgets,
all first-party HUD widgets, and all skin authoring.

---

### TASK-025 · DSL Parser

**What:** Parse `.widget` definition files into a typed AST. The DSL
syntax defined in `systems/dsl-and-skinning.md`.

**Start state:** TASK-002 (USS parser) complete.

**End state:** A parser that reads widget definition files and
produces a typed AST containing: widget name, requires clause,
property declarations with types, component tree with classes and
property bindings, animate blocks, skin definitions with token
overrides and optional ControlTemplate.

**Definition of done:**
- Parses the HealthBar example from dsl-and-skinning.md correctly
- Parses a skin definition with token overrides only
- Parses a skin definition with full ControlTemplate
- Parses `requires ohui >= 1.3` version constraint
- Syntax error produces clear error with file/line/column
- Unknown component types logged as warning, parse continues

**Reference:** `systems/dsl-and-skinning.md` § The DSL

---

### TASK-026 · DSL Runtime Engine

**What:** The runtime that evaluates a parsed widget AST, resolves
data bindings, runs Yoga layout, and produces draw calls.

**Start state:** TASK-025, TASK-001 (Yoga), TASK-003 (tokens),
TASK-012 (data binding engine) complete.

**End state:** Given a widget AST and a set of data binding values,
the runtime: resolves `bind()` references against the data binding
engine, resolves `token()` references against the token store,
builds a Yoga layout tree from the component structure, runs layout,
and emits a draw call list (rectangles, text, icons with computed
positions and styles). The draw calls are renderer-agnostic.

**Definition of done:**
- HealthBar widget with bound values renders correct draw calls
- Token override (skin) changes draw call colours without re-parse
- Animate block produces interpolated values over declared duration
- Widget with no bound values renders static content correctly
- Layout recalculates when widget is resized

**Reference:** `systems/dsl-and-skinning.md` § Rendering Paths

---

### TASK-027 · Renderer Bridge (Scaleform)

**What:** The default rendering backend that consumes DSL runtime
draw calls and paints them via Scaleform.

**Start state:** TASK-026 complete.

**End state:** The Scaleform renderer accepts draw call lists from
the DSL runtime and renders them into the game's UI layer.
Rectangles, text, icons, images, and 9-slice borders all render
correctly. The renderer abstraction layer interface is defined —
any alternative renderer implements this same interface.

**Definition of done:**
- A DSL-defined widget is visible on screen in-game
- Text renders with correct font, size, and colour
- Rectangles render with correct fill, border, and radius
- Icon atlas lookup and tinting functional
- Skin swap (token override) updates visuals without restart

**Reference:** `systems/dsl-and-skinning.md` § Renderer Abstraction Layer

---

### TASK-028 · Viewport Contract

**What:** The viewport interface that native C++ widgets and third-party
renderers implement to participate in the widget system.

**Start state:** TASK-010 (widget registry), TASK-027 (renderer bridge)
complete.

**End state:** A viewport contract interface: given a rectangle
(position + size), render into it. Native widgets implement this
interface and register with the widget registry. They receive edit
mode notifications, layout profile persistence, and canvas management
identically to DSL-defined widgets. Only the pixels inside the
viewport differ.

**Definition of done:**
- A test native widget renders custom content into a viewport
- The native widget participates in edit mode (drag, resize)
- Layout profile saves and restores the native widget's position
- The native widget receives show/hide notifications

**Reference:** `systems/hud-widget-runtime.md` § Non-Scaleform Overlays,
`systems/dsl-and-skinning.md` § Rendering Paths

---

## Phase 4 — MCM Compatibility Layer

Requires Phase 0 complete. Independent of Phases 1–3.

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

**Start state:** TASK-030 complete. Requires component library
primitives (Toggle, Slider, Dropdown, TextInput) — either from
TASK-050A or built inline as MCM-specific controls.

**End state:** When a player opens an MCM, OHUI renders it using
OHUI components. All 8 control types functional: Toggle,
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

## Phase 5 — MCM2 Native API

Requires Phase 4 complete.

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
- Custom state provider registered and queried correctly
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

## Phase 6 — Component Library

Requires Phase 3 (DSL runtime) and Phase 2 (input) complete.
Components are the building blocks for all screens.

---

### TASK-049 · Icon System

**What:** Icon atlas loading, keyword-to-icon resolution mapping,
and skin atlas replacement.

**Start state:** TASK-027 (renderer bridge) complete.

**End state:** OHUI loads a first-party icon atlas covering all
vanilla and DLC item types, spell schools, perk icons, shout icons,
status indicators (stolen, quest, enchanted, favourited), and UI
chrome. Keyword-to-icon resolution maps item/spell properties to
atlas entries automatically. Skin authors can ship replacement
atlases. Resolution is fast enough for list scrolling with thousands
of items.

**Definition of done:**
- Vanilla sword resolves to weapon icon via standard keywords
- DLC item resolves to correct icon
- Mod-added item using standard keywords resolves automatically
- Skin replacement atlas overrides all default icons
- Resolution of 1000 items completes within one frame

**Reference:** `systems/icon-system.md`

---

### TASK-050A · Atom Layer and Token Implementation

**What:** Implement the atom layer (Text, Rectangle, Line, Icon,
Image, Viewport primitives) and the full token set (colour, spacing,
typography, shape) as defined in `systems/component-library.md`.

**Start state:** TASK-027 (renderer bridge) and TASK-049 (icon
system) complete.

**End state:** All 6 atom types render correctly via the renderer
bridge. All token categories loaded from USS and resolvable. Skin
token overrides change atom rendering. The token set from
component-library.md is the shipped default.

**Definition of done:**
- Text atom renders with correct font stack, size, weight, colour
- Rectangle atom renders filled, outlined, and rounded variants
- Icon atom renders from atlas with tint
- Image atom renders with fit/fill/stretch modes
- Token override changes all consuming atoms on next render

**Reference:** `systems/component-library.md` § Token Layer, § Atom Layer

---

### TASK-050B · Text and Value Components

**What:** Label, Caption, RichText, LoreText, StatValue, StatDelta,
DeltaList, ValueBar, TimerBar, CountBadge.

**Start state:** TASK-050A complete.

**End state:** All 10 components render correctly, consume tokens,
and are composable within widget definitions.

**Definition of done:**
- StatDelta renders colour-coded positive/negative deltas
- ValueBar animates fill on value change
- TimerBar shifts colour at registered thresholds
- RichText renders inline bold, italic, colour spans

**Reference:** `systems/component-library.md` § Text Components,
§ Value Display Components

---

### TASK-050C · List Components

**What:** ListEntry, ListEntryCompact, ScrollList, FacetedList.

**Start state:** TASK-050B complete.

**End state:** FacetedList fully functional — SearchField, PresetBar,
FacetPanel (drawer on controller, sidebar on mouse), ActiveFilterChips,
virtualised ScrollList. External facet registration via mod API.

**Definition of done:**
- 1000-item list scrolls smoothly at 60fps
- Search field filters list in real time
- Multiple facets active simultaneously, chips show each active
  facet, tapping chip removes that facet
- External facet registered by a test mod appears in FacetPanel

**Reference:** `systems/component-library.md` § List Components

---

### TASK-050D · Navigation and Input Components

**What:** TabBar, Breadcrumb, Pagination, TextInput, SearchField,
Toggle, Slider, Dropdown, Stepper, ContextMenu.

**Start state:** TASK-050A complete.

**End state:** All navigation and input components functional with
keyboard, mouse, and controller. Full OSK support on TextInput.

**Definition of done:**
- TextInput: type text, clear, OSK triggers on controller focus
- Dropdown: open, scroll, select, keyboard search
- ContextMenu: trigger from list entry, navigate, select, dismiss
- Stepper: increment, decrement, min/max constraints honoured

**Reference:** `systems/component-library.md` § Navigation Components,
§ Input Components

---

### TASK-050E · Container and Layout Components

**What:** Panel, SplitPanel, ScrollPanel, Modal, Tooltip, Drawer.

**Start state:** TASK-050A complete.

**End state:** All layout containers render correctly. Modal traps
focus and dims backdrop. Tooltip repositions at screen edges. Drawer
slides in/out with animation.

**Definition of done:**
- Modal: opens, traps focus, dismisses on backdrop click and cancel
- Tooltip: appears on hover, repositions when near screen edge
- SplitPanel: draggable divider adjusts ratio
- Drawer: slides in from edge, slides out on dismiss

**Reference:** `systems/component-library.md` § Container Components

---

### TASK-050F · Indicator and Media Components

**What:** StatusBadge, IndicatorDot, AlertBanner, CompletionRing,
Portrait, CharacterViewport, SceneViewport, MapViewport.

**Start state:** TASK-050A complete. TASK-028 (viewport contract) for
media viewport components.

**End state:** All indicator components render with correct tokens.
Media viewport components render into viewport contract surfaces.
CharacterViewport supports rotate/zoom/pan input.

**Definition of done:**
- AlertBanner renders dismissable warning with icon
- CharacterViewport renders player character with camera control
- Portrait renders NPC bust with fixed framing

**Reference:** `systems/component-library.md` § Indicator Components,
§ Media Components

---

### TASK-050G · Edit Mode Components

**What:** WidgetBoundingBox, AlignmentGuide, EditModeToolbar,
GridOverlay.

**Start state:** TASK-013 (edit mode runtime) complete, TASK-050A
complete.

**End state:** Edit mode visuals rendered via the component library.
Replaces any placeholder visuals from TASK-013. Alignment guides
appear on widget proximity. Grid overlay toggleable.

**Definition of done:**
- Bounding box shows resize handles and widget label
- Alignment guides appear when dragging near another widget edge
- Toolbar shows coordinates and grid snap toggle
- Grid overlay matches snap increment, toggles on/off

**Reference:** `systems/component-library.md` § Edit Mode Components

---

### TASK-050H · HUD-Specific Components

**What:** ResourceBar, ShoutMeter, CompassRose, DetectionMeter,
StealthEye, NotificationToast.

**Start state:** TASK-050B (ValueBar/TimerBar), TASK-014 (data
binding schema) complete.

**End state:** All 6 specialised HUD components render correctly
with live data bindings. ResourceBar animates damage flash and
regen. ShoutMeter shows word-count segmentation. CompassRose
renders heading with marker overlay.

**Definition of done:**
- ResourceBar: damage flash on health loss, regen shimmer visible
- ShoutMeter: 3-word segmentation visible, cooldown animates
- CompassRose: rotates with player heading, shows quest markers
- DetectionMeter: transitions between Hidden/Caution/Detected
- StealthEye: fills proportionally to detection level
- NotificationToast: appears, persists, fades on timer

**Reference:** `systems/component-library.md` § Specialised HUD Components

---

## Phase 7 — Screens

Each screen is an independent task. All require Phase 6 (component
library) and Phase 2 (input) complete. Screens using FacetedList
require TASK-050C.

---

### TASK-051 · Inventory Screen

**Start state:** TASK-050C complete.
**End state:** Inventory screen fully functional with FacetedList,
item detail panel, sort controls, all item categories, and all
item action verbs (equip, drop, favourite, inspect).
**Definition of done:** Player can open inventory, filter by type and
condition, select and equip an item, drop an item, mark favourite.

**Reference:** `screens/inventory-barter-container.md`

---

### TASK-052 · Barter Screen

**Start state:** TASK-050C complete.
**End state:** Player inventory and merchant inventory side by side.
Offer panel. Gold/value display. Transaction confirm.
**Definition of done:** Complete a buy, complete a sell, confirm
transaction completes correctly with gold updated.

**Reference:** `screens/inventory-barter-container.md`

---

### TASK-053 · Container Screen

**Start state:** TASK-050C complete.
**End state:** Player inventory and container inventory side by side.
Take All. Transfer individual items. Search across both lists.
**Definition of done:** Take All works. Individual transfer works
both directions.

**Reference:** `screens/inventory-barter-container.md`

---

### TASK-054 · Magic Screen

**Start state:** TASK-050C complete.
**End state:** Spells, powers, shouts and lesser powers in FacetedList.
Equip to left/right hand. Favourite. Shout section shows word unlock
and cooldown state.
**Definition of done:** Equip spell to both hands, favourite a shout,
filter by school.

**Reference:** `screens/magic-spells-shouts.md`

---

### TASK-055 · Crafting Screens

**Start state:** TASK-050C complete.
**End state:** Smithing forge (craft and temper), alchemy lab,
enchanting table, and cooking pot. All functional. Shared QOL:
search, filter (can-craft-now default on), sort. Active effect
timer strip visible when fortify effects are active. Smithing
output/temper preview engine-sourced. Alchemy effect matching
on known effects only. Enchanting strength preview with soul gem
comparison. Enchanting item naming field. Bulk enchanting flow.
Cooking survival context when survival mode active.
**Definition of done:** Craft a smithing item with output preview.
Temper with quality tier roadmap. Brew a potion with effect matching.
Enchant with strength preview and naming field. Active effect timer
visible and draining during fortify buff.

**Reference:** `screens/crafting.md`

---

### TASK-056 · Quest Journal

**Start state:** Phase 6, Phase 2 complete.
**End state:** Active and completed quests. Quest stages and objectives.
Map marker integration. Sort by faction, type, date. Dialogue history
per quest (stage-implied mechanism for vanilla quests).
**Definition of done:** All active quests visible, objectives correct,
completed quests accessible. Dialogue history shows conversation
transcripts attributed to correct NPCs.

**Reference:** `screens/quest-journal.md`

---

### TASK-057 · Map Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** World map with markers, zoom, fast travel, free text
search, marker type filtering, quest marker colour coding. World
beacons placeable and persistent via cosave.
**Definition of done:** Open map, search for a location by name,
place a beacon, fast travel to a known location. Beacon persists
after save/reload.

**Reference:** `screens/map.md`

---

### TASK-058 · Skills Screen

**Start state:** Phase 6, Phase 2 complete. Investigation into
constellation rendering approach complete.
**End state:** All 18 skill trees functional. Perk acquisition
functional. Perk points correctly tracked. Perk description panel
with structured info (name, rank, effect per rank, requirements,
conditional effects, synergies, source). Navigation between
prerequisites and dependents. Skill XP bar with exact XP-to-next.
**Definition of done:** Spend a perk point, confirm tree updates.
Perk description shows all ranks and requirements. Navigate from
a perk to its prerequisite and back.

**Reference:** `screens/skills.md`

---

### TASK-059 · Dialogue Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** Topic list, subtitles, speaker attribution, full
sentence preview option. Speaker portrait (real-or-nothing — render
if available, omit gracefully if not).
**Definition of done:** Complete a branching dialogue tree without
missing any response options. Speaker name and portrait displayed.

**Reference:** `screens/dialogue.md`

---

### TASK-060 · Level Up Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** Attribute selection (Health/Magicka/Stamina), impact
preview showing stat consequences of each choice, skill summary for
the level, perk point award notification.
**Definition of done:** Level up, select attribute, confirm point
awarded and attribute increased. Impact preview shows before/after
values for the selected attribute.

**Reference:** `screens/level-up.md`

---

### TASK-061 · Main Menu

**Start state:** Phase 6, Phase 2 complete.
**End state:** Main menu with Continue, New Game, Load Game, Settings,
Credits, Quit to Desktop. Styled consistently with active OHUI skin.
Settings entry point functional (OHUI config, display, audio,
controls, gameplay).
**Definition of done:** All menu options functional. New Game starts
a new game. Continue loads most recent save. Settings opens. Quit
exits directly without confirmation.

**Reference:** `screens/main-menu.md`

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

**Reference:** `screens/load-game.md`

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

**Reference:** `screens/load-game.md`

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

**Reference:** `screens/load-game.md`

---

### TASK-065 · Wait and Sleep Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** Distinct Wait and Sleep screens. Extended duration
slider (72hr default, configurable). Time picker with direct input.
Quick increment buttons. Danger indicator (Safe through Do Not Sleep
Here). Rest bonus information with live feedback as duration changes.
**Definition of done:** Wait 48 hours using extended slider. Sleep
and see rest bonus update as duration changes. Danger indicator
displays appropriate level for current location.

**Reference:** `screens/wait-sleep.md`

---

### TASK-066 · Loading Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** Vanilla loading screen running faithfully — rotating
model, loading bar, tip text. Bar and tip styled consistently with
active OHUI skin. Custom loading screen mods (NIF/texture/tip
replacements) remain compatible.
**Definition of done:** Loading screen appears during cell transition
with skinned bar and tip text. A custom loading screen mod's assets
display correctly.

**Reference:** `screens/loading-screen.md`

---

### TASK-067 · Stats Screen

**Start state:** Phase 6, Phase 2 complete.
**End state:** Vanilla stats presented cleanly in organised tabs.
**Definition of done:** All vanilla stat categories visible and
correctly populated.

**Reference:** `screens/stats.md`

---

### TASK-068 · Tween Menu

**Start state:** Phase 6, Phase 2 complete.
**End state:** Configurable radial quick-nav widget. Default 5 slots
matching vanilla (Inventory, Magic, Map, Skills, Wait). Inline
configuration via edit mode — reassign destinations from live
registry, add/remove slots. Slot configuration persisted in cosave.
**Definition of done:** Open tween menu, navigate to each default
destination. Enter edit mode, add MCM as a new slot, reassign a
slot to a different destination, confirm it persists after reload.

**Reference:** `screens/tween-menu.md`

---

### TASK-069 · Favourites Radial

**Start state:** Phase 6, Phase 2 complete.
**End state:** Favourites radial with slot types: weapon, spell,
shout, potion (smart selection), item. Equip from radial. Cycle
groups. Outfit slot type deferred to TASK-115 (outfit integration).
**Definition of done:** Assign a weapon, spell, and potion to radial
slots. Equip each from the radial. Potion smart selection picks
optimal potion from available stock.

**Reference:** `screens/favourites-radial.md`

---

### TASK-069B · Gift Menu

**Start state:** Phase 6, Phase 2 complete.
**End state:** Gift menu with disposition indicator, item staging
panel, NPC acceptance/rejection feedback.
**Definition of done:** Open gift menu with a follower, stage items,
confirm gift. Disposition indicator visible.

**Reference:** `screens/gift-menu.md`

---

## Phase 8 — Message Log

Requires Phase 3 (DSL runtime) and Phase 2 (input) complete.

---

### TASK-075 · Message Stream Infrastructure

**What:** The core append-only message stream with typed messages,
publisher registration, and subscriber notification.

**Start state:** Phase 0 complete.

**End state:** A message stream that accepts typed messages with
content, type, timestamp, source, priority, and lifetime hint.
Publishers fire-and-forget into the stream. Subscribers receive
filtered notifications. Built-in publishers: notification intercept,
subtitle intercept, quest update, skill increase, OHUI system.

**Definition of done:**
- Native notification call publishes to stream as `notification` type
- Subtitle event publishes as `subtitle_interactive` or `subtitle_ambient`
- Subscriber with type filter receives only matching messages
- Stream retains all messages for session duration

**Reference:** `systems/message-log.md` § Architecture

---

### TASK-076 · HUD Message Widget

**What:** The on-screen message display — a configurable viewport
into the message stream.

**Start state:** TASK-075, TASK-050H (NotificationToast component)
complete.

**End state:** A HUD widget that displays recent messages from the
stream, filtered by type, with configurable fade timing, maximum
visible count, and anchor position. Replaces vanilla notification
and subtitle display.

**Definition of done:**
- Messages appear on screen when published to stream
- Messages fade after lifetime expires
- Scrolling back reveals older faded messages
- Type filter hides/shows message categories
- Widget participates in edit mode (position, size, visibility)

**Reference:** `systems/message-log.md` § HUD Message Widget

---

### TASK-077 · Message Log Panel

**What:** The full persistent log panel — tabbed, searchable,
scrollable, with clipboard copy and share-to-web.

**Start state:** TASK-075, Phase 6 (component library) complete.

**End state:** A modal panel accessible from any context. Tabbed by
message type. Chronological. Searchable. Per-entry copy to clipboard.
Panel-level copy. Quest-attributed entries link to journal.

Share-to-web: a share button adjacent to the copy icon publishes
the current tab's content to a web service (Pastebin or GitHub
Gist, selectable via popover) and copies the returned URL to
clipboard. HTTP call via SKSE networking. No browser required.
Nothing transmitted without explicit player action.

**Definition of done:**
- All message types appear under correct tabs
- Search filters across all types
- Copy single entry to clipboard
- Copy entire tab to clipboard
- Custom mod-registered type appears as its own tab
- Share to Pastebin: URL returned and copied to clipboard
- Share to GitHub Gist: URL returned and copied to clipboard
- Share with no network: falls back to raw text clipboard copy

**Reference:** `systems/message-log.md` § Message Log Panel,
§ Share to web

---

## Phase 9 — Mod Integration

Requires Phase 1 (TASK-010 widget registry, TASK-012 data binding
engine) complete. Independent of screen tasks.

---

### TASK-085 · Custom Widget Registration

**What:** The widget registration API — the only integration point
where mods must actively communicate with OHUI.

**Start state:** TASK-010, TASK-012 complete.

**End state:** A mod can register a custom HUD widget by providing
a manifest (ID, display name, default position/size, min/max
constraints, default visibility) and a rendering implementation.
Registered widgets participate fully in the canvas — edit mode,
layout profiles, skin support. Manifest validation (mod ID required).
Registration window enforcement (post-load). Duplicate ID supersedes.
Graceful degradation when no widgets registered.

**Definition of done:**
- A test mod registers a widget — appears on canvas
- Widget participates in edit mode (drag, resize)
- Layout profile saves and restores widget position
- Missing mod ID rejected with warning log
- Registration outside valid window rejected with warning log

**Reference:** `systems/mod-registration-api.md` § Custom Widgets

---

### TASK-086 · Custom Data Binding Registration

**What:** The custom binding registration API — mods expose game
state values OHUI doesn't know about.

**Start state:** TASK-012 (data binding engine) complete.

**End state:** A mod can register a custom data binding with a
key (e.g. `mymod.player.hunger`), a type, and a poll source.
Any widget can then bind to it. Frozen giant integrations use
this mechanism — OHUI registers bindings on the mod's behalf.
Graceful empty state when no custom bindings registered.

**Definition of done:**
- A test mod registers a Float binding — a test widget reads it
- Binding updates propagate to subscribed widgets
- Duplicate key supersedes with warning log
- No custom bindings registered: all built-in bindings unaffected

**Reference:** `systems/mod-registration-api.md` § Custom Data Bindings

---

## Phase 10 — Localisation

Can begin as soon as Phase 0 is complete. Independent of all other
phases but blocks release of any user-visible text.

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

**Reference:** `systems/localisation.md`

---

### TASK-081 · RTL Layout

**Start state:** TASK-001 (Yoga) complete.
**End state:** Yoga `direction: RTL` applied when active language
is RTL. Semantic directional properties (`inline-start/end`)
resolve correctly for both LTR and RTL. Visual test: a simple
two-column layout mirrors correctly in RTL.
**Definition of done:** LTR layout correct. RTL layout mirrors
correctly. No hardcoded `left`/`right` in any first-party component.

**Reference:** `systems/localisation.md`

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

**Reference:** `systems/localisation.md`

---

## Phase 11 — Compatibility Layers

Requires Phase 6 (component library) for UI rendering.

---

### TASK-090 · UIExtensions Compatibility Shim

**What:** Intercept UIExtensions Papyrus calls and render via OHUI
components.

**Start state:** Phase 6 complete.

**End state:** UIExtensions Papyrus API surface (OpenMenu, OpenInputMenu,
GetMenuResultString, GetMenuResultInt, AddEntryItem, AddEntryText,
SetMenuPropertyString, SetMenuPropertyInt) intercepted and routed to
OHUI components. Functions as standalone implementation — mods do not
require UIExtensions installed when OHUI is present.

**Definition of done:**
- A mod calling OpenMenu(ItemMenu) gets an OHUI selection list
- A mod calling OpenInputMenu gets an OHUI input dialog with OSK
- Return values match UIExtensions format exactly
- Works without UIExtensions installed
- Works alongside UIExtensions (shim takes priority for covered calls)

**Reference:** `compatibility.md` § UIExtensions

---

### TASK-091 · Native Notification Intercept

**What:** Intercept Skyrim's native notification function and publish
into the message stream.

**Start state:** TASK-075 (message stream) complete.

**End state:** Native notification calls from any mod publish as
`notification` typed messages in the OHUI message stream. Attribution
to calling mod where determinable. Always on — not disableable.

**Definition of done:**
- Native notification call from a mod appears in message log
- Message typed correctly as `notification`
- Message persists in log after on-screen display fades

**Reference:** `compatibility.md` § Native Notification System

---

## Phase 12 — Frozen Giant Integrations

Requires relevant screen tasks complete. Each integration is
independent and can be built in parallel.

---

### TASK-095 · Survival Mod Integrations (Sunhelm, Frostfall, iNeed, Campfire)

**What:** Native support for the four survival/camping frozen giants.

**Start state:** TASK-065 (wait/sleep screen) complete.

**End state:** OHUI detects each mod at startup, reads their exposed
data, and surfaces it in the sleep screen survival context panel and
HUD status indicators. Graceful fallback when absent.

**Definition of done:**
- Sunhelm installed: hunger, thirst, fatigue, cold visible in sleep
  screen. HUD status indicators active.
- Frostfall installed: temperature and warmth data visible in sleep screen.
- iNeed installed: needs visible in sleep screen (distinct from Sunhelm).
- Campfire installed: warmth source detection feeds sleep screen.
- Any absent: corresponding panel/indicator absent, no error.
- Log shows detection result for each on startup.

**Reference:** `compatibility.md` § Frozen Giants

---

### TASK-096 · Follower Framework Integrations (NFF, AFT)

**What:** Native support for follower tracking data from NFF and AFT.

**Start state:** TASK-057 (map screen) complete.

**End state:** Follower location, state, and assignment data feeds
the map screen NPC tracking feature. NFF and AFT detected and used
as data sources. Graceful fallback when absent.

**Definition of done:**
- NFF installed: followers appear as trackable NPCs on map with
  current location
- AFT installed: same feature, AFT data model
- Neither installed: NPC tracking shows only vanilla followers
- Log shows detection result on startup

**Reference:** `compatibility.md` § Frozen Giants

---

### TASK-097 · LOTD Integration

**What:** Native support for Legacy of the Dragonborn across
map markers, custom skill trees, quest journal, and tween menu.

**Start state:** TASK-057 (map), TASK-058 (skills), TASK-056
(quest journal), TASK-068 (tween menu) complete.

**Definition of done:**
- LOTD map markers appear on map screen
- LOTD skill trees render in skills screen
- LOTD quests appear in quest journal with full feature set
- LOTD screens available as tween menu destinations
- LOTD absent: all above absent, no error

**Reference:** `compatibility.md` § Frozen Giants

---

### TASK-098 · Dirt and Blood / General Stores Integration

**What:** Native support for Dirt and Blood (HUD status indicators)
and General Stores (barter/economy data).

**Start state:** Relevant screen tasks complete.

**Definition of done:**
- Dirt and Blood installed: dirt/blood status visible in HUD
- General Stores installed: merchant data available in barter screen
- Either absent: corresponding feature absent, no error

**Reference:** `compatibility.md` § Frozen Giants

---

## Phase 13 — Outfit System

Requires TASK-051 (inventory screen) and TASK-053 (container screen)
complete. TASK-004 (cosave) complete.

---

### TASK-115 · Player Outfit System

**What:** Named equipment sets — save, browse, preview, equip from
the inventory screen.

**Start state:** TASK-051 (inventory screen), TASK-004 (cosave)
complete.

**End state:** Player can save the current equipment state as a
named outfit. Outfit panel in inventory screen lists all saved
outfits. Hovering an outfit updates character preview in real time
and shows aggregate character impact delta. Equipping swaps all
slots, flags missing items. Outfit management: rename, overwrite,
duplicate, delete, reorder. Semantic label field for trigger mod
integration. Outfits persisted in cosave Outfit Definitions block.

**Definition of done:**
- Save current equipment as named outfit
- Browse outfit panel, hover shows preview and delta
- Equip outfit — all slots swapped, missing items flagged
- Rename, overwrite, duplicate, delete all functional
- Save game, reload — outfits restored from cosave
- Label field accepts freeform text, retrievable via API

**Reference:** `systems/outfit-system.md`

---

### TASK-116 · Follower Outfit System

**What:** Extend outfit system to followers via the container screen.

**Start state:** TASK-115 (player outfits), TASK-053 (container
screen) complete.

**End state:** Follower container screen has an outfit panel in the
header. Same behaviour as player outfit panel — browse, preview on
follower in character viewport, character impact delta, equip in one
action. Follower outfits managed per-follower. Outfit swap is
immediate (no script queue). Label field for trigger mod integration.
Per-follower outfit data persisted in cosave.

**Definition of done:**
- Save follower's current equipment as named outfit
- Browse follower outfit panel, preview on follower viewport
- Equip follower outfit — immediate swap, no script delay
- Multiple followers maintain independent outfit lists
- Save game, reload — follower outfits restored from cosave

**Reference:** `systems/outfit-system.md` § Follower Outfits

---

### TASK-117 · Outfit Radial Integration

**What:** Add outfit slot type to the favourites radial.

**Start state:** TASK-115 (player outfits), TASK-069 (favourites
radial) complete.

**End state:** Outfit is available as a slot type in the favourites
radial. Player can assign a saved outfit to a radial slot and equip
it directly from the radial without opening inventory.

**Definition of done:**
- Assign an outfit to a radial slot
- Activate slot — outfit equipped
- Outfit deleted — radial slot shows empty/missing state

**Reference:** `systems/outfit-system.md` § HUD Widget Integration,
`screens/favourites-radial.md`

---

## Phase 14 — First-Party HUD Widgets

Requires Phase 3 (DSL runtime) and TASK-014 (data binding schema)
complete. Each widget is independent.

---

### TASK-100 · Health / Stamina / Magicka Bars

**What:** The three primary resource bars as DSL-defined widgets
using the ResourceBar component.

**Start state:** TASK-050H, TASK-026, TASK-014 complete.

**End state:** Three widgets on the HUD canvas. Each bound to its
respective vital binding. Damage flash, regen animation, threshold
colour shifts. Frame-driven update mode. Default positions per
resolution preset. Fully skinnable.

**Definition of done:**
- Health bar drains smoothly during combat damage
- Stamina bar depletes during sprint
- Magicka bar depletes on spell cast and regens visibly
- All three participate in edit mode
- Skin token override changes bar colours

---

### TASK-101 · Compass Widget

**What:** The compass rose as a first-party HUD widget.

**Start state:** TASK-050H (CompassRose component), TASK-014 complete.

**End state:** Compass widget on HUD canvas. Shows cardinal directions,
rotates with player heading. Quest markers and location markers
overlaid. Throttled reactive update mode (~15fps).

**Definition of done:**
- Compass rotates with heading
- Active quest marker visible on compass
- Discovered location markers visible at appropriate range
- Participates in edit mode

---

### TASK-102 · Shout Cooldown Meter

**What:** Shout cooldown as a first-party HUD widget.

**Start state:** TASK-050H (ShoutMeter component), TASK-014 complete.

**End state:** Widget shows shout cooldown progress. Word-count
segmentation markers. Hidden when no shout equipped or cooldown
complete.

**Definition of done:**
- Shows cooldown draining after shout use
- Segmentation markers match equipped shout word count
- Hidden when no shout equipped

---

### TASK-103 · Stealth / Detection Widgets

**What:** Detection meter and stealth eye as first-party HUD widgets.

**Start state:** TASK-050H, TASK-014 complete.

**End state:** Two widgets: DetectionMeter (graduated indicator) and
StealthEye (classic eye icon). Both bound to sneak detection bindings.
Both reactive — only render when sneaking.

**Definition of done:**
- Both appear when player enters sneak
- Both hide when player exits sneak
- Detection state transitions animate smoothly
- Both participate in edit mode independently

---

### TASK-104 · Enemy Health Bar

**What:** Enemy/target health bar as a first-party HUD widget.

**Start state:** TASK-050B (ValueBar), TASK-014 complete.

**End state:** Widget appears when player has a combat target.
Shows enemy name, level, health bar. Boss variant with distinct
presentation. Hidden when no target.

**Definition of done:**
- Appears on crosshair target in combat
- Shows name, level, health
- Boss enemy shows boss bar variant
- Hidden when no target

---

### TASK-105 · Active Effects Widget

**What:** Display of active magic effects on the HUD.

**Start state:** TASK-050B, TASK-014 complete.

**Note:** Blocked on list-binding design (data-binding-schema.md
open question 2). Requires a list-typed binding for active effects.
Design the list binding mechanism as part of this task.

**Definition of done:**
- Active effects display with icon and remaining duration
- Effects appear/disappear as they are applied/expire
- Participates in edit mode

---

### TASK-106 · Notification / Quest Update Toast

**What:** On-screen transient message display as a first-party widget.

**Start state:** TASK-075 (message stream), TASK-050H (NotificationToast)
complete.

**End state:** Widget subscribes to the message stream. Displays
recent notifications, quest updates, and skill increases with
configured fade timing. Replaces vanilla notification positioning.

**Definition of done:**
- Notification from native call appears on screen
- Quest update banner appears with quest name
- Messages fade after configured duration
- Participates in edit mode

---

### TASK-107 · Breath / Oxygen Meter

**What:** Underwater breath meter as a first-party HUD widget.

**Start state:** TASK-050B (ValueBar), TASK-014 complete.

**End state:** Widget appears when player is underwater. Shows
breath depletion. Hidden when breath is full or player is not
submerged.

**Definition of done:**
- Appears when submerged
- Drains as breath depletes
- Hidden when surfaced and breath full

---

### TASK-108 · Quick Loot Widget

**What:** Quick loot panel that appears on container interaction
without opening the full container screen.

**Start state:** Phase 6, Phase 2, TASK-014 complete.

**End state:** A HUD widget that appears when the player's crosshair
is on a lootable container. Shows container contents in a compact
list. Player can take items directly. Dismisses on look-away.

**Definition of done:**
- Panel appears on container crosshair
- Items listed with name and value
- Take single item via keypress
- Take all via keypress
- Panel dismisses on crosshair away

---

### TASK-109 · Quest Tracker Widget

**What:** Active quest objective display as a first-party HUD widget.

**Start state:** TASK-050B, TASK-014 complete.

**End state:** Widget shows active quest objectives on the HUD.
Multiple active quests supported. Updates when objectives change.
Participates in edit mode.

**Definition of done:**
- Active quest objectives visible on HUD
- Objective completes — updates immediately
- Multiple tracked quests shown simultaneously
- Participates in edit mode

---

### TASK-109B · Recent Loot Widget

**What:** Compact visual feed of items just picked up.

**Start state:** TASK-050B, TASK-049 (icon system), TASK-014 complete.

**End state:** Widget shows a feed of recently acquired items with
icon, name, and quantity. Entries appear on pickup and auto-fade
after a configurable duration. Stacks identical items.

**Definition of done:**
- Pick up item — entry appears with icon and name
- Pick up 5 of same item — single entry with ×5
- Entries fade after configured duration
- Participates in edit mode

---

## Dependency Graph (summary)

```
Phase 0 (TASK-001–005)
  ├── Phase 1 (TASK-010–014)  ← widget runtime + data binding schema
  │     ├── Phase 2 (TASK-020–022)  ← input
  │     │     └── Phase 6 (TASK-049–050H)  ← icon system + component library
  │     │           └── Phase 7 (TASK-051–069B)  ← screens
  │     │                 ├── Phase 12 (TASK-095–098)  ← frozen giants
  │     │                 └── Phase 13 (TASK-115–117)  ← outfit system
  │     │
  │     └── Phase 9 (TASK-085–086)  ← mod integration (widget + binding registration)
  │
  ├── Phase 3 (TASK-025–028)  ← DSL runtime
  │     ├── Phase 6 (merges here — components need DSL)
  │     ├── Phase 8 (TASK-075–077)  ← message log
  │     └── Phase 14 (TASK-100–109B)  ← first-party HUD widgets
  │
  ├── Phase 4 (TASK-030–033)  ← MCM compat
  │     └── Phase 5 (TASK-040–043)  ← MCM2 native
  │
  ├── Phase 10 (TASK-080–082)  ← localisation (parallel to all)
  │
  └── Phase 11 (TASK-090–091)  ← compatibility layers
```

**Key dependency notes:**
- Phase 3 (DSL runtime) is the architectural spine. Phase 6
  (components) requires it. Phase 14 (HUD widgets) requires it.
  This is the critical path.
- Phase 4 (MCM compat) is fully independent of Phases 1–3.
  Can be built in parallel by separate sessions.
- Phase 9 (mod integration) depends only on Phase 1 (widget
  registry + data binding engine). Independent of screens.
- Phase 10 (localisation) is independent of everything and can
  run alongside any phase.
- Phase 12 (frozen giants) is late-stage and depends on multiple
  screens being complete.
- Phase 13 (outfit system) depends on inventory and container
  screens. TASK-117 (radial integration) depends on both TASK-115
  and TASK-069.

---

## Open Design Questions (must resolve before dependent tasks start)

These are unresolved design decisions identified across the documentation.
Each blocks or affects specific tasks.

1. **Compositor ownership** — Scaleform layer or native DirectX layer?
   Affects TASK-027, TASK-028. Must resolve before Phase 3 is locked.
   *Source: dsl-and-skinning.md, performance-model.md*

2. **VR rendering architecture** — Scaleform and input differences.
   VR is a day-one target. Affects Phase 3 (DSL runtime), Phase 2
   (input). Must be investigated early.
   *Source: dsl-and-skinning.md*

3. **Skills constellation rendering** — 3D scene vs native C++ widget
   vs dedicated approach. Affects TASK-058 scope and timeline.
   *Source: dsl-and-skinning.md, skills.md*

4. **List-typed bindings** — enemy multi-target, active effects,
   compass markers all need list bindings not yet designed. Blocks
   TASK-105 and full TASK-101, TASK-104 scope.
   *Source: data-binding-schema.md*

5. **Danger indicator data reliability** — can OHUI reliably
   approximate threat level from available SKSE data? If unreliable,
   danger indicator does not ship. Affects TASK-065 scope.
   *Source: wait-sleep.md*

6. **Dialogue tree extraction mechanism** — stage-implied dialogue
   and background compilation are novel. Prototype needed before
   TASK-056 dialogue history scope is confirmed.
   *Source: quest-journal.md*

7. **Tween menu secondary slot access** — hold-vs-tap or sub-menu
   for secondary directions. Affects TASK-068 interaction design.
   *Source: tween-menu.md*

8. **Icon atlas format — SkyUI compatibility vs clean break** —
   SkyUI's icon system uses a specific atlas format and keyword
   mapping. Adopting the same format enables compatibility with
   mods that ship SkyUI icon patches. Defining a new format is
   cleaner but breaks existing icon mods. Affects TASK-049.
   *Source: icon-system.md*

---

## Not Yet Tasked

The following items are identified in the documentation but not
broken into tasks. They are post-1.0 features, require further
design, or depend on prior phases being proven.

- **DSL tooling** — syntax highlighting, error reporting, validation
  toolchain for mod authors. Important for adoption, not a launch
  blocker.
- **Stats screen extensibility** — mod-contributed tracked values
  via registration API. Post-1.0, depends on mod integration model
  maturity.
- **Live character render on load game** — post-1.0 exploratory.
- **Map live location preview** — post-1.0 exploratory.
- **Outfit-to-UI theme pairing** — manual colour theme selection
  per outfit. Post-1.0, data model slot included from day one.
- **New Game+** — parked in `systems/new-game-plus.md`. Not in 1.0
  scope. Depends on further design discussion (heirloom items,
  faction standing feasibility, companion plugin scope).

## Cut Features

The following were previously in scope and have been removed from
the design entirely:

- **Provenance stamps** — item crafting history, dedications,
  sentimental protection. Cut. See `systems/provenance-stamps.md`
  for status note.
