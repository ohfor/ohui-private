# Magic, Spells, and Shouts Screen

## Shared Foundation

The Magic screen uses the same `FacetedList` pattern as the inventory screen — a
searchable, filterable list, a detail panel, and action buttons. The data is spell
and power data rather than item data. The component is the same.

All inventory screen features that apply to lists apply here: text search, extended
sorting, facet-based filtering, detail panel depth. The Magic screen is not a separate
design problem — it is the same component configured for spells and powers.

Spell schools are facets, not tabs. Multi-select is supported — a player can filter
to Destruction and Restoration simultaneously. The PresetBar provides quick-tap
school presets for the common single-school browsing case. This replaces the earlier
TabbedListPanel approach.

---

## The Problem

The Magic screen is the most information-sparse screen in Skyrim relative to the
decisions it asks the player to make. Equipping a spell is a meaningful choice that
affects combat strategy, magicka economy, and build viability. The screen provides
almost no information to support that choice:

- **Spell descriptions are shallow.** Base damage or heal amount. Duration where
  applicable. Nothing about how the spell scales with skill. Nothing about magicka
  cost at the player's current skill level versus the displayed base cost.
- **No spell comparison.** Which Flames variant deals more damage per magicka spent
  at the player's current Destruction skill? The screen does not say.
- **No search.** A heavily modded game with spell packs can have hundreds of spells.
  Finding a specific one requires scrolling through every spell in a school.
- **Shouts and powers are separate but not well distinguished.** The tab structure
  separates them but provides no contextual information about shout cooldowns or
  power recharge conditions beyond what vanilla provides.
- **Favourite spells are invisible in the Magic screen.** There is no way to see
  the player's favourited spells as a curated subset from within the screen.
- **Mod-added spell schools have no home.** Mods that add custom spell schools
  (Arcanum, Mysticism, others) get no dedicated tab and no special treatment.

---

## Spell Detail Panel

The detail panel for a selected spell is substantially richer than vanilla's
description text.

**Damage and healing spells:**
- Base magnitude at current skill level (not just the base game value)
- Projected magnitude with relevant perks applied
- Magicka cost at current skill level (base cost modified by skill)
- Cost per second for concentration spells
- Duration where applicable
- Area of effect where applicable
- Spell school and skill level required to cast without stagger
- Dual cast behaviour and cost

**Magicka cost in context:**
The magicka cost shown is the cost the player will actually pay, not the base cost.
It reflects the player's current skill level and relevant perks. If the player's
Destruction is 40 and the spell costs 80 magicka at that level, the screen says 80.
If they reach 80 Destruction and the cost drops to 55, the screen says 55.

This distinction matters. The vanilla screen shows a cost that may have no relation
to what the player will actually spend, making magicka economy planning impossible
without external tools.

**Ward and armor spells:**
- Absorb amount for wards
- Armour rating provided
- Duration
- Magicka per second for sustained spells

**Conjuration:**
- Summon duration
- Bound weapon stats where applicable (compared to similar physical weapons)
- Whether the conjured entity scales with Conjuration skill

**Illusion:**
- Target level cap (above which the spell fails)
- Target level cap at current Illusion skill level
- Whether perks extend the cap

**Restoration:**
- Heal amount at current skill level
- Heal per second for sustained spells
- Undead turn/rout level cap at current skill level

**Source attribution:**
Which mod added this spell. Same on-demand attribution principle as dialogue
options and perk sources. Not shown by default. Available on demand.

---

## Spell Comparison

Any two spells from the same school can be compared side by side. The comparison
surfaces the stats that matter for choosing between them:

- Damage/heal at current skill level
- Magicka cost at current skill level
- Cost efficiency (damage or heal per magicka)
- Duration and area differences
- Dual cast comparison

The comparison is not automatic on hover — spell comparison requires deliberate
activation (a comparison mode toggle or a hold-to-compare binding) because most
spell browsing does not require comparison. It is available when the player wants
it and absent when they don't.

---

## Shouts

Shouts are a distinct category from spells and powers with their own information
needs. The shout detail panel surfaces:

**Words of Power:**
- Which words the player has unlocked
- Which words have been learned (unlocked but not yet activated)
- Word count required for each tier (1, 2, or 3 words)
- Whether dragon souls are needed to unlock remaining words

**Cooldown:**
- Current cooldown state (active cooldown shown as a timer, not just a bar)
- Full cooldown duration at 1, 2, and 3 words
- Active cooldown reduction effects (from perks, blessings, or Standing Stones)

**Shout power at each word count:**
Many shouts have meaningfully different effects at 1, 2, and 3 words. The detail
panel shows what the shout does at each tier, not just the full shout description.
The player knows what they get from a 1-word use versus waiting for full charge.

**Lore text:**
The Dragon Language text for the shout's words, with translation. Present but
collapsible — available for players who engage with the lore, unobtrusive for
those who don't.

---

## Powers and Abilities

Powers (Greater Powers and Lesser Powers) and passive abilities have their own
tab, distinct from active spells. The detail panel for powers covers:

**Greater Powers:**
- Effect description
- Once-per-day limitation clearly stated
- Whether the power has been used today and when it will refresh
- For racial powers: the race it belongs to (relevant for mods that grant
  cross-race powers)

**Lesser Powers:**
- Effect description
- No use limit, but magicka cost where applicable

**Passive Abilities:**
- What the ability does (passives often have descriptions that are never surfaced
  in vanilla because they're never selected)
- Source (race, standing stone, quest reward, mod)

**Standing Stone powers:**
- Currently active Standing Stone highlighted
- What the stone's bonus provides at current level where applicable

---

## Custom Spell Schools

Mods that add custom spell schools (Arcanum, Mysticism, Elemental Destruction Magic,
and many others) register their schools with OHUI and receive a dedicated tab in the
Magic screen. The tab uses the school's name and icon from the mod registration.

No mod author work is required beyond standard school registration if the school
uses standard Skyrim spell school mechanics. Custom school mechanics that extend
beyond the vanilla system benefit from providing additional detail panel data via
OHUI's spell data API.

Custom schools participate in all Magic screen features — search, sort, filter,
comparison — automatically.

---

## Search, Sort, and Filter

**Text search** — same as inventory. Type to filter the spell list in real time.
Searches spell name and description. Partial matches included.

**Extended sorting:**
- Name (A-Z)
- Magicka cost (current, accounting for skill)
- Magnitude (damage/heal, current skill level)
- Cost efficiency (magnitude per magicka)
- School
- Recently learned

**Extended filtering:**
- By school (multi-select — show Destruction and Restoration only)
- Favourited spells only
- Spells meeting a minimum skill level (hide spells above current ability)
- Mod-added only
- Vanilla only

---

## Favourites Integration

The Magic screen's favourites integrate with the HUD action bar and the equipment
quick-slot system. Favouriting a spell from the Magic screen adds it to the pool
available for quick-slot assignment. The Magic screen shows which spells are
currently assigned to quick slots alongside their favourite status.

The favourites tab in the Magic screen shows only the player's curated spell list
— their combat loadout, their utility spells, their rotation. For players with
large spell libraries, this is the tab they spend most time in.

---

## Equipped Spell Indicator

The detail panel clearly indicates whether a spell is currently equipped in the
left hand, right hand, both hands, or not equipped. Equipping from the Magic screen
follows the same pattern as vanilla — direction selection for left or right hand —
but the current equipment state is visible before the decision.

For players using iEquip or a similar equipment management mod, the Magic screen
is aware of equipped spell state from those systems via the viewport floor or
native integration.

---

## Player Experience Goals

The Magic screen's headline improvement is **magicka cost in context**. Showing
the actual cost the player will pay — at their skill level, with their perks — rather
than the base game cost is a fundamental correction. It turns the Magic screen from
a catalogue into a planning tool. Players who have never understood why their spells
cost less than advertised will finally see the number they actually pay.

**Shout tier detail** is the feature Shout-focused players will notice immediately.
"What does Unrelenting Force do at 1 word versus 3?" is a question every player has
had and the game has never answered inside the menu. The answer is already in the
game data. OHUI surfaces it where the question is being asked.

**Custom spell school tabs** are the ecosystem wow moment. Spell pack authors whose
schools have been second-class citizens — lumped into a catch-all or spread across
vanilla tabs — get proper representation in the screen. Their users see a Magic
screen that knows their mod list.

---

## Open Questions

1. **Spell tome display:** Should spell tomes in the inventory link to the spell
   in the Magic screen? A player browsing spells they own as tomes but haven't
   read might benefit from seeing the tome in the inventory detail panel when
   viewing the spell they'll learn from it. Cross-screen linking is a feature
   the shared component foundation makes feasible.

2. **Spell crafting (Phenderix, Spell Forge, etc.):** Mods that allow spell
   crafting add a creation flow that belongs somewhere adjacent to the Magic
   screen. Whether this surfaces as a tab, a button that navigates to a crafting
   screen, or a dedicated sub-panel needs design if it is in scope.

3. **Concentration spell preview:** For continuous damage spells, showing a DPS
   figure (magnitude per second, sustained) alongside the per-cast magnitude
   would be useful. The calculation is straightforward. Whether it adds clarity
   or noise to the detail panel is a design question.

4. **Spell conflicts:** Some magic mods have spells that conflict or override
   vanilla spells. Surfacing this — "this spell replaces vanilla Flames" — may
   be useful for players curating their spell list. Low priority but worth noting.
