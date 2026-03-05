# HUD Widget Runtime

## Foundation

The HUD is a canvas. The canvas contains widgets. Every element of the
gameplay UI — health bar, stamina bar, compass, shout meter, quest
tracker, notification area, detection meter, enemy health — is a widget
on that canvas. Nothing is hardcoded to a position. Nothing is hardcoded
to a size. Nothing is hardcoded to a visual style.

The runtime does one thing: it manages widgets on a canvas. It does not
know what any widget displays. It does not know what any widget means.
It knows where widgets are, how large they are, and whether they are
visible. Everything else belongs to the widget itself.

---

## The Primitive Widget

The foundation of the entire system is the simplest possible widget: a
static text label. No data binding. No game state. No update loop. Text
that exists somewhere on the screen, showing whatever string it was
given when it was created.

This widget is not a toy or a tutorial example. It is the complete and
sufficient expression of every principle the runtime is built on. Every
widget in the system — no matter how complex — is this widget plus
elaboration. The runtime does not distinguish between a static text label
and a health bar. They are the same kind of thing. One has more going on
inside it. The runtime does not care about that.

**A widget is:**
- A rectangular region on the canvas with a position and a size
- A visible or hidden state
- A skin that defines how it draws itself within that region
- Nothing else, from the runtime's perspective

The data a widget displays, the game state it reads, the animations it
runs — these are the widget's internal concerns. The runtime never touches
them.

---

## The Three Operations

Every widget in the system supports exactly three operations at the
runtime level. A static text label supports them. A health bar supports
them. A compass supports them. The operations are:

**Activate** — bring the widget into existence on the canvas, or restore
it from a hidden state. A widget that is not activated is not on the
canvas. It consumes no resources, occupies no position, draws nothing.

**Move** — change the widget's position on the canvas. The widget's
anchor point moves to a new coordinate. Everything about the widget's
internal state, data binding, and appearance is unchanged. Only its
position changes.

**Resize** — change the widget's bounding rectangle. The widget's skin
is responsible for adapting its visual presentation to the new size. The
runtime enforces minimum and maximum size constraints registered by the
widget. Within those constraints, any size is valid.

These three operations are the complete primitive set. No widget requires
a fourth operation at the runtime level. Complexity lives inside widgets,
not in the operations the runtime performs on them.

---

## Edit Mode

Edit mode is the single gateway to all three operations. Outside of edit
mode, widgets are static. Their positions, sizes, and visibility states
are fixed at whatever the player last configured. The runtime enforces
this. No operation can be performed on a widget's position, size, or
visibility outside of edit mode.

Edit mode is entered by a dedicated keybind — configurable, default
assigned. On entry:

- All active widgets gain a visible bounding rectangle overlay
- Widget labels appear showing each widget's name
- The game is paused or slowed (configurable — full pause or slow
  motion, default full pause)
- A minimal edit mode toolbar appears at a fixed screen position
  outside the widget canvas

On exit, the current layout is saved automatically. No explicit save
action required. Edit mode entry and exit are the save boundaries.

### Moving a Widget

In edit mode, any widget can be grabbed by its bounding rectangle and
dragged to a new position. The widget snaps to a configurable grid
(default 4px, configurable down to 1px for pixel-perfect placement).
Grid snapping can be suspended by holding a modifier key for free
positioning.

While dragging, the widget's current coordinates are displayed in the
toolbar. The player can read the exact position and type a coordinate
directly into the toolbar fields for precise placement.

Widgets can be snapped to each other — dragging one widget near another
shows alignment guides and snaps to the nearest edge or center axis of
the target widget. This is how players align a health bar and a stamina
bar without manual coordinate matching.

### Resizing a Widget

In edit mode, resize handles appear at the corners and edges of each
widget's bounding rectangle. Dragging a handle resizes the widget in
that direction. The widget's skin adapts to the new size in real time
as the handle is dragged.

The runtime enforces the widget's registered minimum size — a widget
cannot be resized below the minimum its skin can render meaningfully.
The runtime enforces no maximum by default; widgets can be made
arbitrarily large unless they register a maximum.

Aspect ratio lock is available per widget — some widgets register a
preferred aspect ratio and the resize operation maintains it unless
the player explicitly overrides.

### Showing and Hiding a Widget

In edit mode, every registered widget is visible in the edit mode
overlay — including widgets that are currently hidden in normal play.
Hidden widgets appear dimmed with a distinct visual treatment to
distinguish them from active widgets.

Clicking a widget in the edit mode overlay toggles its active state.
A widget toggled off is deactivated — removed from the canvas in
normal play, invisible to the player, consuming no resources. It
remains in the registry and can be reactivated at any time.

---

## The Widget Contract

Every widget that participates in the runtime fulfils a contract. The
contract is minimal. It does not prescribe what a widget displays or
how it updates. It prescribes only what the runtime needs to manage
the widget on the canvas.

**A widget must provide:**

```cpp
struct WidgetManifest {
    std::string id;          // Unique identifier. Stable across versions.
    std::string displayName; // Human-readable name shown in edit mode.
    Vec2        defaultPos;  // Starting position on a 1920×1080 canvas.
                             // Runtime scales to actual resolution.
    Vec2        defaultSize; // Starting size.
    Vec2        minSize;     // Minimum size the skin can render.
    Vec2        maxSize;     // Maximum size. {0,0} = unconstrained.
    bool        defaultVisible; // Whether active by default on first run.
};
```

That is the complete contract. The runtime asks for nothing else.

**The runtime provides to every widget:**

- Its current position on the canvas (updated when moved)
- Its current size (updated when resized)
- Its current visible state
- Entry and exit notifications for edit mode
- Entry and exit notifications for the widget's own active state

The runtime does not provide game state, player data, or any Skyrim-
specific information. That is not the runtime's concern. Widgets that
need game state acquire it themselves through their own data bindings.
The runtime manages the canvas. Widgets manage their own content.

---

## Layout Profiles

A layout profile is a complete snapshot of the canvas state — every
widget's position, size, and visibility at a moment in time. Profiles
are named and stored in the cosave.

The player can save the current layout as a named profile at any time
from edit mode. They can load any saved profile. Loading a profile
moves all widgets to their saved positions and restores their saved
visibility states in a single operation.

**Built-in profiles:**
- Default — OHUI's designed default layout, restored if the player
  wants to start over
- Minimal — essential widgets only, everything else hidden
- Controller — layout optimised for gamepad play, widgets positioned
  away from stick travel zones

Players create named profiles for their own purposes. A player who
switches between gameplay modes — combat-heavy vs exploration-heavy
— saves a profile for each and switches between them from the tween
menu without entering edit mode.

Profile switching is instant. No transition, no animation. The canvas
snaps to the new layout.

---

## Resolution and Scaling

The canvas is defined in a 1920×1080 coordinate space. Widget positions
and sizes are stored in this space. The runtime scales to the player's
actual resolution at render time.

A widget positioned at (960, 540) in a 1920×1080 canvas definition is
at the center of the screen at any resolution. A widget with a width of
200 in the canvas definition occupies the same proportional screen
fraction at 2560×1440 as at 1920×1080.

Ultrawide support is a first-class concern. Widgets positioned near
screen edges in 16:9 are not orphaned at 21:9. The runtime provides
an anchor system — widgets can be anchored to screen edges or the
screen center, and their positions are calculated relative to their
anchor point after resolution scaling. A widget anchored to the right
edge stays near the right edge at any aspect ratio.

---

## The Simplest Complete Example

A static text widget. The complete implementation from the runtime's
perspective:

```cpp
// Widget registration
WidgetManifest manifest {
    .id             = "ohui.example.static_label",
    .displayName    = "Example Label",
    .defaultPos     = {100, 100},
    .defaultSize    = {200, 30},
    .minSize        = {40, 16},
    .maxSize        = {0, 0},   // unconstrained
    .defaultVisible = true
};
Runtime::RegisterWidget(manifest, &ExampleLabelWidget::instance);

// Widget implementation
class ExampleLabelWidget : public IWidget {
    void Draw(const WidgetContext& ctx) override {
        DrawText(ctx.bounds, "Hello, Skyrim", ctx.skin->GetTextStyle());
    }
};
```

The runtime handles everything else. Position, size, visibility, edit
mode interaction, layout persistence, resolution scaling. The widget
draws its content into whatever bounds the runtime provides.

A health bar is this widget with a data binding to the player's health
value and a skin that draws a filled rectangle instead of text. The
runtime does not know it is a health bar. It sees a widget at a position
with a size. The elaboration is entirely internal to the widget.

---

## What This Enables

Every feature described in every other OHUI document builds on this
foundation without modifying it.

The edit mode described in `edit-mode.md` is the player-facing surface
of the three operations defined here. The skin system described in the
DSL document is the contract between a widget's data and its visual
presentation. The layout profiles here are what the tween menu switches
between. The widget manifest here is what mod authors implement to add
new widgets to the canvas.

The foundation does not grow. Features are added by adding widgets,
adding skins, and adding data bindings. The runtime stays small,
stays stable, and stays ignorant of everything above it.

This is the correct architecture for a system that needs to outlast
the mods built on it.
