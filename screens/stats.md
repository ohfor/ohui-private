# Stats Screen

## Vanilla Foundation

Skyrim's stats screen lives in the journal — General, Combat, Magic, Stealth,
Crafting, Quests, and Crime tabs. It tracks a fixed set of engine-defined
values: gold earned, people killed, dungeons cleared, potions brewed, items
enchanted, locks picked, locations discovered, and so on.

The data is richer than most players realise because it is buried and
presented as a flat list with no context, no trends, and no comparison to
anything. The numbers exist in complete isolation. 847 gold pickpocketed
means nothing without knowing when, at what skill level, or how that
compares to anything else.

**1.0 scope:** Present the vanilla stats screen cleanly. Same QOL treatment
as every other screen — readable layout, organised tabs, nothing buried.
The engine provides the numbers. OHUI presents them clearly. Nothing more
for launch.

---

## Extensible Stats — Post-1.0

The more interesting question is not how to present the vanilla stats but
how far the underlying data model can be extended.

**The honest answer:** not very, at the vanilla level. Skyrim's stat
tracking is a fixed set of engine-defined values incremented at hardcoded
moments. New vanilla stats cannot be added via Papyrus or ESP. "Potions
brewed" increments because the engine increments it at the alchemy lab.
There is no hook to register a custom stat in that same system.

What can be done independently is richer. SKSE and Papyrus can track
anything at any moment. Mods already do this — CACO tracks recipe
discoveries, LOTD tracks museum progress, survival mods track hunger
events and cold exposure and sleep debt. That data exists. It lives in
each mod's own silo with no unified surface.

**The registration API:**
OHUI's stats screen becomes that unified surface. A registration API
where mods contribute their own tracked values and OHUI presents them
alongside vanilla stats in a coherent, consistent screen.

```
[Vanilla]                [CACO]                  [LOTD]
Potions brewed: 312      Recipes discovered: 47  Relics found: 183
Items enchanted: 89      Meals cooked: 156       Displays filled: 241
Dungeons cleared: 34     Ingredients eaten: 203  Pages read: 67
```

The vanilla numbers are fixed and engine-owned. Everything else is whatever
mods register. OHUI defines the display contract. Mods push data into it.

**Stamp system integration:**
Crafting stats derived from stamp history are richer than anything the
engine natively tracks. "312 potions brewed" is a vanilla stat. "47
potions brewed at Alchemy 80+" is stamp data. "Longest carried item:
Valdris's Iron Sword (crafted Day 3, still held Day 187)" is stamp data.
The stamp system and the extensible stats screen are natural complements
— the stamp system generates data the stats screen can surface.

**Sequencing:**
This feature waits on two things being mature before it gets built:
the stamp system in production, and the mod registration API pattern
proven across sleep screen warnings, message log custom types, and
custom skill trees. The stats registration API is the same concept
applied to a new surface. It does not need to be designed from scratch
— it needs to be built after those foundations are proven.

Shipping 1.0 with a clean vanilla stats presentation and building the
extensible version on top of a proven API is the correct sequencing.
Rushing the registration system into 1.0 means building it against
incomplete foundations.
