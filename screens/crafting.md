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
computes it.

---

## Active Effect Timers

For players using Fortify Smithing, Fortify Enchanting, or Fortify Alchemy
potions, the crafting screen surfaces a dedicated timer strip showing any
active effects that influence results at the current station.

The strip is visible only when relevant effects are active. Players who
do not use fortify potions never see it.

Each entry shows the effect name, the current magnitude, the remaining
duration as a countdown, and a draining progress bar. The bar shifts
colour as time runs low — neutral at full duration, amber under 30
seconds, red under 10.

**Station relevance filtering:**
Only effects that influence results at the current station are shown.
Fortify Smithing does not appear at the alchemy lab.

**Live output preview coupling:**
The output previews — enchantment strength, tempering result — update
in real time as active effects tick down. The player watches the predicted
result change as the buff expires.

---

## Shared QOL — All Crafting Screens

### Recipe Search and Filtering

**Text search** — same pattern as inventory and magic screens. Type to
filter the recipe list in real time. Searches recipe name.

**Filters:**
- Can craft now (all materials available) — default on
- By category where categories exist

**Sort:**
- Default (game order, respects overhaul categorisation)
- Alphabetical
- Recently added

The filter and sort state persists per station type.

---

## Smithing Screen

### Crafting Mode

Recipe list with search, filter, and sort.

**Output preview:**
Selecting a recipe shows the finished item's stats in the detail panel —
at the player's current Smithing skill level with relevant perks applied.
Not the base item stats. The stats the player will actually receive.

### Tempering Mode

**Upgrade preview:**
Before committing materials, the player sees exactly what the tempered
item's stats will be at the next quality tier. The preview is engine-
sourced — CCOR's formulas, Ordinator's perk effects, any overhaul's
tempering calculations are reflected automatically because the engine
has already applied them.

**Quality tier roadmap:**
Shows the full progression available to this player for this item — what
quality is achievable now, what requires more skill, what requires perks
the player doesn't have yet.

**Perk requirement transparency:**
If a higher quality tier is locked behind a perk the player doesn't have,
the screen says which perk.

---

## Alchemy Screen

### The Constraint

OHUI does not touch the effect discovery system. What effects are known,
how they become known, what happens when the player eats an ingredient —
none of that is OHUI's concern. OHUI presents whatever the engine reports
about known effects.

### Ingredient Detail

**Known effects** — displayed clearly with magnitude and duration where
applicable.

**Unknown effects** — shown as undiscovered slots. The player knows an
ingredient has four effects and can see how many they've discovered.

**Effect matching indicator:**
When an ingredient is selected, other ingredients in the list that share
at least one known effect with it are highlighted. Based only on known
effects — undiscovered effects do not match. The discovery system is
not circumvented.

---

## Enchanting Screen

### Enchantment Strength Preview

The headline feature. Selecting an enchantment and a soul gem shows the
resulting enchantment magnitude before committing either. The values
are engine-sourced — Ordinator's values, Vokrii's values, or vanilla
values depending on what is installed.

**Soul gem comparison:**
The preview panel shows the enchantment strength for each available soul
gem size side by side.

### Skill-Adjusted Preview

The enchantment preview reflects the player's current skill level.
Active perk modifiers are listed explicitly.

### Item Naming

OHUI's enchanting screen replaces vanilla auto-naming with a two-part
system: better generation and an optional naming field.

**Better generated names:**
Single enchantment — effect name with a numeral tier (I / II / III / IV / V).
Dual enchantment — lead with dominant effect, append secondary.

**Naming field:**
Before confirming an enchant, a text field is shown pre-populated with
the auto-generated name. The player can accept, clear and type custom,
or edit the generated name. OSK on controller, keyboard on PC.

### Bulk Enchanting

No confirmation messagebox per enchant. Enchantment selection persists
after each enchant. Soul gem selection persists where a matching gem is
still available. Absorbed from Better Bulk Enchanting (mod 133701).

### Disenchanting

Selecting an item to disenchant shows which enchantments will be learned.
Already-known enchantments are indicated.

---

## Cooking Screen

### The Constraint

Cooking is the least mechanically complex crafting screen and the most
mod-dependent in terms of recipe volume. The shared QOL — search, filter,
sort — addresses the main pain point.

### Survival Mode Context

For players with survival mode active, the cooking screen surfaces
additional information about what each recipe will provide — hunger
satisfaction, warmth value, duration, stacking behaviour. Same survival
integration approach as the sleep screen.

---

## Perk-Activated Station Interactions

Ordinator and similar perk overhauls can inject additional interactions
at crafting stations. OHUI does not implement these. It does not know
what they are or which mod added them.

OHUI never closes off the interaction surface at a crafting station.
Whatever a perk overhaul injects is surfaced in the crafting screen
consistently alongside first-party elements. The affordance is absent
when no perk-activated interactions are present.

---

## Mod Compatibility

The mechanical fidelity principle means OHUI's crafting screens are
compatible with any crafting overhaul by design. The screen never
computes what the engine computes. It only presents what the engine
returns. No patches required.
