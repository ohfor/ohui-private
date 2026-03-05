# Load Game Screen

## The Problem with a Flat Save List

The current load game screen is a flat list of save files sorted by
date. It answers "which file do I load" but never answers "which
character do I want to play." Those are different questions. The screen
conflates them badly.

A player with three characters and fifty saves per character is
navigating one hundred and fifty undifferentiated entries to find what
they want. Save files are named by the game with a number, a character
name, a location, and a timestamp. The character name is there. The
distinction between characters requires reading every entry.

OHUI replaces this with a two-tier structure. The tiers have different
jobs and different visual treatments. The player answers "who do I want
to play" before they are ever asked "which save do I want to load."

---

## Tier 1 — Character Selection

A full-screen card gallery. One card per character. The player selects
a character before they see a single save file.

### The Character Card

Each card shows:

**Character portrait** — a captured render taken at the last save.
See the Portrait Capture system below for how this is generated and
stored. High quality, consistent framing, neutral pose. The character
as they were when last saved.

**Name and identity** — character name, race, sex, level. The core
facts in large readable type.

**Location** — where the character was when last saved. Not a cell
ID. A human-readable location name — "Whiterun, Dragonsreach" or
"Outside Dustman's Cairn."

**Faction memberships** — the factions the character has joined,
shown as small badges. Companions, College of Winterhold, Thieves
Guild, Dark Brotherhood, Stormcloaks, Imperials. Derived from save
data at load time. A quick visual read of who this character is.

**Active quest** — the character's currently tracked quest. One line.
"The Horn of Jurgen Windcaller" or "Proving Honor." The reminder of
where you left off.

**Playtime and last session** — total playtime for this character and
how long ago they were last played. "84 hours · Last played 2 days
ago."

**Highest skills** — two or three notable skills from the cosave
stats. "Smithing 94, Archery 78." The character's identity in numbers.

```
┌─────────────────────────────────────┐
│                                     │
│   [CHARACTER PORTRAIT]              │
│                                     │
│   Valdris                           │
│   Nord · Level 47                   │
│                                     │
│   Whiterun, Dragonsreach            │
│   Last played 2 days ago · 84h      │
│                                     │
│   [Companions] [College]            │
│                                     │
│   Active: The Horn of Jurgen        │
│           Windcaller                │
│                                     │
│   Smithing 94 · Archery 78          │
│                                     │
└─────────────────────────────────────┘
```

### Gallery Layout

Cards are arranged in a horizontal scroll, sorted by most recently
played. The most recent character is leftmost and selected by default
on load game screen open — pressing confirm immediately proceeds to
that character's save browser. Players who have not played in a while
scan right for an older character.

Two or three cards visible at once depending on screen width. Partial
fourth card visible at the edge signals that more characters exist.
On controller, left/right navigates between cards. On mouse, click
to select, scroll wheel or drag to browse.

A search field at the top filters cards by character name for players
with large character rosters.

**New Character** — a final card at the end of the gallery with a
distinct visual treatment. Not a save file. A prompt to create a new
character. Selecting it exits to the New Game / New Game+ flow.

---

## Tier 2 — Save Browser

Selecting a character card opens the save browser for that character.
Full screen. This character only. The gallery is gone. The browser
owns the screen.

A back button — and the back/cancel input — returns to the character
gallery. Not to the main menu. The player is never stranded.

### Save Entry

Each save entry shows:

**Save type and name** — Manual saves show the player's given name
prominently. "Before Alduin's Wall." Autosaves, quicksaves, and
exit saves show their type and nothing else beyond location.

**Location** — where the save was made. Same human-readable format
as the character card.

**Date and time** — when the save was made. Absolute date and time,
not relative. "March 3 · 14:22." Relative time ("2 hours ago") is
shown in addition for recent saves.

**Level at save** — the character's level when this save was made.
A quick indicator of progression through the save list.

**Active quest** — the tracked quest at save time. The reminder of
what the player was doing at this exact moment.

**Playtime** — cumulative playtime at this save point.

```
Manual Save  "Before Alduin's Wall"
─────────────────────────────────────────────────────────────
Whiterun, Dragonsreach  ·  March 1  21:15  ·  2 hours ago
Level 45  ·  Active: Alduin's Wall  ·  79h 03m

Autosave
─────────────────────────────────────────────────────────────
Outside Dustman's Cairn  ·  March 3  13:47
Level 46  ·  Active: Proving Honor  ·  83h 41m

Quicksave
─────────────────────────────────────────────────────────────
Whiterun, Dragonsreach  ·  March 3  14:22  ·  moments ago
Level 47  ·  Active: The Horn of Jurgen Windcaller  ·  84h 12m
```

Save entries are reverse chronological — most recent at the top.
The most recent save is selected by default. The vast majority of
load game actions load the most recent save. One confirm from the
character card. Efficient.

### Mod List Delta Indicator

If the save was made with a different mod list than the current
active mod list, the entry shows a warning badge. Tapping or hovering
the badge expands a detail view listing which mods are missing from
the current load order and which mods have been added since the save
was made.

Missing mods are listed in order of likely impact — mods that made
significant changes (quest mods, overhauls, SKSE plugins) flagged
more prominently than cosmetic or audio mods.

The indicator is a warning, not a blocker. The player can load any
save regardless of mod list state. The warning is honest information,
not a gate.

### Filters and Search

A filter bar at the top of the save browser:

- **All saves** — default, shows everything
- **Manual only** — hides autosaves, quicksaves, exit saves
- **By location** — groups saves by location, useful for finding
  saves from a specific place
- **By quest** — groups saves by active quest at save time

A search field filters entries by name (for manual saves), location,
or quest name.

These filters exist for players with large save histories. A player
with ten saves never needs them. A player with three hundred saves
needs them constantly.

### Save Management

A context menu on each entry (right-click on mouse, long-press or
context button on controller):

- **Load** — same as selecting the entry
- **Rename** — for manual saves, change the player-given name
- **View details** — expanded view showing full mod list at save time,
  full active effects, character stats snapshot
- **Delete** — with confirmation. Prohibited action via OHUI's own
  UI — OHUI prompts the player and requires an explicit confirmation
  input separate from the standard confirm button, to prevent
  accidental deletion

---

## Portrait Capture

The character portrait displayed on the character card is captured
at save time. Not a screenshot. A dedicated render.

### Capture Process

When Skyrim saves — manual save, autosave, quicksave, exit save —
OHUI triggers a portrait capture in the same C++ viewport used for
the inventory character preview. The capture happens in the background
during the save process, adding negligible time to the save operation.

The viewport renders the character in a neutral pose, centred and
framed from approximately mid-torso upward. Lighting is a standard
three-point setup defined in OHUI's skin — the same lighting used
for the inventory character preview, consistent across all characters
regardless of where in the world the save was made.

The character is rendered exactly as they appear at save time —
their current equipped outfit, their current appearance, their
current body state. Whatever mods have changed about their
appearance is present in the capture. The portrait is faithful
to the character's actual state.

The capture is stored as a compressed texture in the cosave. Size
is approximately 50-80KB per capture at 512×512 resolution. This
is acceptable cosave overhead for the quality of the result.

### One Capture Per Save

Every save generates a fresh portrait capture. The character card
shows the portrait from the most recent save. Each save entry in
the save browser can optionally show its own period-accurate portrait
— how the character looked at that specific save point. This is
available in the "View details" expanded view, not shown inline in
the list (too dense).

### Why Not Live Render

A live render of the character at the main menu would be the wow
version. The character model reconstructed in a viewport from cached
appearance data, rendered in real time with a slow rotation. It would
be spectacular.

It would also be unreliable.

At the main menu no game world is loaded. The character model must
be reconstructed entirely from cached data — race, sex, facial sculpt,
body morphs, equipped items. Every mod that touches character
appearance is a potential failure point. A missing body mesh mod, a
custom race whose assets are not loaded, an appearance overhaul that
stores its data in a format OHUI cannot reconstruct — any of these
produces a broken or incorrect render that misrepresents the character
to the player.

The captured render is taken while the game world is fully loaded,
all mods present, all assets available. It is always correct because
it was rendered under ideal conditions. It does not break when mods
change. It does not break when mods are removed. It is what the
character looked like. Full stop.

**The live render is a post-1.0 ambition.** If the appearance
reconstruction can be made reliable for vanilla characters and the
common mod configurations, it replaces the static captured render
on the character card with a live rotating viewport. The captured
render remains as the fallback — if live reconstruction fails for
any reason, the card silently shows the captured render instead.
The player never sees a broken state.

The captured render ships at 1.0. The live render ships when it
can be made trustworthy. The experience is excellent either way.
The ambition is documented so it is not forgotten.

---

## Character Data Source

The character card data — location, factions, active quest, skills,
playtime — is sourced from two places:

**The Skyrim save file** — read via SKSE at load game screen open.
Location, level, active quest, faction memberships, and skill levels
are all present in the save file and readable without loading the
save into the game world. OHUI reads these fields directly.

**The OHUI cosave** — playtime, last session timestamp, and any
OHUI-specific data (outfit names, etc.) for display on the character
card.

Neither source requires loading the save into the game world. The
load game screen renders entirely from file reads. Character selection
is instant. Save browsing is instant. The game world loads only when
the player confirms a save to load.


