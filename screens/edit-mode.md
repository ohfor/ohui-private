# Edit Mode

> Screen design doc. Covers design intent, interaction model, widget chrome, context menu,
> and settings vocabulary. Implementation detail is out of scope here.

---

## Design Intent

Edit mode is the player's control surface for the entire HUD. It should feel powerful,
immediate, and trustworthy — the player should never feel like they're fighting the tool
or worrying about breaking something. Every action is undoable. The default layout is
always recoverable.

The reference model is World of Warcraft's edit mode — the most battle-tested widget
layout system in gaming, refined over two decades of real-world use by millions of players.
Players who have used WoW's edit mode understand OHUI's immediately. That recognition is
worth more than novelty.

---

## Entering and Exiting Edit Mode

Edit mode is entered via a dedicated keybind (default: configurable, no vanilla conflict)
or via the OHUI settings menu. The game slows to a configurable timescale on entry
(default: 10% — nearly paused but not fully frozen). The HUD remains live — health bars
still update, the compass still reflects actual heading. The player is editing their
layout against a real game state at near-standstill, which means they can see exactly
how their layout behaves before committing without worrying about incoming damage.

The timescale is player-configurable from 0% (full pause) to 100% (real time). Full
pause is available for players who want it. Real time is available for players who want
to test their layout under live conditions. The default of 10% is the practical sweet
spot: effectively paused for purposes of safety, live enough to show widget animations
and data updates.

Exiting saves the current layout automatically. A separate "save as preset" option is
available. The previous layout is preserved as an undo state for the session.

---

## Widget Chrome

When edit mode is active every widget gains a chrome overlay — a non-intrusive set of
controls that make it editable without obscuring what it is.

### Dotted outline

A dotted border traces the widget's current bounds. Communicates "this is a discrete,
moveable thing" without the visual weight of a solid border. Colour is skin-defined but
must have sufficient contrast against both light and dark backgrounds — white dotted
outline with a subtle dark offset is the safe default.

### Title bar

A slim title bar appears above the widget showing its name — "Health Bar", "Compass",
"Enemy Health Bars". This solves the disambiguation problem: on a busy HUD, overlapping
or adjacent widgets need labels so the player knows what they're interacting with. The
title bar is part of the chrome, not part of the widget — it does not appear during
normal gameplay.

### Resize handles

Eight handles: four corners, four edge midpoints. Standard resize affordance. Dragging
a corner handle resizes proportionally by default; holding a modifier key allows
freeform resize. Edge handles resize along a single axis.

Not all widgets are resizable on all axes — a widget that is inherently fixed-ratio
(a circular compass rose, for example) constrains the resize handles appropriately.
Handles that are not available are not shown.

### Drag affordance

The entire widget body is a drag target. Cursor changes to a move cursor on hover.
Dragging moves the widget. Snap-to-grid is active by default (grid size configurable);
holding a modifier key enables free positioning.

---

## Context Menu

Right-clicking a widget (or pressing the context button on controller) opens a context
menu anchored to the widget. This is the primary settings surface for individual widgets.

### Structure

The context menu is divided into sections. The sections and their contents are consistent
across all widgets, with widget-specific entries added where relevant.

**Identity** (top, non-interactive)
The widget name and current skin name. Orientation — the player knows what they're
configuring before they interact with anything.

**Skins**
A submenu listing all available skins for this widget. The active skin is checked.
Selecting a skin applies it immediately — the widget updates live behind the open menu.
Skin names come from the skin author's metadata. OHUI ships default skins; community
skins appear here automatically once installed.

**Variations**
Some widgets have named variations that change their behaviour or data presentation
without being a full skin change. Examples: the compass can show cardinal directions
only, or cardinal + ordinal, or a full 360° strip. Enemy health bars can show one
target or all nearby targets. These are not skin choices — they are configuration
choices. Listed here as named options.

**Display**

- **Always show** — widget is permanently visible regardless of game state
- **Show as needed** — widget appears only when relevant (health bar appears when
  health is not full, sneak indicator appears when sneaking, etc.). This is the
  default for most widgets.
- **Always hide** — widget is disabled. Remains in the layout but does not render
  during gameplay. Useful for temporarily disabling a widget without losing its
  position.

**Appearance**

- **Scale** — a slider, 50%–200%, relative to the widget's default size. Independent
  of the global UI scale setting.
- **Opacity** — a slider, 10%–100%. Applies to the entire widget uniformly.
- **Opacity (out of combat)** — some players want widgets to fade when not in combat
  and return to full opacity when combat begins. A separate out-of-combat opacity
  setting enables this. If equal to the main opacity, no fade occurs.

**Position**

- **Reset position** — snaps the widget back to its default position for the active
  layout preset.
- **Reset to defaults** — resets all settings for this widget (position, scale,
  opacity, skin, variation) to their defaults.

**Widget-specific settings**
Any settings specific to this widget type appear in a clearly labelled section below
the universal settings. Examples: the compass might expose "show location name",
"show direction text". The enemy health bar might expose "max visible targets",
"show level indicator". These are defined by the widget author.

---

## Global Edit Mode Controls

Controls that apply to the whole layout rather than a single widget, accessible via
a persistent toolbar while edit mode is active.

- **Undo / Redo** — full history for the edit session. Ctrl+Z / Ctrl+Y on keyboard;
  dedicated buttons on controller.
- **Save as preset** — saves the current layout as a named preset. Prompts for a name.
- **Load preset** — opens a preset picker. Applying a preset replaces the current
  layout (with confirmation). Current layout is saved as an undo state first.
- **Reset all to defaults** — resets the entire layout to the default for the active
  aspect ratio preset. Requires confirmation. Undoable.
- **Toggle snap to grid** — enables or disables grid snapping globally for the session.
- **Grid size** — sets the snap grid granularity (fine / medium / coarse, or a specific
  pixel value).
- **Exit edit mode** — saves and exits. Also triggered by the edit mode keybind or
  pressing B/Escape.

---

## Controller Navigation

Edit mode must be fully navigable without a mouse. Controller players are first-class
participants.

- **Left stick / D-pad**: move the selected widget in fine increments (D-pad) or
  continuously (left stick). Snap-to-grid applies.
- **Right stick**: scroll the widget list if navigating via list rather than direct
  selection.
- **LB / RB**: cycle through widgets in order (useful on a busy HUD where direct
  selection is fiddly).
- **A**: select / confirm
- **B**: cancel / deselect / exit edit mode
- **X**: open context menu for selected widget
- **Y**: open global edit mode toolbar
- **LT / RT**: resize selected widget (LT = shrink, RT = grow, proportional by default)

---

## Snap and Alignment Aids

- **Grid snap**: widgets snap to a configurable grid during drag. Visual grid overlay
  is shown while dragging (not constantly — only when a widget is in motion).
- **Edge snap**: widgets snap to the edges of other widgets and to the screen edges
  when dragged close to them. A snap line appears briefly to indicate the alignment.
- **Centre lines**: a widget being dragged near the screen centre snaps to the
  horizontal and vertical centre lines. Indicated by a crosshair line.
- **Distribute**: if multiple widgets are selected, a "distribute evenly" action
  spaces them uniformly along the selected axis.

---

## Multi-Select

Holding Shift and clicking (or LB+A on controller) adds a widget to the selection.
All selected widgets can be moved together. Context menu in multi-select mode shows
only settings that apply to all selected widgets (opacity, display mode) — widget-
specific settings are not shown.

---

## Visual Design Notes

These are design intent notes, not skin specifications. The skin implements these
in whatever visual language is appropriate.

- Chrome must not compete visually with the widget content. The dotted outline and
  handles should read clearly but recede when the player is not actively interacting.
- The selected widget should have a visually distinct state from unselected widgets —
  a brighter outline, a fill tint, or equivalent.
- Dragging a widget should feel immediate — zero lag between cursor and widget movement.
  This is a feel requirement, not an animation one.
- The context menu should feel anchored to the widget — positioned relative to it,
  not dropped in a fixed screen position.
- The global toolbar should not obscure the HUD. Top of screen or a collapsible
  side panel are appropriate positions.

---

## Open Questions

1. **Live preview of skins**: when hovering over a skin name in the context menu,
   should the widget preview that skin before it is selected? This is a nice-to-have
   but requires the skin to be loaded in memory before selection.

2. **Preset sharing**: presets as shareable files — a player exports their layout,
   shares it, another player imports it. The layout profile system supports this in
   principle. The sharing mechanism (Nexus? In-game import?) needs a decision.

3. **Per-character vs global layouts**: layout profiles are global assets — files on
   disk, available to all characters, shareable and installable like any other mod asset.
   Which profile is active for a given character is cosaved — persisted in the SKSE
   cosave alongside other character-specific state. A stealth character and a mage can
   use different layouts without any conflict. If a cosaved profile reference cannot be
   resolved (the profile was deleted or renamed), OHUI falls back to the default preset
   for the active aspect ratio and notifies the player.

4. **Widget grouping**: should widgets be groupable — moved and scaled together as a
   unit? Useful for players who want to keep related widgets (health/magicka/stamina)
   spatially locked together. Adds complexity to the interaction model.
