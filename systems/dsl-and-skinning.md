# DSL and Skinning

## Purpose

This document defines the widget authoring layer — the DSL mod authors
use to define widgets and skins, the USS/Yoga style and layout system
those definitions are expressed in, and the rendering paths available
to widgets that need more than the DSL provides.

**Start:** the component library exists and the widget runtime manages
the canvas. A widget author needs a way to declare a widget and a skin.

**End:** a widget author can write a `.widget` definition file and a
`.uss` skin file, register them, and have a fully functional widget
with edit mode, layout profiles, skin switching, and input coordination
— without writing any C++ or touching Scaleform.

---

## The Three-Layer Model Within a Widget

A widget has three separable concerns. Getting this separation right
is what makes skins cheap to author and easy to maintain.

**Logic / behaviour** — what the widget binds to, how it responds to
input. Game-agnostic. Lives in the widget definition and the data
binding schema. Never changes between skins.

**Structure** — the component tree. A button always has a container
and a label. That does not change between skins. A Nordic UI button
and an Edge UI button have the same structure — different materials,
same bones. Defined in the widget definition. Overridable via
ControlTemplate for skins that genuinely need different structure.

**Style** — the visual properties applied to that structure: colours,
typography, spacing, borders, effects, animation. This is the CSS
layer. Flows from design tokens. A skin is primarily a token override,
not a full reimplementation.

The practical consequence: the barrier to creating a skin is low.
Applying a consistent visual identity across every widget, every menu,
and every component requires changing one thing in one place — the
token set.

---

## The DSL

OHUI ships a custom declarative authoring DSL. Not XAML, not QML,
not Slint — inspired by Slint's property-binding model but scoped
tightly to the domain.

**The DSL is game-agnostic by enforced constraint.** If something
cannot be expressed without naming a game-specific concept, it belongs
in the data binding schema layer instead. The DSL describes what a
widget is and what properties it has. The schema describes what
Skyrim-specific values those properties are bound to.

### Widget Definition

```
widget HealthBar {
    // Data contract — game-agnostic typed properties
    property current:  float;
    property maximum:  float;
    property color:    color;
    property label:    string;

    // Structure — component tree
    Panel {
        class: "health-bar-container";

        ValueBar {
            value:      current;
            max:        maximum;
            fill-color: color;

            animate value {
                duration: 150ms;
                easing:   ease-out;
            }
        }

        Label {
            text:  label;
            class: "health-bar-label";
        }
    }
}
```

The widget declares its data contract as typed properties. The runtime
feeds those properties via the data binding schema. The structure
defines which components compose the widget. Classes connect the
structure to the style layer.

### Data Binding Schema

The schema declares how the DSL's game-agnostic properties connect
to live Skyrim data. One schema per game. The DSL is unaware of it.

```
// Skyrim data binding schema (excerpt)
binding player.health.current  → float
binding player.health.maximum  → float
binding player.shout.cooldown  → float
binding player.level           → int
```

Widget instances declare which schema bindings feed their properties:

```
HealthBar {
    current: bind(player.health.current);
    maximum: bind(player.health.maximum);
    color:   token(color.health);
    label:   "Health";
    update:  reactive(player.health.current, player.health.maximum);
}
```

The widget never names Skyrim. The schema names Skyrim. The binding
declaration connects them. Porting to FO4 means writing a new schema.
The widget definition does not change.

### Skin Definition

A skin is an alternate visual implementation of a widget's property
contract. Same properties. Different structure and/or style.

```
skin HealthBar "Nordic UI" {
    // Token overrides — the primary skin mechanism
    tokens {
        --bar-fill-color:       #8B0000;
        --bar-background-color: #1a0a0a;
        --bar-border-radius:    2px;
        --bar-height:           16px;
    }

    // Optional ControlTemplate — only when structure must differ
    template {
        Panel {
            class: "nordic-health-container";

            // Nordic UI uses a different layout — icon left of bar
            Icon {
                source: "icons/health_nordic.dds";
            }

            ValueBar {
                value:      current;
                max:        maximum;
                fill-color: var(--bar-fill-color);
            }
        }
    }
}
```

A skin that only needs different colours provides only the tokens
block. OHUI applies the overrides and the widget re-renders. No
ControlTemplate required. The ControlTemplate is the escape hatch
for skins that need genuinely different structure — available when
needed, not required by default.

### Minimum Version Declaration

```
widget MyWidget {
    requires ohui >= 1.3;
    ...
}
```

OHUI checks at load time. If the requirement is not met the widget
does not load silently and incorrectly — the player receives a clear
message. Same declaration works for skins.

---

## USS — Style Authoring Format

OHUI adopts USS (Unity Style Sheet) as the foundation for its style
authoring layer, extended with a defined set of `-ohui-` prefixed
properties for game-specific needs.

USS is Unity UI Toolkit's styling format — plain text, CSS-derived
syntax, well-documented, supported by existing tooling (syntax
highlighting, validators). The format is adopted for authoring. It
is not coupled to Unity's runtime.

**Why USS rather than a bespoke format:** any skin author who knows
CSS or has worked with Unity UI Toolkit can contribute from day one.
The selector and cascade model is already correct. The community of
people who understand USS is large. The community who would understand
a bespoke OHUI format on day one is zero.

### What USS Covers (~80% of OHUI's style needs)

- Visual: colour, background, borders, border-radius, 9-slice image
  scaling, opacity, visibility
- Text: font size, weight, alignment, word wrap
- Layout: Flexbox via Yoga — stacking, wrapping, alignment,
  justification
- Dimensions: width, height, min/max, aspect-ratio, padding, margin
- Animation: transitions on transform properties, colour, timing
- States: `:hover`, `:focus`, `:active`, `:disabled` pseudo-classes
- Design tokens: CSS custom properties (`--token-name`) with `var()`
- Selectors: type, class, id, pseudo-class, descendant — full cascade

### The -ohui- Extension Properties

USS's standard property set does not cover everything game UI needs.
OHUI adds a defined set of extensions:

```css
/* Glow and edge effects */
-ohui-glow: 0 0 8px rgba(255, 0, 0, 0.6);
-ohui-edge-fade: 12px;

/* Local stacking within a widget */
-ohui-z-index: 10;

/* Sprite animation */
-ohui-sprite-sheet: url("sprites/stealth_eye.dds");
-ohui-sprite-columns: 8;
-ohui-sprite-rows: 2;
-ohui-sprite-index: 0;  /* animated via transition */

/* Radial layout — for hotkey wheels, radial menus */
-ohui-radial-angle: 45deg;
-ohui-radial-radius: 120px;
```

**Notably absent from extensions:** data binding. Properties like
`-ohui-bind-value` were considered but rejected as a layer violation.
Runtime data binding is the data binding schema's responsibility.
The style layer describes appearance. It does not drive runtime values.

### The -unity- Properties

USS's Unity-specific properties are either:
- Mapped to OHUI equivalents (`-unity-slice-*` → 9-slice, a
  first-class game UI concept)
- Dropped as Unity architecture artefacts with no game UI meaning

### Layout Engine — Yoga

OHUI uses **Yoga** for Flexbox layout — Meta's open source Flexbox
implementation, MIT licensed, used in production by React Native and
Unity UI Toolkit. MIT is fully compatible with OHUI's licensing.

Yoga computes layout geometry. It has no renderer of its own. OHUI's
rendering backend consumes the resulting positions. The constraint-
based layout model handles the vast majority of menu and component
layout requirements without requiring absolute positioning.

HUD element positioning is not handled by Yoga — it is handled by
layout profiles. Where a widget sits on screen is a player configuration
concern, not a style concern. Resolution presets ship default positions
for each supported aspect ratio. The style layer describes appearance.
It does not determine canvas position.

### Coverage Summary

| Surface | Coverage |
|---------|----------|
| Menu screens | ~80% |
| MCM config panels | ~75% |
| Component visual styling | ~85% |
| HUD element visual styling | ~85% |
| HUD element positioning | Layout profiles, not style |
| Animations and transitions | ~70% |
| Radial layouts | Dedicated component, -ohui- extensions |

---

## Rendering Paths

Three rendering paths exist. All are equal participants in the widget
system. The choice of path is invisible to OHUI's widget management —
all three produce a viewport that OHUI composites, manages in edit
mode, persists in layout profiles, and applies skins to.

### Path 1 — DSL-defined (primary)

Mod authors declare widgets using the DSL and USS. No C++ required.
No Scaleform knowledge required. This is the path for the vast
majority of widgets.

OHUI's DSL runtime reads the definition, evaluates bindings, runs
Yoga layout, and produces draw calls. Scaleform executes them. The
widget author never touches either layer.

### Path 2 — Native C++ (specialist)

For widgets that cannot be expressed declaratively — a minimap, a
3D item preview, anything requiring direct engine integration. These
register as native widgets, implement the viewport contract, and own
their own rendering into the OHUI-allocated surface.

They still participate fully in the widget runtime — edit mode,
layout profiles, skin chrome, input coordination. Only the pixels
inside the viewport are C++'s responsibility.

### Path 3 — Third-party viewport (open)

Any external rendering system that can paint into a surface can
participate as an OHUI widget by implementing the viewport contract.
OHUI takes no dependency on any third-party renderer. The door is
not closed.

**PrismaUI** (web renderer embedded via SKSE) is the most relevant
example. A PrismaUI widget that implements the viewport contract gets
OHUI's full feature set for free — edit mode, layout profiles, input
coordination, skin chrome — without PrismaUI needing to build any of
that. PrismaUI renders. OHUI provides the infrastructure. No
dependency taken by OHUI.

### The Renderer Abstraction Layer

Between the widget rendering layer and the Scaleform backend sits a
thin abstraction:

> Here is a viewport of these dimensions. Fill it.

The renderer's job is to contribute pixels into that rectangle.
Nothing more. Scaleform is the default implementation. Any other
rendering system implements the same interface and participates
identically.

This means if Scaleform is ever replaced, every USS file, every skin,
and every widget definition written by the community survives
unchanged. They were never talking to Scaleform. They were talking
to USS.

```
┌─────────────────────────────────────────────────┐
│       USS + DSL  (mod author surface)           │
├─────────────────────────────────────────────────┤
│       Renderer Abstraction Layer                │
│       "Here is a viewport. Fill it."            │
├──────────────────────┬──────────────────────────┤
│   Scaleform          │   Alternative renderer   │
│   Default.           │   Implements contract.   │
│   Invisible above.   │   Full feature set free. │
└──────────────────────┴──────────────────────────┘
```

---

## WPF/XAML as Reference Model

The WPF/XAML styling system is the reference model for how OHUI's
style layer behaves. Proven in production applications and in game
engine contexts (Noesis GUI). OHUI does not adopt XAML syntax but
adopts the model.

Three WPF/XAML concepts map cleanly onto OHUI's three-layer separation:

**ResourceDictionary → token store.** Named design values in one
place. Any element references them by key. Swapping the dictionary
updates everything automatically. In OHUI terms: the skin's token
block. Ship a new token set, the entire UI re-skins.

**Style → property values on a component type.** A button's default
foreground, background, padding, font. Not structure — values on
top of structure. Styles compose via inheritance. A skin extends
OHUI's default styles rather than replacing them — override only
what differs.

**ControlTemplate → structural definition.** What a component
actually consists of, independently of the values applied to it.
A skin that needs different colours never touches the ControlTemplate.
A skin that needs different structure provides a new one. The correct
escape hatch — available when needed, not required by default.

---

## Open Questions

1. **DSL tooling** — syntax highlighting, schema validation, and
   error reporting. Not an architectural blocker but important for
   mod author adoption. Needs design before public release.

2. **Compositor ownership** — does the compositor live in the Scaleform
   layer or in a C++ DirectX layer above it? Relevant to how cleanly
   third-party viewport rendering integrates.

3. **VR** — Scaleform and input differences in VR need to be understood
   before the rendering architecture is locked. VR is a day-one target.

4. **Skills / Perks constellation menu** — a 3D scene, not a flat menu.
   Whether this is a native C++ widget or requires a dedicated rendering
   approach needs investigation before scope is confirmed.
