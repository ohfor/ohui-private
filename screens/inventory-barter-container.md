# Inventory, Barter, and Container Screens

## Shared Foundation

Inventory, Barter, and Container are three instances of the same structural pattern:
a category tab bar, a filterable and sortable item list, an item detail panel, and
action buttons. The data differs. The available actions differ. The tabs differ.
The underlying component is identical.

SkyUI implements this pattern from scratch in each menu's own file. A bug fix in
inventory doesn't propagate to barter. A skin that changes inventory layout must
be applied separately to barter and container. A mod that extends categorisation
must patch each menu individually.

OHUI defines this pattern once as a first-class component — `TabbedListPanel` —
and all three screens are instances of it, configured with their specific data and
behaviour. A bug fix propagates everywhere. A skin author skins the component once
and all three screens follow. A mod that extends categorisation plugs into the
component rather than patching individual menus.

The three screens are documented together because their design decisions are largely
shared. Where they diverge, the divergence is noted explicitly.

---

## The Problem

The inventory screen is the most-used menu surface in Skyrim. Players spend more
time in it than any other screen. Its failures are therefore felt more often than
any other screen's failures:

- **No item comparison.** Hovering over a weapon shows its stats. It does not show
  how those stats compare to what is currently equipped. Every modern RPG has item
  comparison. Skyrim has never had it natively. Players guess, equip to check,
  unequip, repeat.
- **Sorting is primitive.** Name, value, weight. No sorting by damage, armour
  rating, enchantment, or any stat that actually matters for item decisions.
- **Filtering is primitive.** The tab bar separates broad categories. Finding a
  specific item in a full inventory requires scrolling manually through hundreds
  of entries.
- **No text search.** There is no way to type a name and jump to matching items.
- **Item information is shallow.** Base stats only. No enchantment detail, no
  damage calculation breakdown, no potion effect duration, no food buff duration.
- **No favourite/highlight system** beyond the basic favourites toggle.
- **No crafting material awareness.** The inventory does not tell you that an item
  is a crafting ingredient you need, or that dropping it will leave you one short
  of a recipe.

Barter has the same problems plus:
- No indication of merchant gold available before initiating trades.
- No way to see what a merchant is offering without scrolling their full inventory.
- No running total of what the current transaction will cost or yield.

Container has the same inventory problems plus:
- No quick-take-all that respects quest items or stolen goods flags.
- No filter to show only items not already in player inventory.

---

## Item Comparison

The headline feature of the inventory screen. Hovering over any equippable item
surfaces a side-by-side comparison against the currently equipped item in the same
slot. Weapons compare to equipped weapon. Armour pieces compare to equipped armour
in the same slot. Rings compare to each equipped ring.

**Comparison display:**
- Stat-by-stat comparison with delta indicators. Damage +12. Weight -0.5.
  Armour rating -8. Green for improvements, red for decreases, neutral for equal.
- Enchantment comparison. The comparison panel shows both enchantments, not just
  that an enchantment exists.
- Value per weight ratio for both items — a practical signal for loot decisions.
- Two-handed weapons compare against main hand + off hand combined where relevant.

**Dual wield and two-handed:**
Comparing a two-handed weapon against two currently equipped one-handed weapons
shows the combined stats of both one-handers as the baseline. The player sees what
they're trading, not an incomplete comparison.

**Unequipped slot:**
If nothing is equipped in the relevant slot, the comparison shows the base player
stats for that slot with the new item as an improvement against nothing.

**Comparison is always on.** It is not a toggle or a hold-to-reveal. It is the
default presentation for equippable items. Players who don't want it can have a
skin that suppresses it — but the default is comparison always visible.

---

## Text Search and Filtering

**Text search** — a search field, focusable from keyboard (F key default) or
controller (right stick click or dedicated button). Typing filters the item list
in real time. Partial matches included. Search persists within the session until
cleared.

**Extended sorting:**
- Name (A-Z, Z-A)
- Value (high-low, low-high)
- Weight (high-low, low-high)
- Value/weight ratio
- Damage (weapons)
- Armour rating (armour)
- Enchantment (enchanted first, unenchanted first)
- Recently added (items acquired since last inventory open appear first)
- Quantity (for stackable items)

Sort is applied per tab. The player can sort weapons by damage and food by value
simultaneously — each tab remembers its own sort state.

**Extended filtering:**
- Enchanted / unenchanted toggle
- Stolen items toggle
- Quest items (show / hide)
- Favourite items only
- Weight threshold (hide items above N weight — useful for carry weight management)

Filters are additive. Sort and filter state persists across sessions per character.

---

## Item Detail Panel

The detail panel for a selected item is significantly richer than vanilla's name
and base stat display.

**Weapons:**
- Base damage
- Damage with relevant skill level applied
- Critical hit damage
- Speed and reach
- Enchantment with charge level and uses remaining
- Poison applied (name, remaining doses)
- Temper/improvement quality
- Value and weight

**Armour:**
- Base armour rating
- Armour rating with relevant skill level applied
- Enchantment with remaining charge
- Temper/improvement quality
- Value and weight
- Slot (head/chest/hands/feet/shield)

**Potions and food:**
- Full effect description (not just the effect name)
- Duration where applicable
- Magnitude where applicable
- All effects listed for multi-effect potions
- Value and weight

**Ingredients:**
- Known effects (revealed through eating or Alchemy skill)
- Unknown effects indicated as undiscovered
- Value and weight

**Books:**
- Whether the book has been read
- Whether the book teaches a skill
- Which skill it teaches, if known
- Whether it is a skill book that will still yield XP

**Quest items:**
- Which quest this item belongs to
- Whether it can be safely dropped or sold at this point in the quest

**Crafting materials:**
- Whether this item is a known ingredient in any recipe the player has access to
- How many are needed vs how many are held for relevant recipes
- Warning if dropping/selling would leave the player short of a recipe

---

## Crafting Material Awareness

The inventory screen knows what the player can craft. Items that are ingredients
in available recipes carry a subtle indicator — a small icon or tint — so the
player knows before dropping or selling that this item has crafting relevance.

The detail panel goes further: for a selected crafting material it lists the recipes
it participates in, how many the recipe requires, and how many the player currently
holds. "You have 3 iron ingots. The Orcish Sword recipe requires 4."

This is not a crafting guide — it is inventory awareness. The player is not being
told what to make. They are being told what they have that is relevant to making
things, so they can make informed decisions about what to drop.

---

## Favourites and Tagging

The vanilla favourites system — a binary flag per item — is extended to a tagging
system. Players can assign items to named sets that appear as filterable tabs or
persistent filter shortcuts.

**Built-in tags:**
- Favourites (maps to vanilla favourites for compatibility)
- Junk (items the player intends to sell or drop)
- Keep (items the player wants to retain regardless of carry weight pressure)
- Crafting (materials the player is actively collecting)

**Custom tags:** Players can create named tags. An item can belong to multiple tags.

Tags are accessible from the item context menu. Filtering by tag is a one-tap
operation from the filter panel.

---

## Barter Screen

The barter screen follows the layout pattern established across two decades of
RPG trading screens. Everything the player needs is visible simultaneously. No
navigation required. The transaction is visible and reviewable before it is
committed.

### Layout

```
┌─────────────────────────────────────────────────────────────────┐
│                     DISPOSITION HEADER                          │
│  Merchant name, faction, disposition score, price modifier      │
│  Speech skill, active barter perks, relevant standing bonuses   │
└─────────────────────────────────────────────────────────────────┘
┌───────────────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐
│                   │ │          │ │          │ │               │
│  PLAYER           │ │  PLAYER  │ │ MERCHANT │ │  MERCHANT     │
│  INVENTORY        │ │  OFFER   │ │  OFFER   │ │  INVENTORY    │
│                   │ │  PANEL   │ │  PANEL   │ │               │
│  Tabbed list      │ │          │ │          │ │  Tabbed list  │
│  Search/filter    │ │  Items   │ │  Items   │ │  Search/filter│
│  Sort             │ │  staged  │ │  staged  │ │  Sort         │
│                   │ │  for     │ │  for     │ │               │
│                   │ │  trade   │ │  trade   │ │               │
└───────────────────┘ └──────────┘ └──────────┘ └───────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                      TRANSACTION FOOTER                         │
│  Player offers [VALUE in green]  /  Merchant offers [VALUE red] │
│  Net balance  ±[DELTA]  /  Merchant gold remaining [AMOUNT]     │
│  [CANCEL]                                     [CONFIRM TRADE]   │
└─────────────────────────────────────────────────────────────────┘
```

### Disposition Header

The full overview of how this merchant sees the player and what that means
for prices — visible at all times, not buried in a tooltip or a separate screen.

**Disposition score** — the merchant's current disposition toward the player.
Not a raw number in isolation but contextualised: Hostile / Unfriendly /
Neutral / Friendly / Warm / Devoted. The label and a visual indicator. The
raw number is available on hover for players who want it.

**Price modifier** — the direct consequence of disposition, surfaced plainly.
"This merchant is charging you 18% above base price" or "This merchant is
offering you 12% above base value for your items." No arithmetic required
from the player. OHUI displays the pricing values the engine provides.

**Factors contributing to disposition:**
- Speech skill level and relevant perks
- Racial disposition modifiers
- Quest relationship flags
- Previous trade history (if applicable)
- Active bribes or persuasion results
- Faction standing where relevant
- Any active buffs or enchantments affecting Speech

Each factor is listed with its contribution. The player knows exactly why
the merchant likes or dislikes them and what, if anything, they can do about
it. A player who sees "Racial modifier: -10" knows this merchant has a
prejudice. A player who sees "Speech skill: +8" knows levelling Speech
will improve this relationship.

**Merchant specialisation** — the merchant's specialisation is shown in
the header alongside a note of which item categories receive a bonus price.
"Blacksmith — pays 15% above base for weapons and armour."

**Merchant gold** — the merchant's current available gold, shown prominently
in the header. The player knows before building a transaction whether the
merchant can afford what they want to sell.

### How to Improve This

A collapsible affordance in the disposition header. Collapsed by default.
Expand it and the screen surfaces every lever the player can pull right now
to improve their standing with this merchant before committing to a bad rate.

Not a tutorial. A contextual, situational readout calculated from the player's
current inventory, active effects, skill levels, quest state, and the merchant's
specific disposition factors. It shows only what is relevant to this merchant
in this moment. Nothing generic, nothing irrelevant.

**Available now** — actions the player can take immediately:
- Potions in inventory that affect Speech or disposition
  "Speech of Persuasion in inventory — +15 Speech for 60 seconds. Use before trading."
- Apparel in inventory that improves disposition
  "Fine Clothes in inventory — equipping adds +15 disposition with most merchants."
- Spells available that affect disposition or Speech
  "Charm spell available — would add +20 disposition for this conversation."
- Active equipped items penalising disposition
  "Current equipped items: -8 disposition with this merchant."
  The player draws their own conclusions about what to swap.

**Active** — bonuses already in effect:
- Current blessings affecting Speech
- Active enchantments on equipped items affecting Speech
- Active potions or powers
- Confirmed so the player knows what they have already applied

**Actionable later** — improvements within reach but not immediate:
- Quests with this merchant or their faction that would improve standing
  "Complete 'Hired Muscle' for the Companions — +25 with all Whiterun merchants."
- Nearby skill thresholds that unlock relevant perks
  "Reach Speech 50 — unlocks Haggling perk (+10% better prices everywhere)."
- Faction standings that affect this merchant specifically

**Situational context** — fixed factors the player cannot change but should know:
- Racial modifier if applicable
  "This merchant has a racial bias. Disposition is -10 regardless of other factors."
- Merchant-specific relationship flags from quests
- Permanent standing from completed or failed quests

The "available now" section is the one players act on. The crop top, the
bloody armour, the Speech potion sitting in a pocket — all surfaced at the
exact moment they are relevant. Players who have developed the pre-trade ritual
through experience now have the screen confirming and completing that ritual
for them. Players who have never developed it learn it naturally by seeing
what the screen suggests and watching the disposition score respond.

The affordance is collapsed by default so it never clutters the screen for
players who already know what they're doing. It is always one tap away for
everyone else.

---

### Player and Merchant Inventory Panels

Both inventory panels are full instances of the `TabbedListPanel` component
with all inventory screen features active:

- Text search
- Category tabs
- Extended sort and filter
- Item detail panel on selection
- Stolen item flagging (player side — stolen items cannot be sold to
  merchants without the relevant perk, clearly indicated)
- Merchant specialisation indicator per item (merchant side and player side
  — items in the merchant's specialisation are highlighted so the player
  knows which of their items will yield the best price)

Items are moved between inventory and offer panels by a single action —
a button press or click. Moving an item to the offer panel stages it
for trade. Moving it back unstages it. Nothing is committed until confirm.

### Offer Panels

Two staging panels sit between the inventory lists — the player's offer
on the left, the merchant's offer on the right. Both show:

- Items staged for trade with quantity and individual value
- Running subtotal for that side
- Items can be returned to their inventory panel from here
- Quantities are adjustable for stackable items before confirming

The offer panels are the transaction in progress. The player builds their
offer, the merchant's offer is whatever the player has requested from the
merchant inventory. Both sides visible simultaneously.

### Transaction Footer

The footer summarises the transaction in real time as the player builds it:

**Player offers** — total value of items in the player's offer panel,
in green. This is what the player is giving.

**Merchant offers** — total value of items in the merchant's offer panel,
in red. This is what the player is receiving.

**Net balance** — the delta. Positive (green) means the player receives
gold to balance the transaction. Negative (red) means the player pays
gold to balance it. Updates in real time as items are added or removed
from either panel.

**Merchant gold remaining** — how much gold the merchant will have after
this transaction completes. If the transaction would exceed the merchant's
available gold the footer warns before confirm is available.

**Confirm and cancel** — confirm executes the transaction. Cancel returns
all staged items to their original panels with no changes made. The player
can cancel at any point before confirming.

### Fair Value Indicator

Each item in both inventory panels displays its base value alongside its
current trade value — what this merchant will actually pay or charge for
it, accounting for disposition, specialisation, Speech skill, and active
perks. The player sees the gap between what something is worth and what
this merchant will give them for it.

For items in the player's offer panel: "Base value 240 — Merchant pays 187"
For items in the merchant's offer panel: "Base value 180 — Merchant charges 231"

The gap is not hidden. The player knows exactly how much this relationship
is costing them and can decide whether to invest in improving it before
trading.

### Stolen Items

Stolen items in the player's inventory are clearly flagged. Merchants who
will not accept stolen goods filter them out of the player's visible sell
list by default — they are still accessible via a filter toggle for players
who want to see them, but they cannot be moved to the offer panel for these
merchants. Merchants who will accept stolen goods (with relevant perks or
faction relationships) show no restriction.

The fence mechanic — selling stolen goods to specific merchants — is surfaced
clearly. A fence merchant's inventory panel header notes their willingness
to accept stolen items.

---

## Container Screen

The container screen is the inventory screen in a loot configuration — container
contents on one side, player inventory on the other. The same shared foundation
applies.

Two mods define what first-party container intelligence must cover:

**moreHUD** by ahzaab — surfaces contextual item information inline at the point
of looting. Enormous install base, effectively frozen. Every piece of information
moreHUD provides belongs in the container screen as first-party functionality.
OHUI absorbs it entirely.

**The Curator's Companion (TCC)** — surfaces LOTD museum item status at the point
of interaction. Whether an item is a museum piece, whether it is already donated
and displayed, whether it is still needed for a specific exhibit. LOTD is on the
frozen giants list and TCC's functionality is the container-layer expression of
that native support.

Neither mod needs to be installed. Their functionality ships in OHUI. Players who
have them installed benefit from the soft dependency layer. Players who don't get
the same information natively.

### Inline Item Intelligence

Every item entry in the container panel displays contextual information inline —
no hovering required, no separate panel to open. The information needed to make
a loot decision is present on the entry itself.

**Carry count** — how many of this item the player already has in their inventory.
"Carrying: 3" displayed inline. For items where quantity matters (potions,
ingredients, arrows, crafting materials) this is the most important single piece
of loot information and it is never currently surfaced at the point of looting.

**Book status** — for books and skill books:
- Already read
- Skill book — not yet read (will grant skill XP)
- Skill book — already read (no XP remaining)
- Spell tome — spell already known
- Spell tome — spell not yet known

The distinction between a skill book that still has XP and one that doesn't is
one of the most practically useful pieces of information in the game and has
never been surfaced at the point of picking the book up.

**Soul gem status** — for soul gems:
- Empty
- Filled — soul size contained (Petty / Lesser / Common / Greater / Grand)
- Black soul gem status where applicable

**Enchantment indicator** — whether the item is enchanted, visible before
picking it up. For weapons and armour in containers, knowing something is
enchanted before looting it changes the decision.

**Value and weight ratio** — inline on the entry. The single most useful
quick-loot signal for players managing carry weight. A high V:W ratio item
is worth taking. A low one probably isn't.

**LOTD museum status** — for players with Legacy of the Dragonborn:
- Needed — which exhibit or display case requires this item
- Already donated and displayed
- Duplicate — already in the museum, this would be redundant
- Not a museum item

Museum status is shown only when LOTD is detected in the load order. It
does not appear for players who do not have LOTD installed.

**The full inline entry:**
```
[Enchanted] Iron Sword          45g / 9.0  V:W 5.0  Carrying: 2
                                Museum: Needed — Dragonborn Hall
```

All of this visible before the player touches the item. All of it changing
the loot decision.

### Take All

Takes all items from the container subject to configurable rules:
- Skip quest items (default on)
- Skip stolen items (default off — taking from a container you own isn't
  theft, but some containers have mixed ownership)
- Skip items over a weight threshold (configurable)
- Skip items already in player inventory at maximum stack size
- Skip already-read skill books with no XP remaining (default on)
- Skip LOTD items already donated and displayed (default on, LOTD only)

Take All with these rules applied is a single action. No confirmation unless
the action would exceed carry weight capacity, in which case the player is
warned and can choose to take what fits or cancel.

The rules are configurable from the container screen's context menu. The
player can adjust them per session or set persistent defaults in MCM2.

### Filters

**New items** — items not already in player inventory. The most common loot
filter. One tap to show only what the player doesn't have.

**Museum needed** — LOTD players can filter to show only items that are
needed for the museum. Everything else fades. Fast identification of
relevant loot in dense containers.

**High value** — items above a configurable V:W threshold. Fast
identification of worth-taking items in mixed containers.

**Enchanted only** — surface only enchanted items.

**Not carrying** — hide items the player already has at least one of.
Complementary to the carry count indicator — the indicator tells you
how many you have, this filter hides items you already have entirely.

### Carry Weight Warning

If taking all or taking selected items would exceed carry weight, the screen
indicates this before the action is taken. The warning shows:
- Current carry weight
- Weight of items about to be taken
- Projected carry weight after taking
- How far over the limit the player would be

The player can choose to take what fits (items taken in V:W order until
capacity is reached), take all anyway (over-encumbered), or cancel.

### Layout

The container screen uses the full screen. Everything visible simultaneously.
No toggling between container and inventory. No half-screen lists. The same
four-column layout established by the barter screen, with staging panels
between the two inventory lists.

```
┌───────────────────────────────────────────────────────────────────┐
│                        CONTAINER HEADER                           │
│  Container name / owner / locked state / weight capacity if any   │
└───────────────────────────────────────────────────────────────────┘
┌─────────────────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐
│                     │ │          │ │          │ │               │
│  CONTAINER          │ │CONTAINER │ │  PLAYER  │ │  PLAYER       │
│  CONTENTS           │ │  TAKE    │ │  DEPOSIT │ │  INVENTORY    │
│                     │ │  PANEL   │ │  PANEL   │ │               │
│  Full inline        │ │          │ │          │ │  Full         │
│  item intelligence  │ │  Items   │ │  Items   │ │  inventory    │
│  moreHUD / TCC      │ │  staged  │ │  staged  │ │  features     │
│  V:W / carry count  │ │  to take │ │  to leave│ │  active       │
│  Museum status      │ │          │ │          │ │               │
└─────────────────────┘ └──────────┘ └──────────┘ └───────────────┘
┌───────────────────────────────────────────────────────────────────┐
│                       TRANSACTION FOOTER                          │
│  Taking [N items / W weight]  Depositing [N items / W weight]     │
│  Carry weight [current → projected]     [CANCEL]     [CONFIRM]    │
└───────────────────────────────────────────────────────────────────┘
```

The staging panel model from barter applies directly. Nothing moves until
the player confirms. Items to take are staged in the take panel. Items to
deposit are staged in the deposit panel. Both sides can be built simultaneously.
The footer shows the carry weight impact of the entire pending transaction
before anything is committed.

One trip. Full screen. No toggling. The player sees what they are taking and
what they are leaving behind at the same time, with full item intelligence
on both sides.

**Bidirectional transfer** — items move in both directions without leaving
the screen. A player clearing out a dungeon chest and restocking from their
own inventory simultaneously does it in one interaction. Home organisation,
follower inventory management, storage chest maintenance — all the same
screen, same pattern, no round trips.

Both the container panel and the player inventory panel are full `TabbedListPanel`
instances with all inventory features active on both sides — search, sort,
filter, item detail panel. The inline item intelligence (moreHUD / TCC) applies
to the container panel. Full inventory features apply to both.

---

## Character Preview

The inventory screen features a live character preview panel — a real-time render
of the player character as they currently appear, framed at chest height with a
slight angle, photomode style. The actual character. Every body mod, skin mod,
hair mod, and equipment piece reflected exactly as it appears in the world.

This is the Real or Nothing principle applied to the inventory screen. No
silhouettes. No generic mannequins. No placeholder icons standing in for a person.
The actual player character rendered live in a native C++ viewport, or the panel
is absent. Nothing in between.

The same viewport mechanism used for NPC portraits in the dialogue screen and
location preview in the map screen. The player character is already being rendered
by the engine. OHUI positions a camera to frame them and renders into the panel.
No new rendering capability required — a new application of an established one.

**Real-time equipment preview:**
Hovering over any equippable item in the list updates the character preview to
show the character wearing that item. The change is immediate. Moving off the item
reverts to the current equipped state. The player is trying on armour in a mirror
before committing.

This works cleanly because the character preview renders a clone of the player
actor in OHUI's photobooth cell. The preview item is applied to the clone only.
The actual player actor and their actual equipment are untouched throughout.

This connects directly to item comparison — the stats panel shows the delta,
the character preview shows the visual. Both update simultaneously on hover.
The player sees the number and the look at the same time.

**Camera control:**
The player can rotate the preview camera with the right stick or mouse drag —
a full 360 degree view of the character. Zoom in and out along the camera axis.
Want to see the back of the armour? Rotate. Want to see the boots up close?
Zoom and rotate. The preview is explorable, not a fixed glamour shot.

**Panel behaviour:**
The character preview panel is a first-class element of the inventory screen
layout — not a tooltip, not an overlay. It occupies a defined region of the
screen alongside the item list and detail panel. Panel size is configurable.
Skin authors can position it anywhere in their layout — left panel, right panel,
background element, floating overlay.

On controller the preview is always visible. On keyboard and mouse it is always
visible. It is not hidden by default and not a mode the player enters — it is
part of the inventory screen as shipped.

**Real or Nothing:**
If a meaningful render of the player character is not available — an edge case
of engine state where the character cannot be framed — the panel is absent. No
placeholder. No silhouette. The skin handles absence by collapsing the panel
area gracefully. This is not expected to be a common case. The player character
is always loaded when the inventory is open.

---

## Character Impact Panel

An item has stats. A character has a state. These are different things and the
inventory screen has always conflated or ignored the distinction entirely.

The character impact panel shows not what the item is, but what equipping it
does to the player's character specifically — right now, with their current
skills, perks, equipment, and active effects. The delta between the player's
current state and the state they would be in after equipping this item.

This sits alongside the item comparison panel. Item comparison shows the item
stats delta. The character impact panel shows the character state delta. Both
update in real time on hover.

### Skill-Adjusted Effectiveness

Displayed item stats have always been lies by omission. A two-handed sword with
48 base damage does not deal 48 damage in the hands of a player with 27 in
Two-Handed. A bow that lists 32 damage does not deal 32 damage at 20 Archery.
A destruction spell listed at 40 magicka cost does not cost 40 magicka at 15
Destruction. The displayed stat is the theoretical maximum. The actual value
depends entirely on the player's skill level.

OHUI displays the skill-adjusted values the engine provides as the primary
stat. The base value is still present but secondary. What the player sees
first is what the item will actually do in their hands today.

**The honest warning:**
When equipping an item would result in significantly reduced effectiveness due
to skill mismatch, the panel says so explicitly:

"At your current Two-Handed skill (27), this weapon deals 38% less damage than
its base value. Your One-Handed skill (48) would make a comparable one-handed
weapon significantly more effective."

This is not a judgment. It is information. The player may have reasons to equip
a weapon in a school they're developing. But they should know the cost before
they commit, not after they've spent ten minutes wondering why their damage
fell off.

The same principle applies to:
- All weapon types (One-Handed, Two-Handed, Archery, blocking with a shield)
- Spell costs (Destruction, Restoration, Conjuration, Illusion, Alteration)
- Armour effectiveness (Light vs Heavy Armour skill, armour cap proximity)
- Lockpicking and pickpocket items where relevant

### Defensive Impact

- Net armour rating change at current Heavy or Light Armour skill
- Armour cap proximity — "you are 12 points from the armour cap" or "you are
  already at the armour cap, additional armour rating has no effect"
- Resistance changes per element (fire, frost, shock, magic, poison, disease)
- Net resistance delta across all elements where the swap changes them

### Offensive Impact

- Weapon damage at current skill level (not base damage)
- Attack speed delta
- Critical hit chance and damage delta
- Spell cost delta at current skill level for enchantments affecting magic schools
- Spell magnitude delta where enchantments affect spell power

### Movement and Encumbrance

- Encumbrance delta — how much this swap adds to or removes from current carried
  weight
- Carry weight remaining after the swap
- Movement speed delta — OHUI displays the movement speed values the engine
  provides for the current and projected equipment states

"Equipping this armour adds 25 to your encumbrance and reduces movement speed
by 8%."

Movement speed impact is the stat that has never been surfaced at the point of
decision. Players discover they have slowed down by noticing it in gameplay.
OHUI tells them before they equip.

### Active Effects Delta

- New enchantment effects gained from the item being equipped
- Enchantment effects lost from the item being replaced
- Net skill bonuses delta across all affected skills
- Net carry weight bonus delta
- Any conditional effects and the conditions under which they activate

### Armour Set Bonuses

For armour sets with matching set bonuses (vanilla's matching armour perk
requirements, mod-added set bonuses), the panel indicates:
- Whether equipping this piece completes a set bonus
- Whether removing the current piece breaks a set bonus
- What the set bonus is and whether it is currently active

---

## Outfits and Loadouts

A named outfit is a saved equipment set — every equipped item in every slot,
captured at the moment of saving and recallable at any time. The player names
it, saves it, and can switch to it from the inventory screen or from a HUD widget.

This is a launch requirement. It is fully in scope, fully a UI feature, and
fully deliverable in 1.0.

**Creating an outfit:**
From the inventory screen, with a desired set of equipment worn, the player
selects "Save as Outfit" from the inventory context menu. A name input appears
(OSK on controller). The outfit is saved with the current equipment state,
including weapons, armour, jewellery, and clothing in every slot.

**Outfit panel:**
A dedicated panel in the inventory screen lists all saved outfits. Each entry
shows the outfit name and, where available, a thumbnail from the character
preview at the time of saving — a small portrait of the character wearing that
outfit. The player can browse saved outfits, preview them in the character
preview panel in real time, and equip them in one action.

**Real-time preview:**
Selecting or hovering over an outfit in the panel updates the character preview
immediately — the character is shown wearing that outfit before the player commits
to equipping it. Simultaneously, the character impact panel shows the full
aggregate delta between the hovered outfit and whatever is currently worn.

The player sees the look and the complete character state consequence side by
side, before committing to anything:

"Switching to Thieves Guild Armour from current equipment:
Armour rating -84 (412 → 328) / Movement speed +6% / Encumbrance -18 /
Frost resist -20% / Sneak +15 / Pickpocket +10 / One-Handed +10"

The outfit delta uses the same calculation engine as the individual item character
impact panel. An outfit is multiple simultaneous item swaps. The delta is the
aggregate of all of them. The outfit selector is therefore not just a wardrobe
— it is a decision-making tool. The player opening it before a dungeon run sees
exactly what they are trading before they swap, not after.

**Equipping an outfit:**
Equipping an outfit swaps all slots to the saved items. If a saved item is no
longer in the player's inventory (sold, dropped, stored in a chest) the outfit
equips everything it can find and flags the missing items clearly. The player
knows what is missing before the swap completes.

**Outfit management:**
- Rename
- Overwrite (update saved state to current equipment)
- Duplicate
- Delete
- Reorder (drag to sort)

**HUD widget integration:**
Saved outfits are available as targets in the HUD action bar and quick-slot
system. A player who switches between a combat loadout and a social loadout
can swap between them without opening the inventory. One button. The character
preview on the HUD widget (if the skin supports it) updates to reflect the
active outfit.

**Combat vs social loadout pattern:**
The most common use case is a combat outfit and a social outfit — heavy armour
for dungeons, fine clothes for court. OHUI ships with a built-in distinction
between combat and social outfit slots that can be toggled automatically based
on context (entering combat, entering a city). This is an optional feature,
off by default, configurable in MCM2.

---

## Follower Outfits

The outfit system is built for characters with equipment slots. Followers are
characters with equipment slots. The extension from player outfits to follower
outfits is almost free once the player-side system exists and it solves a
genuine daily-use problem.

Managing a follower's equipment is currently a manual slot-by-slot operation
through the container screen. Dressing Lydia in heavy armour for a dungeon
run and wanting her in something presentable for Dragonsreach is the same
operation repeated every time — open container, swap piece by piece, close.
With follower outfits it is one selection.

**Where it surfaces:**
The follower container screen gets an outfit panel in the header — the same
outfit panel as the player inventory screen, scoped to the follower's saved
outfits. The player can browse the follower's saved outfits, preview them
on the follower in the character preview viewport, see the full character
impact delta, and equip in one action.

Same system. Same components. Same preview. Same delta panel. Different
subject.

**Outfit management for followers:**
Follower outfits are created and managed the same way as player outfits —
from the follower container screen with the follower's current equipment
state. Named, saved, overwritable, reorderable. A player who maintains
multiple followers can save and manage outfits for each independently.

**NFF integration:**
NFF is on the frozen giants list. Follower outfit management is one of
the things NFF players use NFF for. OHUI's follower outfit system is
native first-party functionality that covers the same use case. NFF
players with OHUI get the full outfit system through the container screen
without any NFF-specific configuration.

**Why native and not NFF:**
NFF's outfit system exists and players use it in large numbers despite it
being genuinely painful to use — buried in MCM pages, entirely text-based,
no preview, no visual feedback, configured through dropdowns that require
knowing what you want before you can express it. Players tolerate it because
the underlying feature is something they want badly enough to accept a bad
implementation of.

OHUI's version is what the feature should have always been. Visual, immediate,
preview-first. The complete design is: here is your follower, here is what
they look like in this outfit, one button to equip it.

The papyrus script approach also has a specific failure mode that has become
a running joke in the community. Arriving home, waiting 8-10 seconds for a
queued script to fire, watching Lydia stand in full Daedric plate while the
helmet slowly dematerialises, hoping the script didn't get lost behind
forty other things. Whatever else a "home outfit" might involve removing
suffers the same indignity.

OHUI's outfit swap is immediate. No scripts queued. No waiting. No helmet
haunting the bedroom. The player opens the container screen, selects the
outfit, sees it in the preview, confirms. Done.

**Outfit labels and the trigger mod pattern:**
Outfits can be assigned a semantic label — a freeform tag the player sets
when creating or editing an outfit. "Home", "Combat", "Social", "Stealth",
or anything else. The label is metadata. OHUI stores it, exposes it via the
API, and does nothing else with it.

What acts on labels is deliberately out of scope for OHUI. A separate trigger
mod reads the label and fires the swap when a condition is met — arriving at
a home cell, entering combat, crossing a city boundary, whatever the mod
decides. That mod does not need to know anything about equipment, followers,
or previews. It asks OHUI to swap a character to their outfit with a given
label. OHUI handles the rest. Instantly, correctly, no scripts queued.

The label API is the clean seam between OHUI's responsibility and the trigger
mod's responsibility. Multiple trigger mods with different philosophies can
coexist — one for cell type, one for time of day, one for quest state, one
for MCM-configured conditions — all speaking the same language, all getting
the same execution quality from OHUI.

This is also the correct answer for the home outfit use case specifically.
OHUI does not decide when to swap Lydia's outfit. A trigger mod decides that.
OHUI makes the swap happen the right way when asked.

**Scope:**
Follower outfits are a launch requirement alongside player outfits. The
infrastructure is shared. The additional cost of extending to followers
is low. Shipping player outfits without follower outfits would be a
noticeable gap for anyone who plays with companions.



---

## On the Radar — Transmog

Transmog — wearing the stats of one item with the appearance of another — is a
gameplay system, not a UI system. OHUI does not ship a transmog mechanic. Whether
a piece of Daedric armour can look like leather armour is a decision that lives
outside the UI entirely.

However: if a transmog system exists — vanilla disguise mechanics, a mod framework,
or a future native implementation — the inventory screen is the natural surface
for managing it. The UI has a role. The character preview makes transmog management
visually immediate in a way no current implementation achieves. Selecting an
appearance override and seeing it applied to the character preview in real time
before committing is the correct experience for this feature.

Transmog UI support is noted here so that when a transmog system exists, the
inventory screen is ready to surface it. It is not in launch scope. The hook
should be considered when designing the detail panel and outfit system so it
does not need to be retrofitted later.

---

## On the Radar — Colour Theme from Outfit

Two interpretations of this concept, both worth capturing:

**Automatic palette extraction:** Extract the dominant colours from the currently
equipped armour and shift the UI's design token palette to match. Dark steel
armour shifts the UI cool and metallic. Forsworn bone shifts it warm and earthy.
The UI reflects the character's aesthetic. This is ambitious, partially outside
the UI's own scope (it requires colour extraction from rendered geometry or
texture data), and the quality of automatic results is unpredictable. It belongs
on the long-term radar, not the launch plan.

**Manual outfit theme pairing:** The player manually selects a UI colour theme
to pair with an outfit, saved as part of the outfit definition. Equipping the
outfit also activates its paired theme. This is simpler, fully in scope, fully
reliable, and a natural extension of the outfit system once it exists. It is
not a launch feature but it is a near-term feature with a clear implementation
path. The outfit data model should include a theme slot from day one so it does
not need to be added later.

Both interpretations are noted here. Neither is in launch scope. The outfit
data model accommodates the manual pairing approach from the start.

---

## Player Experience Goals

Item comparison is the headline wow moment for the inventory screen. It is
immediately obvious, immediately useful, and something every player who has ever
hovered over a weapon wondering if it is better than what they have will feel
instantly. It has been in every major RPG for fifteen years. It has never been
in Skyrim natively. The first time a player hovers over a sword and sees the
delta appears next to their equipped weapon's stats, they will understand
immediately what has been missing.

Text search is the long-overdue correction. Players with large mod-added loot
pools — LOTD collections, Immersive Armours, any content mod with item density
— have been scrolling manually for years. One keystroke to find any item by name
is quiet relief that every player will feel on first use.

Crafting material awareness is the detail that signals OHUI understands how
Skyrim is actually played. Players make unintentional decisions — selling a
crafting ingredient they needed, dropping the last item required for a recipe
they've been assembling — constantly. The inventory screen knowing about crafting
relevance prevents those decisions silently, without a tutorial or a warning
system. It is just there when you need it.

---

## Open Questions

1. **SkyUI sorting mods:** iWant Widgets, Inventory Tweaks, and similar mods
   extend SkyUI's sorting. Do they participate in OHUI's sorting system via the
   compatibility shim, or do they need explicit support?

2. **Equipment set management:** Some mods (Serio's Equipment Manager and similar)
   provide equipment set switching — save a loadout, swap to it. This is a feature
   that belongs in the inventory screen or as a HUD widget. Worth designing but
   probably not launch scope. Noted for future.

3. **3D item preview:** A rotating 3D render of the selected item in the detail
   panel. Uses the same native C++ viewport mechanism as NPC portraits and map
   preview. Visually compelling. Cell loading not required — items are always
   available in the render cache. Feasibility is high. Launch scope question.

4. **Mod-added item categories:** Mods that add items with custom categories
   (LOTD museum items, ammunition types from archery mods) — do they surface as
   additional tabs automatically or require explicit registration?
