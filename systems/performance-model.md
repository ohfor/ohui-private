# Performance Model

## Purpose

OHUI owns the entire UI surface area. Naively re-rendering everything
every frame would be ruinous. This document defines the performance
model — how OHUI decides what to render, when to render it, and how
widget authors control that decision.

**Start:** the widget runtime manages the canvas and the data binding
engine routes game state to widgets.

**End:** the per-frame cost of a fully populated HUD scales with how
many widgets actually changed, not with how many widgets exist. Widget
authors have explicit, understandable control over their widget's
update cost. No implicit performance surprises.

---

## The Core Principle

**Widgets declare what they need. OHUI delivers exactly that and
nothing more.**

The common case is cheap by default. Expensive update modes require
deliberate opt-in. OHUI does not guess. It does not optimise silently.
It does not compensate for widgets that declare more than they need.
The discipline is in the authoring.

---

## Update Modes

Every widget explicitly declares its update mode. A widget that does
not declare an update mode is reactive — the default, and the
cheapest mode.

### Reactive (default)

The widget declares a set of data binding properties it depends on.
OHUI dirty-flags the widget when any of those values change and
re-renders it. If nothing it depends on has changed since the last
render, it is not touched.

A quest tracker, an active effects display, a notification panel —
these may not update for minutes at a time. There is no reason to
touch them every frame. OHUI does not.

```
widget QuestTracker {
    property questName:    string;
    property objectiveText: string;

    update: reactive(questName, objectiveText);
    ...
}
```

### Throttled Reactive

The widget declares its dependencies reactively but also declares a
maximum update rate. OHUI re-renders it when dependencies change,
but no faster than the declared rate.

This handles the case where a dependency changes every frame but the
widget does not need to reflect every change. The compass is the
canonical example — heading updates continuously as the player moves,
but the compass visual only needs to update 10–15 times per second.
Full frame-driven updates for this would be wasteful. Throttled
reactive gets the right visual result at a fraction of the cost.

```
widget Compass {
    property heading: float;
    property markers: list;

    update: reactive(heading, markers) throttled(15fps);
    ...
}
```

The throttle rate is declared in the widget definition. The player
cannot change it. It is the widget author's performance contract.

### Frame-Driven

The widget explicitly opts into a per-frame update. Used for widgets
with continuous animation or smooth interpolation that genuinely
requires every frame — a health bar lerping toward a new value, a
stamina bar draining in real time, anything where frame-level
smoothness is a deliberate visual choice.

This is an explicit opt-in. A widget author who declares frame-driven
is making a conscious decision to spend frame budget. OHUI does not
silently upgrade widgets to frame-driven.

```
widget StaminaBar {
    property current: float;
    property maximum: float;

    update: per-frame;
    ...
}
```

### Update Mode Summary

| Mode | When renders | Use when |
|------|-------------|----------|
| `reactive` | Dependency changed | Most widgets |
| `reactive throttled(Nfps)` | Dependency changed, max N/sec | Continuously-changing values where visual smoothness at full rate isn't needed |
| `per-frame` | Every frame | Smooth animation / interpolation genuinely required |

---

## Dirty Flagging

The data binding engine maintains a dirty flag per widget. The update
loop at the start of each frame:

1. Polls bound game state values (only values with active reactive
   subscriptions are polled)
2. Compares new values against last known values
3. Sets dirty flag on any widget whose dependency value changed
4. For throttled reactive widgets, checks whether the throttle
   interval has elapsed before setting the flag
5. Frame-driven widgets are always dirty

The render pass processes only dirty widgets. A widget that was not
dirtied this frame is composited from its previous rendered output.
No re-render. No layout pass. The GPU work is a texture sample.

**The practical consequence:** a HUD with twenty widgets where two
are frame-driven costs approximately the same as a HUD with five
widgets where two are frame-driven. Widget count is not the cost
driver. Update mode is.

---

## Polling Efficiency

The data binding engine only polls values that have active reactive
subscriptions. A widget that is hidden is not subscribed. Its
dependencies are not polled. It consumes no update budget.

This means a player with many widgets installed but most hidden —
a common configuration — pays only for the visible ones. The
widgets that are there but not shown cost nothing at runtime.

---

## Widget Author Responsibility

The update mode is part of the widget's public declaration alongside
its data contract. It is visible to anyone reading the widget definition.
It is the author's performance contract with OHUI and with the player.

OHUI provides no automatic optimisation that compensates for a widget
that declares frame-driven when throttled reactive would suffice.
If a widget author declares `per-frame` for a quest tracker, it
re-renders every frame. That is the author's choice and their cost.

**First-party widgets shipping with OHUI use the most conservative
update mode their behaviour allows.** This sets the baseline
expectation for the ecosystem. A widget author looking at a first-
party widget as a reference sees the correct pattern for their use
case.

---

## Frame Budget Guidelines

These are guidelines for widget authors, not enforced limits. OHUI
does not cap per-widget render time. Authors are expected to be
responsible.

| Widget complexity | Recommended mode |
|------------------|-----------------|
| Static display (clock, gold, carry weight) | `reactive` — update only when value changes |
| Occasionally-changing display (quest tracker, active effects) | `reactive` — may not update for minutes |
| Continuously-moving indicator (compass, detection state) | `reactive throttled(15fps)` |
| Smoothly-animated bar (health, stamina, magicka) | `per-frame` with lerp |
| Complex animated widget (sneak eye, shout meter) | `per-frame` only if interpolation required, otherwise `throttled` |

---

## Native C++ Widget Performance

Native C++ widgets that implement the viewport contract own their
own rendering performance. OHUI does not manage their update
scheduling — they are called every frame by definition, because OHUI
cannot know whether their internal state has changed without calling
them.

Native widget authors are responsible for their own dirty tracking
and early-exit behaviour:

```cpp
void MyNativeWidget::Draw(const WidgetContext& ctx) override {
    if (!m_dirty) return;  // nothing changed, skip render

    // ... render into ctx.viewport ...

    m_dirty = false;
}
```

OHUI provides no dirty flag infrastructure for native widgets.
They bring their own.

---

## Memory Model

Rendered widget output is cached as a texture in OHUI's compositor.
A widget that was not dirtied this frame is composited from the
cached texture. The cache is per-widget, sized to the widget's
current bounding rectangle.

Memory pressure from the cache scales with total widget surface area
— the sum of all widget bounding rectangles. Large widgets with large
bounding rectangles consume more cache memory than small widgets.
Edit mode chrome (bounding boxes, handles) is not cached — it is
rendered directly and only during edit mode.

The compositor discards cached textures for hidden widgets. Memory
pressure scales with visible widget surface area, not installed
widget count.

---

## Open Questions

1. **Scaleform compositor vs DirectX compositor** — the decision
   about whether the compositor lives in Scaleform or in a C++
   DirectX layer above it affects how the cache is implemented and
   how third-party viewport rendering integrates. Needs resolution
   before the rendering architecture is locked.

2. **Polling rate** — the data binding engine polls game state values
   at the start of each frame. For values that are expensive to poll
   (require crossing the Papyrus boundary), a separate polling rate
   lower than frame rate may be warranted. Needs profiling data to
   decide.
