# Performance Model

## Purpose

OHUI owns the entire UI surface area. Naively re-rendering everything
every frame would be ruinous. This document defines the performance
model — how OHUI decides what to render, when to render it, and how
widget authors control that decision.

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

### Throttled Reactive

The widget declares its dependencies reactively but also declares a
maximum update rate. OHUI re-renders it when dependencies change,
but no faster than the declared rate.

This handles the case where a dependency changes every frame but the
widget does not need to reflect every change. The compass heading
updates continuously but the compass visual only needs 10-15 updates
per second. Throttled reactive gets the right visual result at a
fraction of the cost.

The throttle rate is declared in the widget definition. It is the
widget author's performance contract.

### Frame-Driven

The widget explicitly opts into a per-frame update. Used for widgets
with continuous animation or smooth interpolation that genuinely
requires every frame — a health bar lerping toward a new value, a
stamina bar draining in real time.

This is an explicit opt-in. A widget author who declares frame-driven
is making a conscious decision to spend frame budget. OHUI does not
silently upgrade widgets to frame-driven.

### Summary

| Mode | When it renders | Use when |
|------|----------------|----------|
| Reactive | Dependency changed | Most widgets |
| Throttled reactive | Dependency changed, max N/sec | Continuously-changing values where full-rate smoothness is not needed |
| Frame-driven | Every frame | Smooth animation genuinely required |

---

## Render Cost

Only widgets whose dependencies have changed are re-rendered. A widget
whose values have not changed since the last render is composited from
its previous output. No re-render. No layout pass.

**The practical consequence:** a HUD with twenty widgets where two
are frame-driven costs approximately the same as a HUD with five
widgets where two are frame-driven. Widget count is not the cost
driver. Update mode is.

---

## Hidden Widget Cost

A hidden widget consumes no update budget. Its dependencies are not
tracked. Only visible widgets with active subscriptions cost anything.

A player with many widgets installed but most hidden pays only for
the visible ones.

---

## Widget Author Responsibility

The update mode is part of the widget's public declaration alongside
its data contract. It is visible to anyone reading the widget
definition. It is the author's performance contract with OHUI and
with the player.

OHUI provides no automatic optimisation that compensates for a widget
that declares frame-driven when throttled reactive would suffice.

First-party widgets shipping with OHUI use the most conservative
update mode their behaviour allows, setting the baseline expectation
for the ecosystem.

---

## Frame Budget Guidelines

Guidelines for widget authors, not enforced limits:

| Widget complexity | Recommended mode |
|------------------|-----------------|
| Static display (clock, gold, carry weight) | Reactive |
| Occasionally-changing (quest tracker, active effects) | Reactive |
| Continuously-moving indicator (compass, detection) | Throttled reactive (~15fps) |
| Smoothly-animated bar (health, stamina, magicka) | Frame-driven with lerp |
| Complex animated widget (sneak eye, shout meter) | Frame-driven only if interpolation required, otherwise throttled |

---

## Memory Model

Rendered widget output is cached per widget. A widget that was not
dirtied is composited from cache. Hidden widgets have their cache
discarded. Memory pressure scales with visible widget surface area,
not installed widget count.

---

## Open Questions

1. **Compositor architecture** — whether the compositor lives in the
   Scaleform layer or in a native layer above it affects cache
   implementation and third-party viewport integration. Needs
   resolution before the rendering architecture is locked.

2. **Expensive poll sources** — values that are expensive to read may
   need a separate, lower poll rate. Needs profiling data to decide.
