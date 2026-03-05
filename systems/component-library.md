# Component Library

## Architecture Position

Components are the floor of the developer space — the smallest meaningful
unit for someone building a widget or a screen. Everything below a component
is implementation detail. Everything above a component is composition.

Components consume tokens (colour, spacing, typography, radius) and produce
rendered output. They are not aware of game state. They do not know what
Skyrim is. They receive data and they draw it.

```
Widget           ← user space — draggable, resizable, bindable to game state
  └─ Component   ← developer space — composable, skinnable, data-agnostic
       └─ Atom   ← code space — text, rectangle, icon, line
            └─ Token  ← code space — colour, spacing, typography, radius
```

---

## Derivation Method

The component list is derived from the complete set of OHUI screens and
systems documented in this repository. Every distinct renderable unit that
appears across those documents — inventory, barter, container, crafting,
magic, map, dialogue, skills, level-up, loading screen, message log, HUD
widget runtime — is extracted, rationalised, and defined below.

No component is invented speculatively. Every component on this list has
at least one concrete usage in a documented screen or system.

---

## Token Layer

Tokens are not components. They are the shared vocabulary that components
consume. Defined here as the foundation components build on.

### Colour Tokens
```
color.text.primary          — main readable text
color.text.secondary        — supporting, label, caption text
color.text.disabled         — unavailable options, locked perks
color.text.positive         — stat gains, positive deltas
color.text.negative         — stat losses, negative deltas, warnings
color.text.warning          — amber warnings, low timers
color.text.critical         — red warnings, expired timers, danger
color.text.lore             — flavour text, book content, lore passages

color.surface.base          — panel backgrounds
color.surface.raised        — elevated panels, tooltips
color.surface.overlay       — modal overlays, edit mode overlay
color.surface.input         — text input backgrounds

color.border.default        — standard panel borders
color.border.focus          — focused/selected element borders
color.border.subtle         — dividers, separators

color.accent.primary        — interactive elements, highlights
color.accent.secondary      — secondary highlights

color.health                — health bar fill
color.stamina               — stamina bar fill
color.magicka               — magicka bar fill
color.xp                    — skill/xp progress fill

color.stolen                — stolen item indicators
color.quest                 — quest item indicators
color.enchanted             — enchanted item indicators
color.sentimental           — sentimental/protected item indicators
color.museum.needed         — LOTD needed item indicators
color.museum.donated        — LOTD already donated indicators
```

### Spacing Tokens
```
spacing.xs   — 2px
spacing.sm   — 4px
spacing.md   — 8px
spacing.lg   — 16px
spacing.xl   — 24px
spacing.xxl  — 32px
```

### Typography Tokens
```
font.size.xs      — captions, inline badges
font.size.sm      — secondary labels, detail panel supporting text
font.size.md      — primary body text, list entries
font.size.lg      — panel headers, section titles
font.size.xl      — screen titles, major headings
font.size.display — level up screen, major announcements

font.weight.regular
font.weight.medium
font.weight.bold

font.family.ui    — primary UI font
font.family.lore  — lore text, book content, Nordic/runic contexts
```

### Radius and Shape Tokens
```
radius.sm   — subtle rounding, badges
radius.md   — panels, inputs
radius.lg   — modals, large surfaces
radius.pill — fully rounded, tags
```

---

## Atom Layer

Atoms are the simplest renderable primitives. Not meaningful on their own
to a developer building a feature — reached for when composing components.

- **Text** — single line or multiline, consumes typography tokens
- **Rectangle** — filled, outlined, or both, consumes colour and radius tokens
- **Line** / **Divider** — horizontal or vertical separator
- **Icon** — single icon from the icon atlas, with optional tint
- **Image** — arbitrary texture, with scale mode (fit / fill / stretch)
- **Viewport** — live 3D render region (character preview, map, loading scene)

---

## Component Library

### 1. Text Components

**Label**
Single line of text. Truncates with ellipsis at boundary. Consumes a
typography token and a colour token.

Usage: every text string in the UI — panel headers, list entry names,
detail panel values, HUD widget readouts.

```
Label(text, style: typography token, color: colour token, truncate: bool)
```

**Caption**
Smaller secondary text, typically below or beside a primary label.
Semantically distinct from Label — not a size variant, a role variant.

Usage: item V:W ratio beneath item name, skill level beneath skill name,
timestamps in message log.

**RichText**
Multiline text with inline formatting support — bold, italic, colour
spans, icon embedding. Used for content that mixes styles within a block.

Usage: item descriptions, spell descriptions, book content, lore text,
tooltip body text.

**LoreText**
RichText with the lore font family applied and appropriate line height
and colour tokens. Semantically distinct — marks content as lore-register
rather than UI-register.

Usage: book content, shout dragon language passages, flavour text panels.

---

### 2. Value Display Components

**StatValue**
A label-value pair displayed together. Label in secondary style, value
in primary style. Optionally shows a unit suffix.

Usage: "Damage  48", "Weight  9.0", "Value  240".

```
StatValue(label, value, unit?)
```

**StatDelta**
A before/after value pair with a delta indicator. The delta is coloured
positive or negative. Optionally shows percentage change.

Usage: item comparison panel, outfit delta panel, level-up attribute
preview, character impact panel.

```
StatDelta(label, before, after, unit?, showPercent?)
// Renders: "Damage  38 → 50  (+12)"  in appropriate colours
```

**DeltaList**
A vertical stack of StatDelta entries under a shared header. The natural
container for item comparison and outfit switching previews.

Usage: item comparison panel, outfit selector delta, level-up attribute
consequence panel.

**ValueBar**
A filled progress bar. Fill colour, background colour, current value,
maximum value. Optional label overlay. Optional segment markers.

Usage: health bar, stamina bar, magicka bar, skill progress bar, loading
bar, soul gem fill indicator, disposition meter, timer drain bar.

```
ValueBar(value, max, fillColor, label?, segments?)
```

**TimerBar**
A ValueBar bound to a countdown. Drains in real time from full to empty.
Colour shifts at registered thresholds — neutral → warning → critical.

Usage: active effect timers at crafting stations, shout cooldown.

```
TimerBar(duration, remaining, thresholds: [{at: seconds, color}])
```

**CountBadge**
A small numeric badge overlaid on another component. Integer value,
pill or circle shape.

Usage: carry count on container screen item entries, notification count
indicators.

---

### 3. List Components

**ListEntry**
A single row in a list. Icon slot, primary label, secondary label, and
a flexible right-side slot for badges, values, or indicators. Selectable,
hoverable, supports context menu trigger.

Usage: inventory item rows, spell list rows, recipe list rows, quest list
rows, message log entries.

```
ListEntry(
    icon?,
    primaryLabel,
    secondaryLabel?,
    rightSlot?,       // badge, value, indicator, or empty
    indicators[],     // inline flag icons — enchanted, stolen, quest, museum
    onSelect,
    onContext
)
```

**ListEntryCompact**
ListEntry with reduced height and tighter spacing. For dense lists where
vertical space is at a premium.

Usage: message log, container screen with large inventories.

**ScrollList**
A vertically scrollable container of ListEntry components. Virtual
scrolling — only visible entries are rendered. Keyboard, mouse wheel,
and controller scroll supported.

Usage: inventory list, spell list, recipe list, quest list, barter
inventory panels, container panels, message log.

```
ScrollList(
    entries[],
    itemHeight,
    onScroll,
    emptyState?  // component to show when list is empty
)
```

**FacetedList**
The primary list surface across inventory, barter, container, magic,
crafting, and quest screens. Replaces the tabbed list pattern entirely.

**Guiding principle: the player starts with everything and narrows to
what they need. They never navigate to a category — they describe
what they want.**

Structure:
- SearchField — always visible, always searches everything
- PresetBar — quick-tap named filter combinations for common views.
  The fast path. Handles 80% of use cases in one tap. Presets are
  named saved states of the facet system, not destinations.
- FacetPanel — full power filter surface. Multi-select within groups,
  additive across groups. On controller: Drawer behind a Filter button.
  On mouse/keyboard: optionally always visible as sidebar.
- ActiveFilterChips — chips showing every active filter, always visible
  below the preset bar. Tapping a chip removes that facet. The player
  always knows why they are seeing what they are seeing.
- ScrollList — always a single unified list. No tabs. No tab-switching
  state. Search always searches everything.

The specific facets, their groupings, and the default presets are not
settled at this layer — registered at construction time by the screen
using this component, and by mods via the facet registration API.
A facet is a named filter predicate. The component does not care
who defined it.

Usage: every screen with a browsable filterable list.

```
FacetedList(
    presets[],         // named preset filter combinations
    facetGroups[],     // registered facet groups and their facets
    sortOptions[],
    onFilterChange,
    onSortChange,
    onSearch
)
```

---

### 4. Navigation Components

**TabBar**
A horizontal row of tabs. Each tab has a label and optional icon. Active
tab is visually distinct. Keyboard, mouse, and controller navigation.

Note: TabBar is no longer the primary navigation pattern for list screens
— FacetedList's PresetBar handles that role. TabBar remains available
for structural screen-level navigation where a small fixed set of
mutually exclusive modes genuinely exists — crafting station modes
(craft / temper), journal sections (quests / misc), settings categories.
Not for content browsing.

**Breadcrumb**
A horizontal path indicator showing current location in a hierarchy.
Tappable segments navigate upward.

Usage: quest journal nested quest → objective navigation, skill tree
navigation.

**Pagination**
Page indicator for multi-page content. Current page, total pages, prev/
next controls.

Usage: anywhere content exceeds a single screen — large quest lists,
extended dialogue history.

---

### 5. Input Components

**TextInput**
Single-line text entry field. Placeholder text, clear button, character
limit. Full OSK support on controller.

Usage: item naming field in enchanting, outfit naming, search fields,
New Game+ legacy summary character name.

**SearchField**
TextInput specialised for search — magnifier icon, clear on escape,
real-time onChange callback. Visually distinct from generic TextInput.

Usage: inventory search, recipe search, spell search, quest search.

**Toggle**
Binary on/off switch. Label, current state, onChange callback.

Usage: MCM2 boolean settings, filter toggles, Take All rule toggles.

**Slider**
Continuous value selector between min and max. Label, current value
display, step size, tick marks optional.

Usage: MCM2 numeric settings, scan distance in SCIE, opacity controls.

**Dropdown**
Single-select from a list of options. Label, current selection, options
list. Keyboard and controller navigable.

Usage: MCM2 choice settings, sort order selectors, skin selectors.

**Stepper**
Discrete increment/decrement control for integer values. Minus button,
value display, plus button.

Usage: quantity selection for stackable items in barter and container
staging, soul gem quantity selection.

**ContextMenu**
A floating menu of actions triggered from a list entry or element.
Actions are registered per-entry. Keyboard, mouse, and controller
navigable. Dismisses on outside click or cancel input.

Usage: inventory item context menu (equip, drop, tag, mark sentimental,
favourite, dedicate), container item context menu, outfit context menu.

---

### 6. Container / Layout Components

**Panel**
A rectangular surface with optional border, background, title bar, and
padding. The primary layout container.

Usage: detail panels, header panels, staging panels, every distinct
surface region in every screen.

**SplitPanel**
Two Panel instances side by side with a configurable size ratio and an
optional draggable divider.

Usage: inventory/container split, barter four-column layout base,
item comparison side-by-side.

**ScrollPanel**
A Panel with overflow scrolling on one or both axes.

Usage: detail panel body when content exceeds fixed height, message log
body, skill description panels.

**Modal**
A Panel that overlays the current screen with a dimmed backdrop. Traps
focus. Has a title, body, and action buttons. Dismissable via cancel
input or backdrop click.

Usage: confirmation dialogs, New Game+ configuration screen, name input
prompts, warning dialogs.

**Tooltip**
A small floating Panel that appears on hover or focus. Positioned
relative to its trigger element, repositioned if it would overflow the
screen edge.

Usage: disposition factor details on hover, enchantment detail on hover,
perk description on hover in skills screen.

**Drawer**
A Panel that slides in from a screen edge on demand and slides out on
dismiss. Does not block the underlying screen.

Usage: "How to Improve This" disposition section in barter, collapsible
stamp history in detail panel, collapsible lore text in shout detail.

---

### 7. Indicator Components

**StatusBadge**
A small coloured pill or chip with a text label or icon. Communicates
categorical status.

Usage: stolen indicator, quest item indicator, enchanted indicator,
museum status (Needed / Donated / Duplicate), book read status,
skill book XP remaining status.

```
StatusBadge(label, color, icon?)
```

**IndicatorDot**
A small coloured circle. No text. Pure status signal.

Usage: unread message log entries, new item indicator in container,
active widget indicator in edit mode.

**ProvenanceStamp**
A formatted display of a single provenance stamp entry. Action type,
character name, skill level, date, optional dedication. Collapsible
history list of multiple stamps.

Usage: item detail panel for player-crafted items, container screen
follower inventory detail, gift staging panel.

**AlertBanner**
A full-width or partial-width banner communicating a warning or
information state. Icon, message, optional action link. Dismissable.

Usage: carry weight warning in container footer, merchant gold
insufficient warning in barter footer, mod list delta warning on
load game screen, missing heirloom warning in New Game+.

**CompletionRing** / **CompletionBar**
Visual indicator of progress toward a threshold. Partial fill,
numerical label, threshold marker.

Usage: skill progress toward next perk level, recipe completion
indicator (materials available vs required), armour cap proximity.

---

### 8. Media Components

**Portrait**
A Viewport constrained to face/bust framing with camera control
disabled. For NPC and player character display in fixed frame.

Usage: dialogue NPC portrait, character preview thumbnail in outfit
selector, follower outfit preview.

**CharacterViewport**
A Viewport with full camera control — rotate, zoom, pan. For full
character inspection.

Usage: inventory character preview, outfit selector full preview,
follower container screen preview.

**SceneViewport**
A Viewport pointing at an authored scene with an animated camera path.
Not player-controllable. For atmospheric display.

Usage: loading screen animated scenes, map screen location preview.

**MapViewport**
A Viewport pointing at the game world from above with map-specific
camera behaviour — pan, zoom, marker overlay.

Usage: world map screen, local map screen.

---

### 9. Specialised HUD Components

These components exist specifically for HUD widget composition. They are
not used in screen UIs.

**ResourceBar**
ValueBar with game-state binding conventions for health, stamina, or
magicka. Includes regen animation, damage flash, and threshold
behaviour. The foundation for the three primary resource HUD widgets.

**ShoutMeter**
TimerBar with shout cooldown binding. Word-count segmentation (1/2/3
word unlock markers on the bar). Fully charged state distinct from
partially charged.

**CompassRose**
A horizontal strip showing cardinal direction, nearby map markers,
and distance indicators. Custom rendering — not composable from
simpler components.

**DetectionMeter**
A visibility/detection state indicator. Discrete states (Hidden,
Caution, Detected) with transitional animation between them. Based
on TrueHUD/Detection Meter patterns — first-party HUD component.

**StealthEye**
The vanilla detection eye reimplemented as a first-party HUD component.
Fillable eye shape, discrete detection states, smooth transition
animation.

**NotificationToast**
A transient message entry that appears, persists for a configured
duration, and fades. Part of the message log system's HUD surface.

---

### 10. Edit Mode Components

These components are only active during HUD edit mode and are never
visible in normal play.

**WidgetBoundingBox**
The visible overlay rectangle drawn around each widget in edit mode.
Resize handles at corners and edges. Widget name label. Selection state
visual treatment.

**AlignmentGuide**
A temporary line drawn when a widget being dragged aligns with the edge
or center axis of another widget. Appears on alignment, disappears
immediately when alignment is broken.

**EditModeToolbar**
The fixed toolbar shown during edit mode. Position coordinate display,
size display, grid snap toggle, profile save/load controls. Fixed to
screen — not a canvas widget.

**GridOverlay**
An optional semi-transparent grid drawn over the canvas during edit mode.
Grid spacing matches the snap increment. Toggle from EditModeToolbar.

---

## Component Count Summary

```
Token categories:      4  (colour, spacing, typography, shape)
Atom types:            6  (text, rectangle, line, icon, image, viewport)
Text components:       4  (Label, Caption, RichText, LoreText)
Value components:      6  (StatValue, StatDelta, DeltaList, ValueBar,
                           TimerBar, CountBadge)
List components:       4  (ListEntry, ListEntryCompact, ScrollList,
                           FacetedList)
Navigation components: 3  (TabBar, Breadcrumb, Pagination)
Input components:      7  (TextInput, SearchField, Toggle, Slider,
                           Dropdown, Stepper, ContextMenu)
Container components:  5  (Panel, SplitPanel, ScrollPanel, Modal, Tooltip,
                           Drawer)
Indicator components:  5  (StatusBadge, IndicatorDot, ProvenanceStamp,
                           AlertBanner, CompletionRing)
Media components:      4  (Portrait, CharacterViewport, SceneViewport,
                           MapViewport)
HUD components:        7  (ResourceBar, ShoutMeter, CompassRose,
                           DetectionMeter, StealthEye, NotificationToast)
Edit mode components:  4  (WidgetBoundingBox, AlignmentGuide,
                           EditModeToolbar, GridOverlay)
────────────────────────────────────────────────────────────────
Total components:     59  (excluding atoms and tokens)
```

---

## Completeness Assessment

This list is derived exhaustively from documented screens and systems.
Every component has at least one concrete usage in the design documents.

**Likely additions as implementation proceeds:**
- Additional HUD widget-specific components as each bespoke widget is
  designed in detail (enemy health bar, active effects widget, etc.)
- Animation primitives once the animation system is defined
- Accessibility variants once the accessibility requirements are defined

**Not on this list by design:**
- Game-specific components below the Component layer (atoms handle this)
- Components that belong inside a single widget and are never reused
- Anything that is a widget rather than a component
