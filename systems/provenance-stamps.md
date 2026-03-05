# Provenance Stamp System

## The Problem

Skyrim has no memory of what the player made. An iron sword the player
forged at Smithing 42, carried through fifty hours of play, tempered
three times, and enchanted at Smithing 100 is indistinguishable from
an iron sword picked up off a bandit. The game does not know. The UI
does not know. The item does not know.

This matters more than it sounds. Players form attachments to items
they made. They remember crafting a particular weapon. They name things
in their head — "my sword," "the bow I made for Lydia." The game never
acknowledges any of this. Every crafted item is anonymous the moment
it leaves the forge.

Provenance stamps give items memory.

---

## What a Stamp Is

A provenance stamp is a persistent metadata record attached to an item
at the moment the player performs a crafting action on it. The stamp
travels with the item everywhere — through inventory transfers, gifting,
container storage, across saves. It is never lost unless the item is
destroyed.

A stamp records:

- **Action** — Crafted, Tempered, Enchanted, Cooked, or Brewed
- **Character name** — the player character who performed the action
- **Skill and level** — the relevant skill and level at time of action
- **Quality tier** — Iron, Steel, Fine, Superior, Exquisite, Flawless,
  Epic, Legendary (smithing) or equivalent tiers for other crafts
- **In-game date** — the in-game calendar date
- **Real-world timestamp** — when it actually happened
- **Dedication** — optional freeform text, player-entered

One action, one stamp. An item that has been crafted, tempered twice,
and enchanted carries four stamps. The full history is preserved.

---

## Stamp Accumulation

Items are not limited to a single stamp. Every qualifying action adds
a stamp to the item's history. The history is ordered chronologically
and collapsible in the detail panel — the most recent stamp is shown
by default, the full history available on demand.

**Example — Valdris's Iron Sword:**

```
[1] Crafted by Valdris · Smithing 42 · Fine
    Morndas 14th of Last Seed, 4E 201
    "My first real sword"

[2] Tempered by Valdris · Smithing 58 · Superior
    Turdas 3rd of Hearthfire, 4E 201

[3] Tempered by Valdris · Smithing 71 · Exquisite
    Middas 19th of Frostfall, 4E 201

[4] Enchanted by Valdris · Enchanting 82
    Sundas 7th of Evening Star, 4E 201
    "For when the dragons come"
```

This sword is not an iron sword. It is a record of a character's
growth. The stamps make that visible.

---

## Dedication

Any crafting action can include an optional dedication — a freeform
text field presented after the action is confirmed. The player types
whatever they want. Nothing is mandatory. Bulk crafters accept and
move on. Character builders take a moment to name the thing they made.

Dedications are surfaced prominently in specific contexts:

**Gift staging panel** — when giving a dedicated item to an NPC, the
dedication is shown before the gift is confirmed.

**Follower container screen** — dedicated items in a follower's
inventory show their dedication in the detail panel.

**Detail panel** — the most recent dedication is shown prominently.
The full dedication history is available in the collapsed stamp
history.

**New Game+ legacy summary** — dedications appear in the generated
legacy narrative.

---

## Sentimental Protection

Any stamped item can be flagged as sentimental — an opt-in per-item
designation the player sets from the context menu.

Sentimental items are excluded from bulk operations in OHUI's screens:
quick-sell, junk tagging and bulk sell, Take All in containers. The
protection is against accidental loss in a thoughtless bulk action,
not against deliberate decisions. A player who explicitly chooses to
sell or drop a sentimental item from the context menu can do so.

The sentimental flag is visible as a distinct badge in the inventory
list. The player always knows which items are protected.

**Scope:** Sentimental protection applies within OHUI's own UI
screens. Operations that bypass OHUI (engine-level item transfers,
Papyrus scripts, other mods) are not intercepted.

---

## Data Integrity

**Stamps travel with items.** Stamp data is attached to the item
itself, not maintained in a separate lookup table. Wherever the item
goes — sold, gifted, placed in a container, transferred to a
follower, saved and loaded — its stamps go with it. If a stamped
item is sold and bought back, the stamps are intact. If a stamped
item is placed in a chest and retrieved later, the stamps are intact.

This is a hard design requirement. If stamp data cannot be attached
to items in a way that survives all normal item operations, the
feature does not ship. A parallel registry that must be kept in sync
with the engine's item state is not acceptable — desynchronisation
between the registry and reality is inevitable and unrecoverable.

**Stacked items.** Some items exist as undifferentiated stack members
(potions, arrows, food). Stamped items within stacks need to be
individually identifiable. This is a feasibility question for
implementation — if per-instance identity within stacks cannot be
reliably achieved, stamped items must be unstacked.

---

## Legacy Items

An item that accumulates multiple stamps across a long playthrough
becomes something more than its base stats. A weapon crafted at the
start of a playthrough, carried through hundreds of hours, improved
repeatedly, enchanted, dedicated, and marked sentimental is a
character artifact.

OHUI does not declare any item a legacy item by fiat. The designation
emerges from stamps accumulating on items the player has cared about.
OHUI surfaces the record. The player's choices created it.

Legacy items are eligible for New Game+ heirloom selection. Only
stamped items can be heirlooms — unstamped items have no provenance.
The heirloom selection screen shows stamp history to help the player
choose.

---

## Mod API

The full stamp history is readable by other mods. Any mod with a
reason to know whether an item was player-crafted, who made it, or
when can query the stamp system.

Available queries: whether an item is stamped, whether it is
sentimental, its dedication text, stamp count, and per-stamp details
(character name, skill level, action type, date).

**Use cases across the ecosystem:**

- Display mods — surface provenance on item plaques or mannequins
- Gift systems — treat player-crafted items differently in NPC
  reaction systems
- Museum mods — flag player-crafted donations as distinct from found
  items
- Reputation systems — treat crafted gifts as more meaningful
- Merchant mods — query skill level at craft time
- Follower mods — reference dedications in dialogue

The API is read-only for external mods. Stamps can only be written
by OHUI at crafting trigger points. No mod can inject false stamps
or modify existing ones.

---

## Surface Integration Points

Every OHUI surface that touches items is aware of the stamp system:

**Inventory FacetedList** — "Player-crafted" facet filters to stamped
items. "Dedicated" and "Sentimental" facets available as presets.

**Item Detail Panel** — stamp history in a collapsible section. Most
recent stamp visible by default. Full history on expand. Dedication
shown prominently if present.

**Barter Screen** — sentimental items excluded from the offer panel
by default, with confirmation prompt if explicitly added.

**Container Screen** — Take All skips sentimental items. Dedicated
items in follower inventories show their dedication.

**Gift Menu** — dedication shown in staging panel before gift is
confirmed.

**Crafting Screens** — dedication field presented after action
confirmation. The player types, accepts, or skips.

**Stats Screen** — crafting stats derived from stamp history. Not
just totals but breakdowns: potions brewed at high skill levels,
longest-carried item, most-improved item, first item crafted.

**Level Up Screen** — stamp history contributes to the skill progress
summary.

**New Game+** — heirloom selection shows stamp history. Legacy summary
generated from stamp data.

**Load Game Character Card** — notable stamped items surfaced as
character highlights.

---

## Trigger Points

Stamps are written at the moment the crafting action is confirmed —
after the engine has completed the action, before control returns to
the player:

- **Smithing — Craft:** when the item appears in inventory
- **Smithing — Temper:** when the temper is applied
- **Enchanting:** when the enchantment is applied
- **Alchemy:** when potions appear in inventory (batch stamped —
  all potions from one session carry the same stamp data)
- **Cooking:** same pattern as alchemy
