# Data Binding Schema

## Purpose

The data binding schema is the architectural layer that bridges live
game state to OHUI's game-agnostic widget DSL. It is the only place
in the architecture where game-specific concepts exist. The DSL, the
widget definitions, and the skin files are all unaware of Skyrim. The
schema is the translation layer between them.

---

## The Binding Contract

A binding is a named, typed value that OHUI polls from game state and
makes available to widgets via the data binding engine.

Every binding has:

- **ID** — the binding key widgets reference (e.g. `player.health.current`)
- **Type** — Float, Int, Bool, String
- **Poll rate** — how often OHUI checks for changes
- **Description** — human-readable description for tooling

Widgets reference bindings by ID in their definitions. The DSL does
not know what `player.health.current` represents in the game world.
It knows it is a Float. The schema knows the rest.

---

## Naming Convention

Binding IDs follow a hierarchical dot-notation:

```
{subject}.{category}.{property}[.{qualifier}]
```

Examples: `player.health.current`, `enemy.name`, `time.hour`,
`world.weather.is_raining`.

---

## Built-in Bindings

### Player Vitals

| Binding ID | Type | Description |
|------------|------|-------------|
| player.health.current | Float | Current health |
| player.health.maximum | Float | Maximum health |
| player.health.percent | Float | Current / maximum |
| player.health.regen_rate | Float | Health regeneration rate |
| player.magicka.current | Float | Current magicka |
| player.magicka.maximum | Float | Maximum magicka |
| player.magicka.percent | Float | Current / maximum |
| player.magicka.regen_rate | Float | Magicka regeneration rate |
| player.stamina.current | Float | Current stamina |
| player.stamina.maximum | Float | Maximum stamina |
| player.stamina.percent | Float | Current / maximum |
| player.stamina.regen_rate | Float | Stamina regeneration rate |
| player.breath.current | Float | Current breath |
| player.breath.maximum | Float | Maximum breath |
| player.breath.percent | Float | Current / maximum |

### Player Stats

| Binding ID | Type | Description |
|------------|------|-------------|
| player.level | Int | Character level |
| player.xp.current | Float | Current XP |
| player.xp.to_next_level | Float | XP needed for next level |
| player.xp.percent | Float | Progress to next level |
| player.gold | Int | Gold amount |
| player.name | String | Character name |
| player.race | String | Character race |
| player.carry_weight.current | Float | Current carry weight |
| player.carry_weight.maximum | Float | Maximum carry weight |
| player.carry_weight.percent | Float | Current / maximum |
| player.carry_weight.is_encumbered | Bool | Over carry limit |
| player.speed | Float | Movement speed multiplier |

### Player Skills

All 18 skills follow the pattern `player.{skill_name}.skill` as
Float values: one_handed, two_handed, archery, block, smithing,
heavy_armor, light_armor, pickpocket, lockpicking, sneak, alchemy,
speech, alteration, conjuration, destruction, illusion, restoration,
enchanting.

### Player State

| Binding ID | Type | Description |
|------------|------|-------------|
| player.is_in_combat | Bool | Currently in combat |
| player.is_sneaking | Bool | Currently sneaking |
| player.is_swimming | Bool | Currently swimming |
| player.sneak.detected | Bool | Detected by any NPC |
| player.sneak.detection_level | Float | Detection meter level |
| player.shout.active_name | String | Equipped shout name |
| player.shout.cooldown_remaining | Float | Shout cooldown remaining |
| player.shout.cooldown_total | Float | Shout cooldown total |
| player.shout.cooldown_percent | Float | Cooldown progress |

### Player Location

| Binding ID | Type | Description |
|------------|------|-------------|
| player.location.cell_name | String | Current cell/location name |
| player.location.hold_name | String | Current hold |

### Equipped Items

| Binding ID | Type | Description |
|------------|------|-------------|
| player.equipped.right_hand.name | String | Right hand item/spell |
| player.equipped.right_hand.damage | Float | Right hand weapon damage |
| player.equipped.right_hand.charge | Float | Right hand enchantment charge |
| player.equipped.right_hand.charge_max | Float | Maximum charge |
| player.equipped.left_hand.name | String | Left hand item/spell |
| player.equipped.left_hand.magicka_cost | Float | Left hand spell cost |
| player.equipped.ammo.name | String | Equipped ammunition |
| player.equipped.ammo.count | Int | Ammunition count |

### Enemy Target

| Binding ID | Type | Description |
|------------|------|-------------|
| enemy.health.current | Float | Target current health |
| enemy.health.maximum | Float | Target maximum health |
| enemy.name | String | Target display name |
| enemy.level | Int | Target level |
| enemy.is_boss | Bool | Target is a boss-type enemy |

### Time and World

| Binding ID | Type | Description |
|------------|------|-------------|
| time.hour | Float | Game time, 0.0–24.0 |
| time.day | Int | Day of month |
| time.month | Int | Month |
| time.year | Int | Year |
| time.day_of_week | Int | Day of week |
| time.is_day | Bool | Between 6:00 and 20:00 |
| time.is_night | Bool | Not daytime |
| world.weather.current_id | Int | Current weather form |
| world.weather.is_raining | Bool | Rain active |
| world.weather.is_snowing | Bool | Snow active |

---

## Poll Rates

Every binding has a declared poll rate. The data binding engine uses
this to schedule value reads efficiently.

- **Per-frame** — polled every frame. Reserved for values that change
  continuously and must be visually smooth.
- **Throttled** — polled at a fixed interval, not every frame.
- **On event** — polled only when a specific game event fires.

Most bindings are throttled. Per-frame is reserved for values that
genuinely require frame-level granularity — vitals draining during
combat, breath depleting underwater, shout cooldown ticking down.

| Poll Rate | Bindings |
|-----------|----------|
| Per-frame | health/magicka/stamina/breath current, shout cooldown |
| Throttled (fast) | carry weight, equipped items, detection level |
| Throttled (slow) | location, XP, time, weather, enemy target |
| On event | level, gold, skills |

The widget declares how often it wants to re-render. The binding
declares how often the value is polled. The engine coordinates both
— no unnecessary polls, no unnecessary renders.

---

## Custom Bindings

Mods can register custom bindings via the mod registration API. A
custom binding provides a polling function and declares its type and
poll rate. OHUI routes its value to any widget that binds to its ID.

Custom bindings follow the same naming convention but are namespaced
to the mod ID to prevent collisions. A widget that binds to a custom
key receives a default value if the mod is not installed — the widget
must handle the absent case gracefully.

Widgets that depend on custom bindings should declare them in their
manifest so OHUI can warn if the binding is not present at activation.

---

## Schema Versioning

The schema is versioned independently of the OHUI release. Binding
IDs and types are stable. A binding added in version 1.2 is available
from that version forward.

Removed bindings are deprecated for one major version before removal.
Widgets binding to a deprecated key receive a log warning.

---

## Portability

The schema layer is the entire scope of what changes when OHUI is
ported to a different game. The Fallout 4 schema replaces
`player.health` with `player.hp` and adds `player.ap` for action
points. The Starfield schema adds oxygen and boost pack bindings.
The widget definitions, the DSL, and the skins are unchanged.

This is the layer separation principle made concrete. Every
game-specific concept in the entire architecture lives in this
document and its corresponding implementation. Nothing else knows
about Skyrim.

---

## Open Questions

1. **Enemy targeting:** The current schema assumes a single primary
   target for `enemy.*` bindings. Multi-target enemy health bars
   require a list-typed binding, which is not yet specified.

2. **Active effects list:** The active effects HUD widget needs a
   list of current magic effects. This is a list binding and is
   deferred pending list binding design.

3. **Compass markers:** The compass widget needs a list of map marker
   positions relative to player heading. Same list binding problem.

4. **Schema tooling:** Widget authors need to browse available
   bindings and see their types. A generated reference from the
   schema definition is the right solution.
