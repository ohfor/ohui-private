# Input Handling

## Purpose

OHUI owns the entire UI surface area. It therefore owns the input
layer too. This document defines how input is managed, coordinated,
and exposed to mod authors across the HUD, all menu screens, and
edit mode.

**Start:** the widget runtime and component library exist. Widgets
and screens need to receive input without conflicting with each other
or with the game.

**End:** any mod author can declare a named input context, register
named actions within it, provide default bindings, and receive action
events — without knowing what any other mod is doing, without
conflicting with vanilla bindings, and without writing binding
management code.

---

## The Problem

Skyrim's input system is one of the most significant pain points in
the modding ecosystem. Mods that need keybinds either hardcode them,
hook SKSE's global input events, or fight through MCM's keybind
capture — with no coordination mechanism between them. Two mods
claiming the same key results in both firing simultaneously.

On controller the problem is worse. The button vocabulary is tiny.
Vanilla bindings consume most of it. Mods that add controller support
do so by intercepting combinations or hijacking buttons in specific
contexts with no awareness of anything else.

OHUI solves this structurally. The solution is a context stack.

---

## Controller-First

OHUI is designed controller-first. Keyboard and mouse are fully
supported, but controller is the primary design target for all
navigation, focus management, and interaction models.

Every menu, every widget, every component in the library must be
fully navigable without a mouse. This is a hard requirement, not a
best-effort aspiration. Skyrim is heavily played on controller. The
UI modding ecosystem has historically treated controller support as
an afterthought. OHUI treats it as the baseline.

---

## The Input Context Stack

At any given moment exactly one input context is active. That context
declares the complete mapping of inputs to named actions for its
duration. Contexts stack — when a menu opens it pushes a new context,
when it closes the previous context is restored.

```
[Gameplay context]           ← base, always present
  [Inventory context]        ← pushed when inventory opens
    [Item detail context]    ← pushed when inspecting an item
      [Confirm dialog]       ← pushed on destructive action
```

Only the top context receives input. Nothing below it fires. No two
things can own the same input simultaneously because only one context
is ever active.

The DEL key doing five things at once is structurally impossible.
Each thing that wants DEL registers it in its own named context.
OHUI routes it exclusively to whichever context is currently on top.

For controller this is transformative. The same button means
"confirm" in a menu context, "attack" in gameplay context, and
"select widget" in edit mode context — without any mod needing to
know what any other mod is doing. Each context owns the complete
controller vocabulary for its duration.

---

## Action-Based Binding

Mods do not register handlers for physical inputs. They register
**named actions** in a named context. Physical inputs are mapped to
actions separately, by the player or by default bindings.

A mod declares a named context, then registers named actions within
it — each with a display name, default keyboard key, default controller
button, and handler. Registration is available from both native code
and Papyrus.

**Consequences:**

- A mod that wants a "toggle feature" action defines the action. The
  player binds it to whatever they choose. The mod never knows or
  cares which physical input triggered it.
- Rebinding requires no mod support — the player changes the physical
  mapping, all actions that used that input update automatically.
- Controller and keyboard bindings are independent. An action can
  have both simultaneously. The active input device determines which
  is shown in button prompts.

---

## Context Lifecycle

### Pushing a Context

The pushed context becomes the active context immediately. All input
is routed to it. The previous context receives nothing.

### Popping a Context

The context is removed from the stack. The context below it becomes
active again. If the specified context is not the top of the stack,
a warning is logged — contexts must be popped in the reverse order
they were pushed.

### Context Ownership

Contexts are owned by the UI surface that pushes them. A menu that
opens pushes its context. When the menu closes — for any reason,
including being closed programmatically — its context is popped.
The stack never leaks. A context that is no longer relevant cannot
receive input.

OHUI enforces this by tying context lifetime to the menu or widget
that pushed it. If a widget is destroyed without explicitly popping
its context, OHUI pops it automatically and logs a warning.

### Edit Mode Context

Edit mode is its own context. While active, it captures the full
input vocabulary for drag, resize, confirm, and cancel operations.
Nothing in the game or any other mod fires during edit mode. Exiting
edit mode pops the context and restores whatever was active before.

---

## Button Prompts

Button prompts are a first-class UI concept, not a hardcoded texture.

OHUI knows the active input device — keyboard/mouse, Xbox controller,
PlayStation controller, Steam Deck — and renders the appropriate
prompt icon for any given action automatically.

Mod authors reference actions by name:

```
// In a widget or screen definition
ButtonPrompt {
    action: "com.mymod.browsing.select";
    label:  "Select";
}
```

OHUI resolves the correct visual representation at render time.
Switching from keyboard to controller mid-session updates all visible
prompts immediately. No mod author work required.

### Device Detection

Device is detected from the most recent input event. If the player
uses a keyboard key, device switches to keyboard/mouse. If they use
a controller button, device switches to controller. The switch is
instantaneous and affects all visible prompts.

### Controller Type

Xbox, PlayStation, and Steam Deck button glyphs are distinct. OHUI
detects the controller type from the device ID reported by the OS
input system and selects the correct glyph set. Skin authors provide
glyph atlases for each controller type as part of the skin's assets.

First-party OHUI skins ship glyphs for all supported controller types.
Skin authors who do not provide their own fall back to OHUI's defaults.

---

## Default Contexts

OHUI ships a set of first-party contexts covering vanilla Skyrim's
input surfaces. These are the contexts active during normal play:

**gameplay** — the base context. Always present at the bottom of the
stack. Arrow keys, WASD, attack, sprint, jump, interact — all vanilla
bindings. Never popped.

**menu.inventory** — active when the inventory is open.

**menu.barter** — active when the barter screen is open.

**menu.magic** — active when the magic screen is open.

**menu.map** — active when the map is open.

**menu.journal** — active when the quest journal is open.

**menu.crafting** — active during any crafting screen.

**menu.dialogue** — active during dialogue.

**menu.skills** — active when the skills screen is open.

**hud.edit** — active during HUD edit mode.

**hud.radial** — active while the favourites radial is open.

Each first-party context is documented in its respective screen
document. Mod authors can register their own contexts alongside these.

---

## Mod Author Contract

A mod author interacts with the input system by:

1. Declaring a named context
2. Registering named actions within that context with default bindings
3. Pushing the context when their UI surface opens
4. Listening for action events
5. Popping the context when their UI surface closes

OHUI handles routing, conflict prevention, rebinding, prompt
rendering, and device detection. The mod author handles what their
action does.

A mod author never:
- Registers handlers for physical inputs directly
- Reads key state or button state from the engine
- Tries to coordinate with other mods about which keys they use
- Worries about what platform the player is on

---

## Rebinding

Players rebind actions in OHUI's settings screen under Controls.
The rebinding surface shows all registered contexts and their actions
with current bindings. Each action can be rebound independently for
keyboard/mouse and for controller.

Conflicts are detected at rebind time. If a player assigns a key to
an action in context A that is already bound to a different action
in context A, OHUI warns and asks for confirmation. Bindings in
different contexts never conflict by definition — only one context
is active at a time.

The complete binding map is stored in the cosave per character.
A player who switches characters gets that character's binding
configuration.

---

## Open Questions

1. **Wait Menu** — the engine binds T directly to its native wait
   menu, bypassing normal input handling. Replacing the wait menu
   likely requires menu registration replacement at the engine level
   rather than a context push. Needs investigation.

2. **VR input** — VR has a fundamentally different input model.
   Controller-first assumptions need to be validated against VR's
   motion controller input before the architecture is locked.

3. **Gamepad combination inputs** — some mods use button combinations
   (LB + A) as distinct inputs. Whether OHUI's context system should
   natively support combination inputs or whether this is mod
   responsibility needs a decision.
