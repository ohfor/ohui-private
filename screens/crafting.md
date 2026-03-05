# Crafting Screens — Smithing, Alchemy, Enchanting, Cooking

## Architectural Foundation

### Mechanical Fidelity First

The crafting screens are the most heavily modded mechanical territory in Skyrim.
The recipe and ingredient systems underpin an enormous ecosystem of overhauls —
CACO, CCOR, Apothecary, Alchemy Redone, Smart Optimal Salves, Ordinator,
Vokrii, and dozens of others — all of which hook into crafting mechanics at the
engine level, below the UI entirely.

OHUI's crafting screens own the presentation layer. They own nothing else.

**What OHUI owns:**
- How recipes are displayed, searched, sorted, and filtered
- How ingredient and material information is presented
- How output and upgrade previews are shown
- QOL features layered on top of what the engine surfaces

**What OHUI never touches:**
- Recipe conditions and visibility logic
- Crafting formulas and output calculations
- Ingredient effect systems
- Temper and upgrade value computation
- Anything a crafting overhaul might modify

When OHUI's smithing screen needs to know which recipes are available, it asks
the engine. The engine answers — having already been modified by CCOR, or
Ordinator, or whatever overhaul is installed. OHUI renders the answer. It never
computes it. The moment OHUI tried to compute it, it would conflict with every
overhaul that has already computed it differently.

This is the correct and non-negotiable architectural constraint for all four
crafting screens. Mechanical fidelity to the base game's systems — as modified
by whatever the player has installed — is the foundation everything else builds
on.

### SCIE

SCIE (Skyrim Crafting Inventory Extender, mod 170497) solves the inventory
pooling problem for crafting stations — containers, followers, horses, global
storage — via zero-transfer C++ interception of the engine's inventory queries.
It operates entirely below the UI layer.

SCIE has no hard dependency on OHUI. OHUI has no hard dependency on SCIE.
Neither mod knows the other exists. The relationship is architectural:

**SCIE without OHUI** — works exactly as today against vanilla screens or SkyUI.
SCIE does not care what renders above its inventory layer.

**OHUI without SCIE** — crafting screens work normally against player inventory.
No degraded state. No warnings. No missing functionality. The vanilla inventory
scope applies.

**OHUI with SCIE** — OHUI's crafting screens transparently receive the pooled
inventory SCIE provides at the engine level. Neither mod coordinates with the
other. The player gets the sum of both.

This is the soft dependency principle expressed in reverse. Not OHUI reaching
out to SCIE — SCIE's work flowing upward into OHUI naturally because they
operate at different layers of the same stack. The player who has both installed
benefits. The player who has neither gets the correct vanilla experience. No
special case required.

---

## Active Effect Timers

For players using Fortify Smithing, Fortify Enchanting, or Fortify Alchemy
potions, the crafting screen surfaces a dedicated timer strip showing any
active effects that influence results at the current station.

The strip is visible only when relevant effects are active. Players who
do not use fortify potions never see it. Players who do see exactly what
they are working with and how long they have.

```
⏱ Fortify Smithing 32%     0:43 remaining  ████████░░░░
⏱ Fortify Enchanting 25%   1:12 remaining  ████████████░░░░
```

Each entry shows the effect name, the current magnitude, the remaining
duration as a countdown, and a draining progress bar. The bar shifts
colour as time runs low — neutral at full duration, amber under 30
seconds, red under 10. Not alarming. Present and honest.

**Station relevance filtering:**
Only effects that influence results at the current station are shown.
Fortify Smithing does not appear at the alchemy lab. Fortify Alchemy
does not appear at the enchanting table. The strip shows only what is
actually affecting what the player is doing right now.

**Live output preview coupling:**
The output previews — enchantment strength, tempering result, potion
magnitude — update in real time as active effects tick down. The
player watches the predicted result change as the buff expires. They
know exactly when they need to stop deliberating and commit.

A player who drinks a Fortify Enchanting potion and opens the enchanting
table no longer has to juggle the active effects widget on their HUD
while also making decisions about enchantments and soul gems. The
information is where they are looking, updated every second, accurate
until the last tick.

**Scope note:**
This feature is for players using vanilla-style crafting buff potions.
Many players and modded setups do not use Fortify Smithing, Fortify
Enchanting, or Fortify Alchemy at all — whether by choice, by mod
restriction, or by overhaul design. For those players the timer strip
is simply absent. The screen is not designed around the assumption that
these potions will be present. The timer strip is an additive layer
that appears when relevant and disappears when not.

---

## Shared QOL — All Crafting Screens

Before the screen-specific features, several QOL improvements apply to all
four crafting screens equally.

### Recipe Search and Filtering

Vanilla recipe lists are flat scrolls with no search and minimal categorisation.
A heavily modded game with CACO, a cooking overhaul, and a few content mods can
have hundreds of recipes at a single station. Finding a specific one means
scrolling through all of them.

**Text search** — same pattern as inventory and magic screens. Type to filter
the recipe list in real time. Searches recipe name. Partial matches included.

**Filters:**
- Can craft now (all materials available) — default on
- Missing one ingredient (close to craftable)
- Missing materials (show everything, dim uncraftable)
- Recently learned recipes (mod-added recipes appear at top on first visit)
- By category where categories exist

**Sort:**
- Default (game order, respects overhaul categorisation)
- Alphabetical
- Recently added
- By output type where relevant

The filter and sort state persists per station type. The player's alchemy
filter preferences do not bleed into their smithing preferences.

### Recipe Completion Indicator

Every recipe in the list shows at a glance how complete the player's materials
are. A clear visual distinction between:

- **Craftable** — all materials present
- **Almost** — one ingredient or material short
- **Incomplete** — multiple materials missing

The "almost" indicator is the useful one. Players who are one iron ingot short
of a recipe they want should know that immediately rather than discovering it
after clicking through to the recipe detail. The "almost" filter surfaces all
of these at once for players managing their crafting list.

### Material Source Awareness

Where materials are coming from is surfaced inline. If the player has 3 iron
ingots in their inventory and 5 more in a SCIE-registered container, the
recipe list shows the combined count without distinction — SCIE handles that
transparently. What OHUI adds is source attribution on the detail panel:
"Iron Ingot ×8 — Inventory: 3, Chest (Breezehome): 5." The player knows
where their materials are without having to check separately.

This is read-only information. OHUI does not manage SCIE containers. It only
surfaces what SCIE's API reports.

---

## Smithing Screen

### The Problem

The smithing screen has two distinct modes — **crafting** (making new items)
and **tempering** (improving existing ones) — and fails differently in each.

In crafting mode: the recipe list is a flat scroll with no search, no
indication of what the player is close to making, and no preview of what
the finished item will look like compared to what they already have.

In tempering mode: the player selects an item and commits materials to improve
it with no information about what the improvement will actually produce. The
stat change is invisible until after the materials are spent.

### Crafting Mode

Recipe list with all shared QOL applied — search, filter, sort, completion
indicator, material source attribution.

**Output preview:**
Selecting a recipe shows the finished item's stats in the detail panel —
at the player's current Smithing skill level with relevant perks applied.
Not the base item stats. The stats the player will actually receive.

This connects to the character impact panel from the inventory screen. If
the recipe produces an equippable item, the detail panel shows the delta
against the player's currently equipped item in the same slot. The player
sees whether what they're about to craft is an upgrade before they commit
the materials.

**Material requirement detail:**
Each required material shown with current availability — inventory count,
SCIE container count if applicable, and how many are needed. The player
sees at a glance whether they have enough and where it's coming from.

### Tempering Mode

The tempering screen is where the most significant QOL work lives.

**Upgrade preview:**
Before committing materials, the player sees exactly what the tempered item's
stats will be at the next quality tier. The comparison is explicit:

```
Fine Iron Sword → Superior Iron Sword
Damage:        23 → 26  (+3)
Current skill: Smithing 42 — Journeyman quality available
```

The preview is engine-sourced. OHUI asks the engine what the tempered result
will be. CCOR's formulas, Ordinator's perk effects, any overhaul's tempering
calculations are reflected automatically because the engine has already applied
them. OHUI renders the answer.

**Quality tier roadmap:**
Shows the full progression available to this player for this item — what
quality is achievable now, what requires more skill, what requires perks
the player doesn't have yet. Read-only planning information. The player
knows what they're working toward.

**Perk requirement transparency:**
If a higher quality tier is locked behind a perk the player doesn't have,
the screen says which perk. "Master quality requires Ebony Smithing perk."
The player is not left guessing why their upgrades have a ceiling.

---

## Alchemy Screen

### The Problem

Alchemy has the most opaque information design of any system in the game.
The ingredient effect discovery mechanic is intentional — the player learns
effects by experimenting — but the screen provides no tools for reasoning
about combinations even with known effects. A player with full effect
knowledge still has no way to plan a potion without external tools or
trial and error.

### The Constraint

OHUI does not touch the effect discovery system. What effects are known,
how they become known, what happens when the player eats an ingredient —
none of that is OHUI's concern. Alchemy Redone, CACO, and others modify
this system extensively. OHUI presents whatever the engine reports about
known effects. Nothing more.

### Ingredient Detail

In the vanilla screen an ingredient's known effects are shown when selected.
OHUI extends this without touching discovery:

**Known effects** — displayed clearly with magnitude and duration where
applicable. Same information as vanilla, better presented.

**Unknown effects** — shown as undiscovered slots. The player knows an
ingredient has four effects and can see how many they've discovered. The
mystery is preserved — the number of unknown slots is surfaced but not
their content.

**Effect matching indicator:**
When an ingredient is selected, other ingredients in the list that share
at least one known effect with it are highlighted. The player can see at a
glance what combines usefully with what they've selected. This is purely
presentation of known information — no discovery is bypassed.

The matching is based only on known effects. An ingredient whose matching
effect is undiscovered does not get highlighted. The discovery system is
not circumvented.

### Combination Preview

When two or more ingredients are selected, the combination preview shows
what potion or poison will result — based on known effects only. If the
combination would produce an effect via two undiscovered effects, it is
not predicted. The player discovers it by brewing.

**Preview panel:**
- Potion or poison name (as the engine would generate it)
- All effects the combination produces, with magnitude and duration
- At current Alchemy skill level with relevant perks applied
- Value of the resulting item

Again — OHUI asks the engine what this combination produces. The engine
answers with CACO's values, Apothecary's values, whatever overhaul is
present. OHUI renders the answer.

**Skill-adjusted values:**
A player with Alchemy 20 and a player with Alchemy 80 brewing the same
combination get different results. The preview shows what this player
will produce right now, not a theoretical maximum.

### Favourite Combinations

Players who brew the same potions repeatedly can save favourite
combinations — a named set of ingredients that appears in a favourites
tab for fast access. One tap to load the combination, one tap to brew.

Favourite combinations that are no longer fully available (missing an
ingredient) are indicated as incomplete rather than removed. The player
sees what they're missing.

---

## Enchanting Screen

### The Problem

Enchanting is the most complex system in the game and the screen is the
flattest. A list of soul gems. A list of enchantments. No information about
what strength an enchantment will be at the player's current skill. No
comparison between soul gem sizes and resulting enchantment strength. No
indication of whether a larger soul gem is worth using for this particular
enchantment. No build planning surface at all.

### Enchantment Strength Preview

The headline feature. Selecting an enchantment and a soul gem shows the
resulting enchantment magnitude before committing either.

```
Fortify One-Handed
Soul Gem: Grand (filled)   Skill: Enchanting 67
Result: +28% One-Handed damage
Perk modifier: +20% (Insightful Enchanter active)
```

The values are engine-sourced. OHUI asks what this combination produces
at the player's current skill. The engine answers with Ordinator's values,
Vokrii's values, or vanilla values depending on what is installed. OHUI
renders the result.

**Soul gem comparison:**
The preview panel shows the enchantment strength for each available soul
gem size side by side. The player sees the actual gain from using a Grand
soul gem versus a Greater soul gem for this specific enchantment. For
some enchantments the difference is significant. For others it is minimal.
The player makes an informed decision about which soul gem to spend.

### Skill-Adjusted Preview

The enchantment preview reflects the player's current skill level. A
player with Enchanting 30 and a player with Enchanting 90 see different
predicted values for the same combination. The screen shows what this
player will produce today, not the theoretical maximum.

Active perk modifiers are listed explicitly — which perks are contributing
to the final value and by how much. The player understands their
enchanting ceiling and what would improve it.

### Item Naming

Vanilla auto-naming for enchanted items is nearly useless in practice.
"Iron Dagger of Minor Flames" repeated thirty times in an inventory is
not thirty identifiable items — it is noise. The name does not reflect
magnitude meaningfully, does not handle dual enchantments gracefully,
and gives the player no way to distinguish items at a glance without
opening each detail panel individually.

OHUI's enchanting screen replaces vanilla auto-naming with a two-part
system: better generation and an optional naming field.

**Better generated names:**

Single enchantment — effect name with a magnitude tier expressed as a
numeral rather than an adjective:

```
Iron Sword of Flames III
```

Numerals (I / II / III / IV / V) over adjectives (Minor / Lesser / Major /
Greater / Grand) because they are consistent, sortable, and immediately
comparable across items. "Flames III" and "Flames IV" communicate a
relationship. "Major Flames" and "Greater Flames" do not.

Dual enchantment — lead with the dominant effect, append the secondary:

```
Iron Sword of Flames III and Frost II
```

The dominant effect is the one with the higher magnitude or the one the
player placed first — configurable in MCM2. The name is longer but it
is accurate. A player browsing their inventory knows what this item does
without opening its detail panel.

**Naming field:**
Before confirming an enchant, a text field is shown pre-populated with
the auto-generated name. The player can:
- Accept it as-is — confirm and move on
- Clear it and type their own name — full custom naming via OSK on
  controller, keyboard on PC
- Edit the generated name — adjust it without starting from scratch

The naming field is never mandatory. A player doing bulk enchanting
accepts the generated name and moves straight to the next item. A player
building a specific named item for their character takes ten seconds to
give it a proper name. The field is present for those who want it and
costs nothing for those who don't.

**Inventory coherence:**
Better auto-naming is most valuable at the inventory level. A collection
of enchanted items with consistent, magnitude-indicating names is
browsable without detail panel diving. "Sword of Flames III" is above
"Sword of Flames II" alphabetically and communicates the relationship
between them without any additional UI work.

---

### Bulk Enchanting

The vanilla enchanting screen imposes a confirmation messagebox on every
single enchant — "Enchant the item?" — and resets the enchantment and soul
gem selection after each one. For a player doing any volume of enchanting
this is a per-item tax that serves no purpose. The alchemy lab has never
had this problem. You brew, the result appears, you brew again.

OHUI's enchanting screen behaves like the alchemy lab:

- No confirmation messagebox. The item is enchanted when the player
  confirms the selection. That action is the confirmation.
- Enchantment selection persists after each enchant. The player does not
  reselect the same enchantment for the fifteenth iron dagger.
- Soul gem selection persists where a matching soul gem is still available.
  If the selected size is exhausted the screen steps down to the next
  available size automatically.

A player enchanting a stock of items for sale, building a set of fortify
gear, or farming enchanting XP can work continuously without friction.
The flow is: select item, enchant, next item, enchant. Nothing in between.

This behaviour is absorbed from Better Bulk Enchanting (mod 133701) by
JerryYOJ. It is first-party OHUI functionality. The mod is not required.

---

### Disenchanting

Selecting an item to disenchant shows which enchantments will be learned
from it. If the player already knows any of those enchantments, this is
indicated — the player knows before disenchanting whether they are
learning anything new or just recovering a soul gem's worth of value.

### Enchanting Planner

A planning mode — separate from the active enchanting interface — where
the player can explore combinations without committing materials. Select
an item type, select enchantments, see the projected results at the
player's current and projected skill levels. Read-only. Nothing is
enchanted until the player exits planning mode and uses the actual
enchanting interface.

The planner is a build planning surface. A player assembling an
enchanting-based build can reason about what they're working toward
without spending materials on experiments. The values are engine-sourced
and therefore reflect whatever enchanting overhaul is installed.

---

## Cooking Screen

### The Constraint

Cooking is the least mechanically complex crafting screen and the most
mod-dependent in terms of recipe volume. CACO alone adds an enormous
number of cooking recipes. The shared QOL — search, filter, sort,
completion indicator — addresses the only real pain point directly.

### Survival Mode Context

For players with survival mode active (vanilla, Sunhelm, Frostfall,
or any registered survival mod), the cooking screen surfaces additional
information about what each recipe will provide:

- Hunger satisfaction value
- Warmth value where applicable
- Duration of food buffs
- Stacking behaviour with other active food effects

This is the same survival mod registration API used by the sleep screen.
The same integration, different surface. Cooking and sleeping are the
two natural places where survival context belongs.

### Ingredient Tracking

Cooking shares the alchemy screen's material awareness feature — knowing
what you have and where it is. For players using SCIE, cooking ingredients
in registered containers are available at the cooking pot without transfer.
For players without SCIE, the player inventory is the scope.

The recipe completion indicator and "almost craftable" filter are
particularly useful for cooking given the recipe volume CACO introduces.

---

## Perk-Activated Station Interactions

Ordinator and similar perk overhauls can inject additional interactions at
crafting stations — perk-unlocked modes, special crafting options, and
station-specific behaviours that vanilla has no concept of. These surface
as additional available actions at the station for players who have the
relevant perks.

OHUI does not implement these interactions. It does not know what they
are, which mod added them, or what they do. That is correct — attempting
to own or predict perk-activated station behaviour would mean competing
with every perk overhaul that defines it.

What OHUI commits to instead is soft support from the start:

**OHUI never closes off the interaction surface at a crafting station.**
Whatever a perk overhaul injects as an available action at a station is
surfaced in the crafting screen. A dedicated affordance in the screen
header or station context area renders perk-activated interactions
consistently alongside first-party screen elements. OHUI does not know
it is Ordinator. It knows there are additional interactions available at
this station for this player and it puts them somewhere visible,
accessible, and visually consistent with everything else.

The affordance is absent when no perk-activated interactions are present.
It appears when they are. Players without Ordinator never see an empty
slot waiting for something. Players with Ordinator see their perk
unlocks surfaced cleanly in a screen that was designed to accommodate them.

This is the same pattern as the sleep screen's extensible character state
warnings and the message log's custom type registration. OHUI defines the
container. The ecosystem fills it.

---

## Mod Compatibility Note

The mechanical fidelity principle means OHUI's crafting screens are
compatible with any crafting overhaul by design. The screen never computes
what the engine computes. It only presents what the engine returns.

Specific known overhauls and their relationship to the UI layer:

**CACO / Apothecary / Alchemy Redone** — modify ingredient effects and
potion formulas below the UI layer. OHUI's alchemy combination preview
reflects their values automatically.

**CCOR / Complete Crafting Overhaul Remastered** — modifies smithing
recipes and tempering formulas below the UI layer. OHUI's smithing
preview reflects their values automatically.

**Ordinator / Vokrii** — modify perk effects on crafting at the engine
level. OHUI's previews reflect their perk contributions automatically.

**SCIE** — operates below the UI layer. OHUI's crafting screens receive
SCIE's pooled inventory transparently. No coordination required.

No patches are required for any of these mods. The architecture makes
patches unnecessary.
