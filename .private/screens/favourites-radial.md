# Favourites Radial

## The Problem with Vanilla Favourites

Vanilla favourites is a vertical list accessed via a keypress that pauses
the game. It works. It is also completely at odds with how favourites are
actually used — in the middle of combat, under pressure, with a controller
in hand, needing to act fast. A vertical list you scroll through is the
wrong shape for that moment.

The radial is the right shape. It is spatial — segments have positions the
player learns and reaches for by muscle memory. It is fast — one stick flick
to a segment, one confirm. It scales to controller naturally. It is the
established pattern for quick-access under pressure and every game that has
solved this problem has converged on it.

OHUI replaces the vanilla favourites menu with a configurable radial.

---

## Structure

The radial has eight segments arranged in a circle. Each segment is
independently configured by the player. Empty segments are inactive —
not visible on the radial during play. Filled segments redistribute
evenly around the circle regardless of how many are active.

A radial with three active segments spaces them 120° apart. A radial
with six active segments spaces them 60° apart. The player never
navigates past empty gaps.

**Activation:** Dedicated keybind (default: H on keyboard, left d-pad
hold on controller). The game slows to a configurable time scale on
radial open — default 10% speed, configurable from full pause to full
speed. The radial closes on segment selection, on cancel input, or on
the activation keybind released (hold-to-use mode, configurable).

**Selection:** On controller — hold activation, flick right stick to
segment, release to confirm. On mouse — hold activation, move cursor
to segment, release to confirm. On keyboard — hold activation, number
keys 1-8 select segments by position.

**Time scale:** The default 10% speed is enough to act without stopping
the world entirely. Players who want full pause set it. Players who want
the radial accessible in real time set it to 100%. The choice is theirs.

---

## Slot Types

Each segment holds one slot of one of the following types. The slot type
is chosen when the segment is configured. Different segment types behave
differently when selected.

### Outfit Slot

Triggers an outfit switch. Bound to a named outfit from the outfit system.
Selecting this segment equips the bound outfit immediately — same
instantaneous swap as manual outfit selection, no scripts, no delay.

The segment displays the outfit name and a thumbnail of the character
wearing it. On radial open, the currently active outfit's segment is
highlighted.

**The favourites flag:** Outfits appear in the radial configuration
screen only if they have been flagged "Show in Favourites Radial" in
the outfit definition. This flag is off by default. The player explicitly
promotes outfits to radial-eligible. Outfits without the flag — home
outfits, sleeping clothes, social outfits the player does not want
accessible mid-combat — are invisible to the radial entirely.

This makes accidental mid-combat outfit switching architecturally
impossible rather than just unlikely. The radial cannot show what
the player has not promoted to it.

### Potion Category Slot

Consumes a potion from a named category rather than a specific item.
"Drink a healing potion" — OHUI selects the most appropriate potion
from the player's inventory according to configurable logic.

**Selection logic options (per slot, configurable):**
- Weakest sufficient — the weakest potion that covers the current
  deficit. Preserves strong potions for when they are needed.
- Strongest available — always drink the best potion in inventory.
- Most abundant — drink whichever type you have the most of.
- Custom threshold — drink only if below X% of maximum value.

The segment displays the category name and a count of available
potions in that category. "Healing (12)" — the player knows at a
glance whether they are stocked before opening the radial.

Count turns amber when below a configurable low-stock threshold.
Count turns red when one remains. Segment dims when empty.

**Built-in categories:** Healing, Stamina, Magicka, Resist [element],
Fortify [skill], Cure [effect]. Mod-added potion categories register
via the same API as everything else.

### Spell Slot

Equips a specific spell to the player's preferred hand (configurable
per slot — left, right, both). Same as vanilla favourites spell
selection, faster to reach.

### Shout Slot

Sets the active shout. Does not activate it — that remains the
dedicated shout key. The slot just makes the shout ready.

### Item Slot

A specific item for edge cases where a specific item is genuinely
needed — a quest item, a unique consumable, a torch. The slot dims
when the item is not in inventory.

---

## Configuration Screen

The radial configuration screen is accessible from the tween menu and
from the favourites radial itself (long-press on an empty segment or
via a dedicated edit button visible on radial open).

**Layout:** Eight segment slots arranged in a circle matching the
in-play radial layout. Each slot shows its current contents or an
empty state prompt. The player selects a slot to configure it.

**Configuring a slot:** Selecting an empty slot opens a slot type
picker — Outfit / Potion Category / Spell / Shout / Item. Selecting
a type opens the relevant picker:

- Outfit picker: shows only outfits flagged as favourites-eligible.
  Thumbnail preview, outfit name, current label.
- Potion category picker: built-in categories plus mod-registered
  categories. Selection logic configured after category selection.
- Spell picker: the player's full known spell list in a FacetedList.
- Shout picker: known shouts with word count and cooldown info.
- Item picker: player inventory in a FacetedList.

**Clearing a slot:** Select a filled slot, choose Clear. The segment
becomes inactive and the remaining segments redistribute.

**Reordering:** Slots can be dragged to different positions within
the eight-slot ring. The player arranges their muscle memory layout.

---

## Multiple Radials

The player can define multiple named radials and switch between them.
A combat radial, a utility radial, a magic radial. The active radial
is selected from the tween menu or via a modifier held during radial
activation (hold activation + up d-pad cycles radials).

Multiple radials do not complicate the common case. A player who only
needs one radial never configures a second one. The switcher is absent
from the radial UI until a second radial is defined.

---

## Relationship to Other Systems

**Outfit system:** The radial is the manual trigger surface for outfit
switching. The label-based trigger mod pattern is the automatic trigger
surface. Both use the same outfit definitions. Both execute the same
instantaneous swap. The favourites flag is the gate between the outfit
system and the radial.

**Potion categories:** The potion category slot is the first-party
answer to quick-potion mods. Players who have been using dedicated
hotkey mods for potion drinking get the same functionality built into
the radial with better information (live count, low-stock warning)
and better integration (the radial knows your full inventory state
via SCIE if present).

**iEquip / similar equipment managers:** iEquip is on the frozen giants
list. Its core use case — quick equipment switching from a radial-
adjacent widget — is addressed by the outfit slot and the spell/shout
slots. OHUI's radial is not iEquip. It is what iEquip's users actually
needed, built on the outfit system rather than on a parallel equipment
state manager.

**HUD widget:** The active radial state is surfaced as an optional HUD
widget — a small arc or ring showing the active radial's segments and
their current state (active outfit highlighted, potion counts, shout
ready state). Configurable visibility. The player who wants to see their
radial state at a glance without opening it can. The player who wants
a clean HUD hides the widget.

---

## Design Principle

The radial is a fast-access surface for things the player has
deliberately promoted to it. Nothing appears by default. Nothing
appears automatically. The player builds their radial from their
outfits, their potions, their spells. The radial reflects their
playstyle because they defined it.

The outfit favourites flag is the clearest expression of this principle.
The system does not guess which outfits are combat-appropriate. The
player decides. The nighties stay home because the player said so,
not because OHUI made a judgement about sleepwear.
