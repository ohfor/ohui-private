# MCM2 — Mod Configuration Menu

## Two Distinct Concerns

MCM2 is two things that must be kept strictly separate in the design:

**The compatibility layer** — a complete implementation of SkyUI's MCM
Papyrus API. Every function SkyUI's MCM exposes is implemented here.
Every callback fires at the correct moment. Papyrus scripts that register
with SkyUI's MCM work against this layer without modification. They never
know OHUI is underneath.

**The presentation layer** — how OHUI renders registered MCM controls
in its own settings UI. This is a standard OHUI screen built from the
component library. It has nothing to do with SkyUI's presentation. It
looks like OHUI. It feels like OHUI. The mod author's registered controls
appear in it correctly regardless of what the author intended visually.

The compatibility layer serves existing mods. The presentation layer
serves players. They are separate concerns and are designed separately.

---

## The Compatibility Contract

SkyUI's MCM API surface is finite and fully knowable from the source.
OHUI implements every function in this surface exhaustively. Nothing
is approximated. Nothing is stubbed. Every function either does what
SkyUI's version does or delegates to OHUI's equivalent system.

### Registration

OHUI implements SkyUI's full registration surface — `RegisterMCM`,
`AddPage`, `OnConfigInit`, `OnConfigOpen`, `OnConfigClose`, and
`OnPageReset`. The mod's pages appear as sections in OHUI's settings
screen under the mod's name. All events fire at the exact moments
SkyUI fires them — on first load, on settings screen open, on settings
screen close, and on page navigation respectively.

### Control Registration Functions

Every control registration function is implemented: `AddToggleOption`,
`AddSliderOption`, `AddMenuOption`, `AddTextOption`, `AddColorOption`,
`AddKeyMapOption`, `AddHeaderOption`, and `AddEmptyOption`.

Each function returns an option ID that the mod stores and uses in
callbacks to identify which control was interacted with. OHUI returns
stable IDs using the same generation logic as SkyUI — mods that store
and compare option IDs across sessions get consistent results.

### State Manipulation Functions

All state manipulation functions are implemented —
`SetToggleOptionValue`, `SetSliderOptionValue`, `SetMenuOptionValue`,
`SetTextOptionValue`, `SetColorOptionValue`, `SetKeyMapOptionValue`,
`SetOptionFlags`, and `ForcePageReset`.

These update the rendered control in OHUI's settings screen in real
time without requiring a page reload. OHUI's component layer handles
the live update — the Toggle, Slider, Dropdown, and other components
accept value updates at any time.

### Callbacks

Every callback is fired: `OnOptionSelect`, `OnOptionDefault`,
`OnOptionHighlight`, `OnOptionSliderOpen`, `OnOptionSliderAccept`,
`OnOptionMenuOpen`, `OnOptionMenuAccept`, `OnOptionColorOpen`,
`OnOptionColorAccept`, and `OnOptionKeyMapChange`.

**Timing guarantee:** Every callback fires within the same frame as
the player action that triggered it. No deferred firing. No queued
execution. The mod's handler runs before OHUI returns control to the
player. This matches SkyUI's behaviour exactly — mods that depend on
synchronous callback execution work correctly.

### Slider and Menu Data Callbacks

SkyUI uses a two-step pattern for sliders and menus — the mod registers
the control with a current value, then provides the range/options data
in a callback when the control is opened. OHUI implements the same
pattern. All slider dialog and menu dialog data functions are
implemented.

OHUI calls the appropriate open callback when the player activates a
slider or menu. The mod provides range/options data in its handler.
OHUI reads those values and opens the correct component. The mod never
knows OHUI is underneath — it is talking to the same API it always has.

---

## The Presentation Layer

What OHUI renders from the registered MCM data. This is a standard
OHUI screen built from the component library. It has no relationship
to SkyUI's Flash-based MCM presentation.

### Screen Structure

MCM2 is accessible from the tween menu under Settings → Mod Settings.
Each registered mod appears as a named section. Mods are listed
alphabetically with a search field at the top for large mod lists.

Selecting a mod opens that mod's control pages. Pages registered via
`AddPage()` appear as TabBar tabs at the top of the mod's settings
panel — this is one of the legitimate TabBar use cases: a small fixed
set of mutually exclusive mode pages, not content browsing.

### Control Rendering

Each MCM control type maps to an OHUI component:

```
AddToggleOption    →  Toggle component
AddSliderOption    →  Slider component  
AddMenuOption      →  Dropdown component
AddTextOption      →  StatValue component (read-only label/value pair)
AddColorOption     →  ColourPicker component (new — see below)
AddKeyMapOption    →  KeyBind component (new — see below)
AddHeaderOption    →  section divider with Label
AddEmptyOption     →  spacing element
```

Controls are laid out in a single-column list matching the registration
order, same as SkyUI. Description text registered via `asDescription`
appears as a Tooltip on hover/focus — the player sees the mod author's
description when they navigate to a control.

**ColourPicker** — a new component not previously in the library. A
colour swatch button that opens a Modal with a full HSL colour picker,
hex input field, and preview. The component OHUI implements natively.
Mod authors who registered `AddColorOption` in SkyUI get a better
colour picker than SkyUI provided for free.

**KeyBind** — a new component. A read-only label showing the current
key assignment, activated to enter capture mode. In capture mode the
next keypress (keyboard or controller button) is captured as the new
binding. Escape cancels. This is the standard keybind capture pattern
implemented as a reusable component.

### Live Updates

When a mod calls `SetToggleOptionValue` or any other state setter, the
rendered control updates immediately without a page reload. The player
sees the change in real time. This is a significant improvement over
SkyUI's MCM which requires `ForcePageReset()` for most live updates —
OHUI's component layer handles value updates natively.

Mods that do call `ForcePageReset()` still work correctly — OHUI
re-renders the page. But mods that relied on `SetOptionValue` functions
without `ForcePageReset()` and were broken in SkyUI work correctly in
OHUI because the components respond to value updates directly.

---

## Two New Components for the Library

ColourPicker and KeyBind are added to the component library as a result
of MCM2's requirements. Both are independently reusable — ColourPicker
for any colour selection surface, KeyBind for OHUI's own settings
controls as well as mod-registered controls.

Component count increases from 59 to 61.

---

## What Mod Authors Get for Free

Mod authors who have existing MCM registrations get OHUI's presentation
for free with no code changes:

- Clean OHUI visual language instead of SkyUI's Flash aesthetic
- Tooltips for description text that SkyUI buried in a separate panel
- Live control updates without ForcePageReset
- A better ColourPicker than SkyUI's
- Controller navigation that matches the rest of OHUI's controller UX
- Search across all mod settings from the mod list

Nothing breaks. Everything improves. The contract is honoured completely
and the presentation is better on every dimension.

---

## MCM List Management

MenuMaid2 (mod 67556) exists to solve the MCM list management problem
that SkyUI never addressed — hiding, renaming, and reordering MCM
entries, and paginating a list that SkyUI handles poorly beyond a
certain count.

OHUI's MCM2 absorbs all of this as native first-party behaviour.
MenuMaid2's entire reason for existing evaporates.

**Hide** — every mod entry in the MCM2 list has a visibility toggle
accessible from its context menu. Hidden mods are absent from the list
in normal use. They remain accessible via a "Show hidden" filter toggle
for players who want to unhide them later. A heavily configured mod list
where half the MCMs are set-and-forget can be reduced to only the mods
the player actively uses.

**Rename** — every mod entry accepts a player-defined display name
override. Set from the context menu. The override is stored in OHUI's
settings. The original mod name is preserved internally and shown on
hover in the tooltip. A mod with a cryptic or unhelpful name gets
whatever name the player gives it.

**Reorder** — the MCM2 mod list is drag-and-drop reorderable. The
player arranges their MCM list in whatever order makes sense to them.
The order persists across sessions. Alphabetical and most-recently-used
sort options available as alternatives to manual ordering.

**Pagination** — absent entirely. FacetedList virtualises its scroll.
A thousand MCM entries scroll as smoothly as ten. SkyUI's pagination
was a workaround for Flash's rendering constraints. Those constraints
do not exist in OHUI's native renderer.

**Unlimited MCMs** — SkyUI's MCM count ceiling was a Flash memory
constraint. OHUI has no equivalent constraint. The ceiling does not
exist.

Hide, rename, and reorder state is stored per character in the cosave
alongside layout profiles and outfit definitions. A player who switches
characters gets that character's MCM list configuration, not a shared
global one.

---

## Developer and Maintenance Tools

MenuMaid2 provides an MCM registry refresh for developers — forcing a
re-registration pass without a full save/load cycle. This is a developer
workflow tool that saves significant time during mod development. Iterate
on an MCM layout, hit refresh, see the result immediately.

OHUI's MCM2 includes a Maintenance section with this and extended
developer tooling. Not buried in a separate mod. Not an afterthought.
Present and accessible to anyone developing against the MCM2 API.

**Refresh MCM Registry** — re-runs the full registration pass. Picks
up new registrations from mods loaded since last pass. Drops
registrations from mods no longer present. The primary tool for
developers iterating on MCM layouts mid-session.

**Reload Single Mod** — refreshes one mod's MCM registration without
touching the rest of the registry. Faster when iterating on a specific
mod. Triggers `OnConfigInit` and `OnPageReset` for that mod only.

**Registration State Inspector** — a read-only diagnostic view showing
every currently registered mod, its registration timestamp, its page
and control structure, and whether each callback has fired this session.
Answers "did my mod register correctly" without needing log diving.

**Force Callback** — triggers `OnConfigOpen`, `OnConfigClose`, or
`OnPageReset` for a specific mod on demand. Useful when testing
callback behaviour without navigating through the full MCM open/close
flow. Developer tool — not exposed in normal MCM2 navigation.

**Clear Registration** — removes a specific mod's registration
entirely, as if it had never registered. Forces a clean re-registration
on next refresh. Useful when testing first-run registration behaviour.

The Maintenance section is visually separated from the mod list and
requires a deliberate navigation action to reach. Players who are not
developing mods never encounter it. Developers who need it find it
exactly where they expect it.

All maintenance actions are logged at debug level with before/after
state. The log is the paper trail for diagnosing registration issues
in complex mod lists.

---

## MCM2 Native API — Declarative MCM

The compatibility layer honours the SkyUI contract. The native MCM2 API
is what MCM should have been from the start.

The fundamental difference is architectural. SkyUI's MCM is imperative —
Papyrus scripts call registration functions, implement callback handlers,
manage control state procedurally. The MCM is a running script with live
state. This makes it fragile, hard to iterate on, and resistant to
tooling.

The native MCM2 API is declarative. An MCM is a data file that describes
what the MCM contains. It has no runtime state of its own. The behaviour
— what happens when a control is interacted with — is registered
separately as named callbacks. The definition and the behaviour are
decoupled. This separation is what makes everything else possible.

### Definition Format

An MCM definition is a structured data file in OHUI's DSL format.
It describes the MCM completely — pages, sections, controls, default
values, descriptions, conditions. Nothing imperative. No function calls.
Pure data.

```yaml
mcm:
  id: com.mymod.settings
  displayName: My Mod
  version: 1.0.0

  pages:
    - id: general
      displayName: General
      sections:
        - id: gameplay
          displayName: Gameplay
          controls:
            - type: toggle
              id: enableFeature
              label: Enable Feature
              description: Enables the main feature of this mod.
              default: true
              onChange: OnFeatureToggled

            - type: slider
              id: featureStrength
              label: Feature Strength
              description: How strong the feature is.
              min: 0.0
              max: 100.0
              step: 1.0
              default: 50.0
              format: "{0}%"
              onChange: OnStrengthChanged
              condition: enableFeature == true

            - type: dropdown
              id: featureMode
              label: Mode
              options: [Subtle, Normal, Aggressive]
              default: Normal
              onChange: OnModeChanged

        - id: advanced
          displayName: Advanced
          collapsed: true
          controls:
            - type: keybind
              id: activationKey
              label: Activation Key
              default: 0
              onChange: OnKeyChanged
```

The definition is complete. It is also entirely static. It contains no
logic. It describes structure, defaults, and named callback references.
OHUI reads this file and renders the MCM. No script execution required
to display the MCM.

### Callback Registration

Callbacks are registered separately — in a C++ plugin or a Papyrus
script — by the names referenced in the definition. The definition
references callbacks by name. The callbacks are implemented wherever
makes sense for the mod — C++ for native plugins, Papyrus for script-
based mods. OHUI resolves the reference at interaction time. The
definition does not need to know where the callback lives.

### Conditions

Controls can declare conditions for visibility and availability
directly in the definition. Conditions reference other control values
by ID. OHUI evaluates them reactively — when `enableFeature` changes,
OHUI re-evaluates all conditions that reference it and updates the
rendered controls immediately.

```yaml
condition: enableFeature == true
```

No callback required. No `ForcePageReset`. The UI is reactive to its
own state by design. The mod author declares the relationship. OHUI
maintains it.

Conditions can also reference game state via registered state providers:

```yaml
condition: player.skill.smithing >= 50
condition: mod.isLoaded("Ordinator")
condition: ohui.version >= "1.2.0"
```

State providers are registered via the mod registration API. Built-in
providers cover common cases — player skills, active mods, OHUI version.
Mods can register their own.

### Persistence

Control values in a declarative MCM are persisted automatically by
OHUI. The mod author does not write persistence code. OHUI stores
current values in the cosave keyed by `mcmId + controlId`. Values
are restored on load. Defaults are applied on first run.

The mod author reads current values via a simple typed API — get bool,
float, or string by MCM ID and control ID. No separate storage. No
JSON config files. No Papyrus properties. OHUI owns the persistence
and the mod reads from it. One source of truth.

### Hot Reload

This is what the declarative architecture enables that the compatibility
layer cannot safely provide.

An MCM definition is a data file. Data files can be reloaded safely
at any time because they have no runtime state. When the file watcher
detects a change to a definition file, OHUI:

1. Parses the updated definition
2. Validates it — syntax errors are logged and the reload is rejected,
   the current definition remains active
3. Diffs the new definition against the current registered state
4. Applies the delta — new controls appear, removed controls disappear,
   changed controls update in place, reordered controls move
5. Re-evaluates all conditions against current values
6. The MCM reflects the new definition immediately

No save. No load. No relaunch. Edit the definition file, save it,
the MCM updates in the running game within seconds.

**What hot reloads safely:**
- Adding, removing, or reordering controls
- Changing labels, descriptions, defaults
- Changing conditions
- Adding, removing, or reordering pages and sections
- Changing slider ranges, dropdown options, format strings

**What requires a session restart:**
- Changing a control's `id` (breaks persistence key — treated as
  remove old + add new, current value lost)
- Changing the MCM's `id` (breaks all persistence — logged as
  a destructive change, requires explicit confirmation)
- Removing a callback name that is currently registered in C++
  (the callback registration is in compiled code, not the definition)

Destructive changes are detected, logged, and flagged in the
Registration State Inspector with a clear explanation of what changed
and what the consequence is. The mod author is never surprised.

**Hot reload for callbacks:**
Callback implementations in Papyrus can be hot-reloaded if the engine
cooperates — the file watcher detects a changed PEX, OHUI attempts
a script reload. This is best-effort and engine-dependent. Callback
implementations in C++ require a relaunch by definition.

The definition and the callbacks have independent reload cycles.
The definition reloads safely always. The callbacks reload if the
engine allows it. The separation means a mod author iterating on
MCM layout — the most common iteration task — never needs to
wait for a full reload cycle regardless of where their callbacks live.

### The Developer Inner Loop

With the declarative API and hot reload the MCM development inner
loop becomes:

```
Edit definition file → Save → MCM updates in running game
```

Seconds per iteration. Compare to the SkyUI MCM development cycle:

```
Edit script → Compile → Save game → Quit → Relaunch → Load save
→ Navigate to MCM → See result
```

Minutes per iteration, minimum. For a non-trivial MCM with many
controls and conditions, an afternoon of iteration work collapses
from hours to minutes.

The Papyrus log in the message log system, the Registration State
Inspector, force callbacks, and hot reload together constitute a
first-class MCM development environment inside the running game.
No external tooling required. No alt-tabbing. No log file hunting.
The tools are where the work is.

### Migration from Compatibility Layer

Mod authors who have existing SkyUI MCM implementations and want to
migrate to the native declarative API can do so incrementally:

1. Write the definition file describing the current MCM structure
2. Register callbacks pointing at the existing Papyrus handler functions
3. Remove the SkyUI registration calls from the Papyrus script
4. The MCM now runs through the native API with the same behaviour

The existing handler functions do not need to change. The persistence
migration is handled by OHUI — values stored in the old SkyUI cosave
format are read and migrated to OHUI's format on first load with the
new definition. The player's settings are preserved.

Migration is optional. The compatibility layer continues to work
indefinitely. Migration is the path to hot reload, reactive conditions,
automatic persistence, and a cleaner authoring experience. It is
never forced.
