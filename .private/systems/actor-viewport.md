# Actor Viewport

## Purpose

Several OHUI screens need a live render of a specific actor — an NPC
or the player character — as they currently appear in the game world.
Equipped, injured, visually modified by whatever mods are active. Not
a cached portrait. Not a generic silhouette. The actual actor as the
engine currently has them.

All screens that need actor rendering use the same system. The
requesting screen does not manage the camera, the lighting, or the
render target. It asks for an actor. It receives pixels.

---

## The Feature

OHUI renders actors in a controlled environment isolated from the
game world. Whatever cell the player is in, whatever weather or time
of day is active, none of it touches the actor render. The actor is
rendered against a neutral backdrop with consistent lighting.

The rendered actor is a clone — it inherits the source actor's
current appearance (equipment, body, face, all appearance mods) but
exists separately. The source actor remains in the game world,
completely unaffected.

This also resolves equipment preview cleanly: to preview an
unequipped item, OHUI applies it to the clone only. The actual
actor's equipment is unchanged. Moving off the item reverts the
clone.

---

## Framing Modes

The requesting screen specifies how the actor should be framed:

- **Head and shoulders** — mid-torso to above head, slight angle
- **Half body** — waist to above head
- **Full body** — full actor, slight offset from centre
- **Custom** — caller provides exact camera parameters

Default framing per use case:
- Dialogue NPC portrait: head and shoulders
- Inventory character preview: half body, player-rotatable
- Load game character card: head and shoulders, captured at save time

---

## Lighting

Default lighting is a balanced three-point rig (key, fill, rim).
Screens that want a different mood can request a named preset:

- **Neutral** — default, balanced
- **Warm** — warmer key, softer fill
- **Cool** — cooler key, blue-tinted fill
- **Dramatic** — high-contrast, strong key, minimal fill

Skin authors can define additional named presets.

---

## Update Rate

- **Static** — rendered once, not updated (captured portraits)
- **On change** — re-rendered when actor appearance changes
- **Continuous** — re-rendered at a configurable rate (default ~20fps)

Default per use case:
- Dialogue NPC portrait: on change
- Inventory preview during item hover: continuous
- Inventory preview at rest: on change
- Load game character card portrait: static (captured at save time)

### Preview Throttling

Equipment preview in the inventory does not trigger on every cursor
movement. A short dwell timer (150–200ms) starts when the cursor
lands on an equippable item. The clone's equipment only changes if
the cursor remains on that item past the threshold. Fast scrolling
through item lists never triggers preview loads. This prevents
excessive asset loading during rapid navigation.

---

## Absence Handling

If a meaningful render is not available, the viewport is absent. No
placeholder. No silhouette. No generic icon standing in for a face.
The requesting screen handles absence by collapsing the viewport area
gracefully. This is a skin responsibility.

Reasons a viewport may be unavailable: clone creation failed (actor
data in inconsistent state), or resource limits reached.

---

## Use Cases

### Dialogue — NPC Portrait

Requested on conversation start, released on conversation end. If
clone creation fails the portrait area collapses; dialogue continues
normally.

### Inventory — Character Preview

Requested on screen open. On-change at rest, upgraded to continuous
while the player dwells on an equippable item.

Player can orbit the camera with right stick or mouse drag. On hover
over an equippable item (past the dwell threshold), OHUI applies the
item to the clone only. The actual player equipment is unchanged.
Moving off the item reverts the clone. Confirming equip changes
actual equipment state separately.

### Load Game — Character Card Portrait

No live viewport at the main menu. Character card portraits are
captured renders taken at each save event: OHUI renders a static
head-and-shoulders viewport of the player character, captures the
result, compresses it, and writes it to the cosave. The load game
screen reads this stored image. No live rendering at the main menu.

---

## Resource Limits

- Maximum 2 simultaneous active viewports (configurable)
- Viewports idle for more than 30 seconds are suspended (rendering
  paused, resumed on next access)
- Viewports released automatically when the requesting screen closes
- Requests beyond the limit return nothing; screens show fewer
  portraits rather than failing

---

## What This Is Not

**Not a photo mode system.** Photo mode captures the live game world.
The actor viewport renders a clone in a controlled environment.

**Not a loading screen renderer.** Loading screen scenes are a
separate, independent problem.

**Not a 3D item preview.** Rendering standalone items without an
actor is a different concern — no skeleton, different lighting
assumptions. A natural extension but a separate feature.

---

## Open Questions

1. **Performance:** Two simultaneous viewports at the expected
   ceiling in normal use (inventory open during dialogue). Profiling
   required before resource limits are finalised.

2. **VR:** VR rendering has a fundamentally different pipeline. May
   work identically or may require a separate approach. Needs
   investigation.
