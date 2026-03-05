# Loading Screen

## The Problem

Skyrim's loading screen is a rotating 3D model on a black background with
a tip text line and a loading bar. Thirteen years of staring at a slowly
spinning weapon or creature while waiting for the next cell to load.

Loading screen replacer mods have tried to improve this for years. Every
single one of them hits the same wall: the engine's loading screen system
is built around 3D geometry. A mod author who wants to display a flat 2D
image — concept art, a screenshot, a lore illustration — has to create a
flat plane NIF, texture it with the image, and present a technically-3D
scene that happens to look exactly like a 2D image. It works. It is deeply
silly. It is the only option available because the engine provides no
native 2D image pathway for the loading screen.

OHUI owns the loading screen. It fixes this properly and then goes
considerably further.

---

## Three Tiers

### Tier 1 — Vanilla

OHUI runs the vanilla loading screen faithfully. The rotating model, the
loading bar, the tip text. Players who want vanilla get vanilla. No
configuration required. This is the default for players who have not
installed any loading screen content.

### Tier 2 — Native 2D Images

OHUI provides a native 2D image pathway for the loading screen. Mod
authors register flat images — JPG, PNG, DDS — directly. OHUI displays
them as a proper 2D surface. No NIF. No plane mesh. No UV mapping a
texture onto geometry to simulate something the engine should have
supported from the start.

The registration is simple:
- Image path
- Optional caption or lore note (displayed as overlay text)
- Optional conditions (show only in Solstheim cells, show only at night,
  show only when player is in a specific faction, etc.)

OHUI handles display, scaling, aspect ratio correction, loading bar
overlay, and tip text positioning. The mod author provides an image.
OHUI does the rest.

This tier exists to give the loading screen mod ecosystem a clean
migration path away from the NIF workaround. Authors who have been
maintaining plane mesh NIFs for years can ship a flat image instead.
The result is identical. The authoring overhead is a fraction.

### Tier 3 — Live Animated Scenes

The headline tier. A real-time animated 3D scene rendered in a native
C++ viewport — the same mechanism used for NPC portraits in the dialogue
screen, live location preview in the map screen, and character preview
in the inventory screen — playing while the game loads in the background.

Not a pre-rendered video. Not a looping NIF animation baked to a texture.
An actual live scene with geometry, dynamic lighting, weather effects,
and full skeletal animation running in real time. The same engine that
renders the game world, rendering a curated scene in a dedicated viewport,
while the next cell loads.

The scene is authored specifically for the loading screen. Not repurposed
from the game world. Designed to look extraordinary in a fixed frame with
a fixed camera, for a duration of anywhere from five seconds to two
minutes depending on the load.

**The camera is not static.** A slow automated pan, a subtle parallax
drift, a gradual push toward a focal point. The scene is alive for the
entire duration of the load. It does not loop awkwardly at a fixed point
— the camera movement is designed to sustain indefinitely without feeling
repetitive.

---

## Scene Design

First-party scenes shipping with OHUI establish the quality baseline.
Each one is designed around a specific dramatic or atmospheric moment
in Skyrim's world:

A Dark Brotherhood assassin performing a stealth killmove — the blade
catching the torchlight at the moment of contact, the victim's body
beginning to fall, the assassin already withdrawing into shadow. The
whole scene lit in deep red and black. Held on a slow push toward the
moment of contact for as long as the load takes.

The aurora borealis over Winterhold — the college towers in silhouette,
the lights shifting and breathing overhead, snow drifting across the
frame. A scene that could run for ten minutes without wearing out its
welcome.

A forge in full operation — sparks showering from a blade being worked,
the fire roaring, the glow catching the smith's armour. Heat and craft
and industry. A slow orbit around the forge.

Waves on the Sea of Ghosts at dusk — ice floes moving, spray catching
the last light, the horizon stretching. A scene designed for the long
loads between far-separated cells.

Each first-party scene is assigned conditions — cell type, location,
time of day, player faction — so the right scene appears in the right
context where possible. A load into a Dark Brotherhood sanctuary has
a higher weight toward the assassin scene. A load into Winterhold has
a higher weight toward the aurora scene. Conditions are probabilistic,
not absolute — variety is preserved.

---

## Mod Author Registration

Any mod can register loading screen scenes at any tier. The registration
API is consistent with the rest of OHUI's mod author contracts:

**2D image registration:**
```
OHUI_LoadingScreen.RegisterImage(
    imagePath:   "Data/Textures/MyMod/loading_solstheim.dds",
    caption:     "The ash plains of Solstheim",
    conditions:  ["cell:solstheim"]
)
```

**3D scene registration:**
```
OHUI_LoadingScreen.RegisterScene(
    scenePath:   "Data/MyMod/LoadingScenes/ash_storm.nif",
    cameraAnim:  "Data/MyMod/LoadingScenes/ash_storm_cam.hkx",
    caption:     "Ash storms roll in without warning",
    conditions:  ["cell:solstheim", "weather:ashstorm"]
)
```

A mod set in Solstheim ships Solstheim loading scenes. A city mod ships
that city's atmosphere as a loading scene — narrow streets at night, the
market at dawn. A Dark Brotherhood overhaul ships Brotherhood scenes.
A Dwemer ruin mod ships the interior of a great Dwemer machine in operation.

The conditions system means the loading screen is aware of context. Where
the player is going, what time it is, what faction they belong to, what
weather is active — all of these can influence which scene is selected.

---

## Shared Elements

Across all three tiers, OHUI owns the full loading screen surface:

**Loading bar** — positioned and styled consistently with the active skin.
Not baked into the background. An independent element that sits above
whatever the background tier is displaying.

**Tip text** — the existing Skyrim tip system is preserved and extended.
Tip text panels are skinnable. The tip text position is configurable.
Mods can register their own tip text entries via the existing Papyrus
tip registration system — OHUI respects whatever tips are registered.

**Caption text** — for 2D and 3D scenes, an optional caption line
positioned below or above the loading bar. Lore text, location names,
atmospheric flavour. The caption is registered by the scene author,
not generated by OHUI.

**Transition** — the cut to and from the loading screen is a skinnable
transition rather than a hard cut. A brief fade, a directional wipe,
a darkening vignette. The transition duration and style are skin-defined.
The default is a clean fade — present and intentional rather than an
abrupt interrupt.

---

## The Wow Moment

The first time a player with a long load sees a Dark Brotherhood assassin
stepping silently out of shadow, blade raised, the scene holding on a
slow push toward the kill while their next cell finishes loading — the
loading screen stops being dead time.

It is not a loading screen anymore. It is a moment. The game is still
telling a story while it prepares the next one.

That reframing is the design ambition for this screen. The load cannot
be eliminated. The experience of the load can be transformed entirely.
