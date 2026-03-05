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

```
action:        Crafted | Tempered | Enchanted | Cooked | Brewed
characterName: string         — the player character's name
skillName:     string         — the relevant skill (Smithing, Enchanting, etc.)
skillLevel:    uint8          — skill level at time of action
qualityTier:   string         — Iron | Steel | Fine | Superior | Exquisite
                                | Flawless | Epic | Legendary (smithing)
                                | equivalent tiers for other crafts
inGameDate:    string         — in-game calendar date (Morndas 14th of
                                Last Seed, 4E 201)
realTimestamp: uint64         — real-world unix timestamp
dedication:    string         — optional freeform text, player-entered
```

One action, one stamp. An item that has been crafted, tempered twice,
and enchanted carries four stamps. The full history is preserved.

---

## Stamp Accumulation

Items are not limited to a single stamp. Every qualifying action on
an item adds a stamp to its history. The history is ordered
chronologically and collapsible in the detail panel — the most recent
stamp is shown by default, the full history available on demand.

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
dedication is shown in the staging panel before the gift is confirmed.
"Crafted by Valdris for Lydia" is visible at the moment of giving.
The act of the gift is confirmed before it is committed.

**Follower container screen** — dedicated items in a follower's
inventory show their dedication in the detail panel. A sword given
to Lydia remembers that it was given to Lydia.

**Detail panel** — the most recent dedication is shown prominently.
The full dedication history across all stamps is available in the
collapsed history.

**New Game+ legacy summary** — dedications appear in the generated
legacy narrative. "Crafted by Valdris for Lydia · carried 201 days"
is the kind of line the summary produces for a dedicated heirloom.

---

## Sentimental Protection

Any stamped item can be flagged as sentimental — an opt-in per-item
designation the player sets from the context menu. The flag is stored
in the stamp data.

Sentimental items are excluded from:
- Quick-sell operations
- Junk tagging and bulk sell
- Take All in containers
- Any bulk operation that moves or removes items

The protection is against accident, not against deliberate decision.
A player who opens the item's context menu and explicitly chooses to
sell or drop a sentimental item can do so. The flag prevents the item
from being lost in a thoughtless bulk action. It does not prevent
intentional choices.

The sentimental flag is visible as an indicator in the inventory list
— a small distinct badge alongside the other status indicators. The
player always knows which items are protected.

---

## Legacy Items

An item that accumulates multiple stamps across a long playthrough
becomes something more than its base stats. A weapon crafted at the
start of a playthrough, carried through hundreds of hours, improved
repeatedly, enchanted, dedicated, and marked sentimental is a character
artifact in all but name. The stamps make this visible.

OHUI does not declare any item a legacy item by fiat. The designation
emerges from the stamps accumulating on items the player has cared
about. An item with four or more stamps, a dedication, and a sentimental
flag has earned that status through the player's own actions. OHUI
surfaces the record. The player's choices created it.

Legacy items are eligible for New Game+ heirloom selection. Only
stamped items can be heirlooms — unstamped items have no provenance
and no reason to carry forward. The heirloom selection screen shows
the stamp history of eligible items to help the player choose which
ones to bring into the next playthrough.

---

## Technical Implementation

### Storage

Stamp data is stored in the OHUI cosave per item. Items are identified
by their form ID plus a unique instance identifier generated at first
stamp — vanilla Skyrim's item reference system allows individual item
instances to be distinguished from stack members.

```
[StampedItem]
  instanceId:   uint64        — OHUI-generated unique instance ID
  formId:       uint32        — item base form ID
  pluginName:   string        — for cross-plugin stability
  sentimental:  bool
  stampCount:   uint32
  [Stamps]
    action:       uint8
    charName:     string
    skillName:    string
    skillLevel:   uint8
    qualityTier:  uint8
    inGameDate:   string
    realTime:     uint64
    dedication:   string
```

### Trigger Points

Stamps are written at the moment the crafting action is confirmed —
after the engine has completed the craft, before control returns to
the player. The trigger points are:

- **Smithing — Craft:** immediately after the item appears in
  inventory from the forge
- **Smithing — Temper:** immediately after the temper is applied
  and the item's stats update
- **Enchanting:** immediately after the enchantment is applied
  and the item is named
- **Alchemy:** immediately when the potion appears in inventory.
  Potions are stamped as a batch — a brewing session of twelve
  potions produces twelve individually stamped potions, all with
  the same stamp data. Stamped potions surface in the "player-brewed"
  filter but are not individually named unless the player explicitly
  names the batch
- **Cooking:** same pattern as alchemy

### Item Tracking Across Transfers

When a stamped item moves — sold, gifted, placed in a container,
transferred to a follower — its stamps move with it. OHUI maintains
a persistent item registry keyed by instance ID. Wherever the item
is, its stamp history is recoverable from the registry.

If a stamped item is sold to a merchant and later bought back, the
stamps are intact. If a stamped item is found in a container (placed
there by the player), the stamps are intact. The stamp history is
not tied to the item's location or owner. It is tied to the item.

### Items the Engine Cannot Distinguish

Some items exist as undifferentiated stack members — potions, arrows,
food. The engine does not natively distinguish one health potion from
another health potion in a stack of ten. OHUI generates instance IDs
for stamped items at the moment of stamping, which allows individual
instances to be tracked even within stacks.

This works for newly crafted items. It does not retroactively stamp
existing items — items already in inventory before OHUI is installed
are not stamped. The stamp system applies from the moment of crafting
forward.

---

## API Surface

The full stamp history is readable via both C++ and Papyrus. Any mod
with a reason to know whether an item was player-crafted, who made it,
or when it was made can query the stamp system.

```cpp
// C++ API
auto stamps = OHUI::Stamps::GetHistory(itemInstanceId);
bool isCrafted = OHUI::Stamps::IsCrafted(itemInstanceId);
bool isSentimental = OHUI::Stamps::IsSentimental(itemInstanceId);
std::string dedication = OHUI::Stamps::GetDedication(itemInstanceId);
```

```papyrus
; Papyrus API
Bool   OHUI_Stamps.IsCrafted(Form item)
Bool   OHUI_Stamps.IsSentimental(Form item)
String OHUI_Stamps.GetDedication(Form item)
Int    OHUI_Stamps.GetStampCount(Form item)
String OHUI_Stamps.GetCharacterName(Form item, Int stampIndex)
Int    OHUI_Stamps.GetSkillLevel(Form item, Int stampIndex)
```

**Use cases across the ecosystem:**

- **Display mods** — surface provenance on item display plaques or
  mannequins. "Forged by Valdris, Smithing 94."
- **Gift systems** — treat player-crafted items as categorically
  different from found items in NPC reaction systems
- **Museum mods** — LOTD or similar can flag player-crafted donations
  as distinct from found items in display records
- **Reputation systems** — a mod that tracks character reputation can
  treat a player-crafted gift as more meaningful than a purchased one
- **Merchant mods** — merchants with opinions about craftsmanship can
  query skill level at craft time and respond accordingly
- **Follower mods** — followers who received dedicated items can
  reference the dedication in dialogue

The API is read-only for external mods. Stamps can only be written
by OHUI at crafting trigger points. No mod can inject false stamps
or modify existing ones.

---

## Surface Integration Points

Every OHUI surface that touches items is aware of the stamp system:

**Inventory FacetedList** — "Player-crafted" facet filters to stamped
items. "Dedicated" facet filters to items with dedications. "Sentimental"
facet is a preset that shows protected items.

**Item Detail Panel** — stamp history shown in a collapsible section.
Most recent stamp visible by default. Full history on expand. Dedication
shown prominently if present.

**Barter Screen** — sentimental items excluded from the offer panel
by default. A player who puts a sentimental item in the offer panel
gets a confirmation prompt.

**Container Screen** — Take All skips sentimental items silently.
Dedicated items in follower inventories show their dedication in the
detail panel.

**Gift Menu** — dedication shown in staging panel before gift is
confirmed. The giving moment is marked.

**Crafting Screens** — dedication field presented after action
confirmation. Pre-populated with empty string. The player types,
accepts, or skips. No friction for players who don't want it.

**Stats Screen** — crafting stats derived from stamp history. Not
just "312 potions brewed" but "47 potions brewed at Alchemy 80+",
"longest-carried item", "most-improved item", "first item crafted."

**Level Up Screen** — stamp history contributes to the skill progress
summary. A level gained substantially through smithing can note how
many items were crafted this level.

**New Game+** — heirloom selection shows stamp history. Legacy summary
generated from stamp data. The character's crafting life is readable
in full at the moment of carrying it forward.

**Load Game Character Card** — notable stamped items surfaced as
character highlights. "Carried Valdris's Iron Sword for 184 days."
The character's relationship to their crafted items visible before
the save is even loaded.
