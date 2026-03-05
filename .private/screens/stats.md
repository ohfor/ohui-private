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

**Sequencing:**
Shipping 1.0 with a clean vanilla stats presentation. The extensible
version can be built post-1.0 once the core systems are proven and
the mod integration model is established.
