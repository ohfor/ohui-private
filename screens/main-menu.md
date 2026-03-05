# Main Menu

## Overview

The main menu is presentation first. OHUI's version looks like OHUI —
same font system, same design tokens, same skin as everything else. No
jarring visual disconnect between the main menu and the game UI. The
skin that governs the in-game UI governs the main menu.

Standard options: Continue, New Game, Load Game, Settings, Credits, Quit.
MCM is not here — it moved to the Tween Menu as a first-class destination.
Settings covers OHUI configuration, display, audio, controls, and gameplay.

Nothing exotic. The main menu is not where complexity lives.

Except for one thing.

---

## New Game+

Skyrim has never had New Game+. No mod has ever implemented it properly
at the UI level with real mechanical meaning behind the choices. OHUI adds
it — as a deliberate option at the New Game prompt, built on infrastructure
that already exists for other purposes.

The New Game button becomes a choice:

```
NEW GAME
NEW GAME+  ─  carry your legacy forward
```

Selecting New Game+ opens a configuration screen before the new save is
created. The player selects what their character carries into the next
life. Everything else resets.

### The Configuration Screen

A selection screen presenting every available carry-forward category.
Each category has a description of exactly what it means in mechanical
terms — not vague promises, concrete statements. Each is independently
toggleable. The player assembles their own New Game+ from the categories
that are meaningful to them.

The screen reads from a completed save — or any save the player selects
as the source. The source save does not need to be a completed playthrough.
It is whatever the player designates as the legacy they are carrying
forward.

### Carry-Forward Categories

**Perk Points** — unspent perk points from the source save carry forward
as a starting pool. Not the perks themselves — the points. The player
rebuilds their character fresh with a head start on choices.

"You will begin with 12 unspent perk points."

**Known Spells** — spells learned in the source playthrough are known
at the start of the new game. Spell tomes still exist in the world —
finding them again yields no spell but sells for gold. The knowledge
is the carry-forward, not a shortcut past the world.

"47 spells will be known from the start."

**Crafting Recipes** — recipes discovered via experimentation, quest
rewards, or reading in the source playthrough are known at the start.
Alchemy combinations, smithing patterns, cooking recipes. The world
still contains the ingredients. The experimentation is complete.

"312 alchemy combinations and 28 smithing recipes will be known."

**Shout Words** — words of power learned in the source playthrough are
known at the start. The words are familiar — the Greybeards note this
immediately. Dragon souls must still be collected to unlock them. The
knowledge carries. The power must be re-earned.

"14 shout words will be known. Dragon souls required to unlock them."

**Map Memory** — locations discovered in the source playthrough appear
on the map as known but uncleared from the start. The character has
been here before. The world remembers that. Fast travel to discovered
locations is available immediately — the character knows how to get there.
Dungeons are still full. Enemies respawn. The map shortcut is the only
carry-forward.

"537 locations will be discovered on your map."

**Lore — Books Read** — books read in the source playthrough are marked
as read. Skill books already consumed yield no further XP. The knowledge
is carried. The benefit was already taken.

"203 books read. 14 skill books already consumed."

**Faction Standing** — reputation with factions from the source
playthrough carries forward. Guild membership does not — quests must
be completed again. But a character who proved themselves to the
Companions is known to the Companions from the start. NPCs react
accordingly. Questlines still play out in full.

"Companion standing, Thieves Guild reputation, and College of Winterhold
standing will carry forward. Quest progress resets."

**Heirloom Items** — provenance-stamped items designated as heirlooms
in the source playthrough appear in a chest at the game's opening
location. The items are physically present in the new world. A weapon
the player crafted and carried through an entire playthrough can be
waiting for them at the start of the next one.

Players designate heirlooms from the inventory screen in the source
save before starting New Game+. Only stamped items are eligible. A
maximum of N items (configurable, suggested default: 5) to prevent
the system from trivialising early game.

"3 heirloom items will be waiting for you."

**LOTD Museum Knowledge** — for Legacy of the Dragonborn players,
the museum's discovery records carry forward. The character knows
what the museum contains and what is still needed. Display states
reset — everything must be found and donated again. The knowledge
of what exists and where to look is the carry-forward.

"183 museum entries known. All displays reset."

### The Legacy Summary

Before confirming the New Game+, the configuration screen presents a
legacy summary — a brief narrative of what the source character
accomplished that will be remembered in the new playthrough:

```
The legacy of Valdris:

A master smith who enchanted 89 items and brewed 312 potions.
Completed the main questline, joined the Companions and the
College of Winterhold. Discovered 537 locations across Skyrim.
Carried the same iron sword from Smithing 42 to Smithing 100.

3 heirlooms designated:
  Valdris's Iron Sword  —  crafted Day 3, carried 187 days
  Valdris's Nightingale Bow  —  enchanted at Enchanting 94
  Amulet of Talos  —  found in Ustengrav, worn for 201 days
```

The legacy summary is generated from stamp data, stats data, and quest
completion records. It is a final look at what is being carried forward
and what is being left behind before the new journey begins.

### Why This Is Not Hard

New Game+ sounds like a large feature. It is not.

There is no new game system to build. There is no mechanical complexity
that does not already exist elsewhere in OHUI's infrastructure. The
implementation is:

1. A configuration screen at the New Game prompt — a selection UI
2. A cosave read from the designated source save — SKSE already does this
3. A set of data writes into the new game's starting state — the same
   data OHUI writes for layout profiles, stamp data, and outfit persistence
4. The new game starts

OHUI already reads and writes cosave data. The stamp system already
has item data APIs. Spells, perk points, faction reputation, and map
discoveries are all readable from a save via SKSE. The hard part of
New Game+ is the UI that makes the selection feel meaningful — and that
is exactly what OHUI builds.

The headlines write themselves. "Skyrim finally gets New Game+." Thirteen
years in. First time anyone has done it properly at the UI level with
a real selection screen and real mechanical meaning behind the choices.
The implementation is a careful application of infrastructure that already
exists for other purposes.

---

## Everything Else

The main menu is otherwise deliberately thin.

**Continue** — loads the most recent save. Nothing exotic.

**Load Game** — the save browser. Sorted by date, searchable by
character name, save thumbnails where available. Mod list delta
indicator — if the save was made with a different mod list than the
current one, the browser flags which mods are missing or added.
Not a blocker. An honest warning before the player commits to loading.

**Settings** — OHUI configuration, display, audio, controls, gameplay.
Clean, organised, nothing buried.

**Credits** — present and accessible. No exotic design.

**Quit to Desktop** — direct. No confirmation dialog for a deliberate
menu action.
