# Quest Journal

## The Problem

The vanilla quest journal is one of Skyrim's most used and least improved screens. Players
open it every session. It has received no meaningful redesign in thirteen years of ports,
remasters, and anniversary editions. SkyUI never touched it.

The specific failures:

- No filtering or sorting. Quests accumulate in a single undifferentiated list.
- No separation of active and completed quests beyond a toggle that reloads the entire view.
- No proximity or hold awareness. A quest in Whiterun looks identical to one in Solstheim.
- No pinning. Important quests cannot be elevated above noise.
- No dialogue history. Every conversation that advanced a quest is gone the moment it ends.
- No session context. Returning to a save after weeks away provides no orientation.
- No NPC attribution. "Report to Aela" assumes you remember who Aela is.
- Quest items have no connection to the journal entry that explains why you have them.
- Completed quests are a graveyard. No sense of story arc or timeline.

The result is a screen players endure rather than use. OHUI redesigns it properly.

---

## Design Intent

The quest journal should feel like an actual record of the player's story — not a flat
task list. It knows where quests are, how recently they were updated, what was said, and
what has been accomplished. It surfaces the right quest at the right moment and gets out
of the way. It answers the questions players actually have: where was I, who is this
person, why do I have this, what did I decide, what have I done.

---

## Information Architecture

### Quest List (left panel)

Filterable and sortable. Default sort: recently updated first.

**Filter options:**
- Active / Completed / All
- By hold (Whiterun, The Reach, Solstheim, etc.)
- By proximity (closest active quest first, using player world position)
- By quest type (main, faction, side, misc)
- Pinned (shows pinned quests only)
- Text search

**Sort options:**
- Recently updated (default)
- Proximity
- Alphabetical
- Quest type

**Per-quest entry shows:**
- Quest name
- Faction/type indicator
- Hold indicator
- Pin state
- Completion status
- Game date of last update

### Quest Detail (right panel)

Selected quest expands into full detail.

**Active objective** — current stage objective, prominent.

**Objective history** — all completed objectives for this quest, collapsed by default,
expandable. Shows the full arc of what has been done.

**Quest notes** — vanilla journal text for the current stage. Skinnable — can be
presented as a physical journal page, a chronicle entry, or plain text depending on skin.

**Location** — map marker name and hold. Tapping/clicking navigates to the world map
with the quest marker selected.

**NPC attribution** — key NPCs for this quest with last known location. Answers "who
is Aela and where is she" without leaving the journal.

**Quest items** — items in the player's inventory flagged as belonging to this quest,
shown inline. Answers "why do I have this" without opening the inventory. Completed
quests surface any lingering quest items that are no longer needed — flagged clearly
so the player can dispose of them.

**Dialogue history** — see below. The headline feature of this screen.

---

## Dialogue History

Every conversation that advances a quest is available in the journal, attributed to
speaker, in order. The player can scroll back through the full transcript of every
dialogue exchange that touched this quest — from the moment it was accepted to the
current stage.

This is surfaced as a collapsible section in the quest detail panel. Collapsed by
default to keep the primary objective information prominent. Expanded on demand.

**Per-conversation entry:**
- NPC name and relationship to quest (quest giver, key NPC, witness, etc.)
- Location where the conversation took place
- Game date and time
- Full dialogue transcript — player choices and NPC responses, in order

**Presentation:**
- Chronological, oldest first within each conversation
- Player lines visually distinguished from NPC lines
- Multiple conversations listed in chronological order
- Skinnable — transcript format is a data contract, visual presentation is skin's concern

### Scope and Capture

Dialogue history works on every save, every character, every quest — including saves
started before OHUI was installed, and including mod quests without any author work
required. No new playthrough required. This is a hard requirement, not a best-effort
feature.

OHUI uses two complementary mechanisms that together provide complete coverage across
all quests in the load order:

**Stage-implied dialogue** (primary mechanism for vanilla and pre-analysed quests)
Skyrim's dialogue trees are finite and fully known. Every quest has a defined set of
conversations that must have occurred for the player to be at any given stage. OHUI
ships with pre-mined dialogue data for all vanilla quests — every branch, every
response, every stage transition. When the player opens the quest journal, OHUI reads
their current quest stage and presents all dialogue that must have led to that point.
This works retroactively on any save. The game state tells OHUI where the player is.
The dialogue data tells OHUI what conversations that implies.

This data was established as viable in prior ESMA research. The mining approach and
data format carry forward directly.

**Background compilation for mod quests**
OHUI does not limit stage-implied dialogue to vanilla content. On game load, OHUI
analyses the dialogue trees of all quests present in the current load order — vanilla,
DLC, and mods — and builds a stage-implied dialogue map dynamically. This compilation
runs in the background, is cached after the first analysis, and is invalidated and
rebuilt when the load order changes. A player who installs a new quest mod loads the
game, OHUI quietly maps the quest's dialogue tree, and by the time the player starts
that quest the full stage-implied history is ready. No mod author work required.

**Runtime capture** (supplementary mechanism for exact fidelity)
When OHUI is present during a conversation, it records the exact choices made and
responses received. This layers on top of stage-implied dialogue to provide exact
choice fidelity — which branch the player actually took, word for word. For saves
where OHUI was present from the start, runtime capture provides complete precision.
For existing saves, stage-implied dialogue fills the gap seamlessly.

The two mechanisms are invisible to the player. They see dialogue history. Whether
any given entry came from stage inference, background compilation, or runtime capture
is an implementation detail, not a user-facing concept.

**Mod author opt-in**
Mod authors who want to ship pre-mined dialogue data alongside their mod — bypassing
the background compilation step and guaranteeing coverage before the player ever loads
the game — can do so via a simple data format. This is an opt-in enhancement, not a
requirement. OHUI handles every mod quest automatically regardless.

**Storage:** Runtime capture data is stored in OHUI's cosave. Stage-implied and
background-compiled dialogue is derived at runtime and requires no save storage beyond
the game's own quest stage data. History is per-character.

---

## Session Context

Players return to a save after days or weeks away with no orientation. The current
objective tells them what to do next but not where they were or what they were in the
middle of. The quest journal surfaces a **last session summary** at the top of the
active quest list:

- Last played date (real time)
- Player location at last save
- Quests that were updated in the last session
- Objectives completed in the last session
- Items picked up in the last session that are flagged as quest items

This is not a tutorial feature. It is a practical tool for the way people actually
play Skyrim — in sessions separated by real life, returning to a story mid-thread.

---

## Completed Quests Timeline

Completed quests are not a graveyard. They are the player's story.

OHUI presents completed quests as a **chronicle** — a chronological record of
everything the player has accomplished, with game dates. Not just a list of quest
names but a timeline of the playthrough:

- "14th of Hearthfire — Became Thane of Whiterun"
- "3rd of Frostfall — Completed The Companions: Glory of the Dead"
- "21st of Sun's Dusk — Retrieved the Amulet of Kings"

Skinnable — the chronicle can be presented as a physical journal, a carved stone
record, a plain log, or anything else. The data contract is dates and quest names.
The presentation is the skin's concern.

**Consequence markers** — where a quest choice closed other questlines or had
meaningful downstream effects, the chronicle notes it. Not as a judgment — as a
record. "Siding with the Imperials in the Civil War made these questlines unavailable."
The player chose. The journal remembers.

---

## Hold Overview

A hold-based view of quest coverage — what has been done in each hold, and what
known quests are available but not yet started. Not a walkthrough or a checklist.
Enough to avoid leaving an entire questline undiscovered in a hold the player has
already visited extensively.

Presented per hold. Collapsed by default. The player opens Whiterun and sees their
completed and active quests there, with an indicator if there are known available
quests they haven't picked up.

---

## Mod Quest Support

All features described in this document apply to mod quests without author work:

- Background dialogue compilation covers all quests in the load order automatically
- Runtime capture works for any conversation regardless of quest origin
- Session context, NPC attribution, and quest items work from game data, not
  quest-specific registration
- The chronicle includes mod quests on the same timeline as vanilla quests

Mod authors who want enhanced coverage — pre-mined dialogue data, custom NPC
relationship labels, custom quest type categorisation — can provide it via a simple
data registration format. OHUI uses it when present. Falls back to automatic
coverage when not.

---

## Player Experience Goals

The quest journal is one of four surfaces explicitly identified as high-impact wow
moments for OHUI's launch. The others are accessibility, item comparison (inventory),
and the dialogue screen.

**Dialogue history** is the headline moment. No Skyrim UI mod, no vanilla version,
no remaster has ever provided it. It works on every save from day one — existing
saves, mod quests, all of it. The first time a player opens a quest they started
months ago and reads back the conversation that started it, that is the moment.

**The chronicle** is the secondary moment. Skyrim players invest hundreds of hours
in their characters. Seeing that investment reflected as an actual story — dated,
ordered, consequential — is something no other UI has offered.

**Session context** is the most practically valuable feature for returning players.
It is noticed immediately and every time.

---

## Open Questions

1. **Misc quests:** Misc objectives accumulate in a single bucket in vanilla. Does
   OHUI treat each misc objective as its own addressable quest with its own dialogue
   history? Probably yes, but the data model needs thought.

2. **Radiant quests:** Radiant quests repeat with different targets. Does dialogue
   history show all instances or only the current one? All instances is more complete
   but potentially very long for heavily repeated radiant quests.

3. **Background compilation performance:** The dialogue tree analysis on load needs
   to be genuinely background — zero impact on load time from the player's perspective.
   Cache invalidation strategy when load order changes needs design.

4. **Conversation log (non-quest):** Is there appetite for a separate screen showing
   all captured dialogue, not just quest-linked? Not a launch question but worth noting.

5. **Failed quests:** Quests that were failed rather than completed. Separate
   filter state? Same as completed? Vanilla doesn't distinguish well.
