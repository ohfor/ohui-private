# Cosave and Persistence

## Purpose

OHUI persists a significant amount of data across sessions — layout
profiles, widget positions and sizes, outfit definitions, provenance
stamps, MCM control values, message log history, favourites radial
configuration, MCM list ordering, hide and rename overrides, New Game+
legacy data, and any data registered by mods via the persistence API.

None of this belongs in the Skyrim save file. The save file is the
game's domain. OHUI writes a cosave — a companion file alongside the
Skyrim save, owned and managed entirely by OHUI, invisible to the
engine.

---

## The Cosave File

OHUI's cosave file lives alongside the Skyrim save file:

```
Saves/
  Save1_CharacterName_Location.ess        ← Skyrim save
  Save1_CharacterName_Location.ohui       ← OHUI cosave
```

The `.ohui` extension is OHUI's. No other mod writes to it. No other
mod reads from it directly — they read through OHUI's persistence API.

A separate export function produces a human-readable JSON
representation for debugging purposes.

---

## Character Identity

The cosave is per-character. Each character has a stable identity
generated on character creation and stored in the OHUI cosave. This
ID associates cosaves with characters across save files.

A player with multiple save files for the same character — manual
saves, autosaves, quicksaves — gets one set of OHUI data, not one
set per save file. Layout changes made on save 47 are present on
save 46 when loaded. OHUI data is character state, not save-point
state.

**The association rule:** When loading any save, OHUI identifies the
character and finds the most recent cosave for that character. The
cosave does not need to be the one paired with this specific save
file.

**Override:** A player who explicitly wants to roll back OHUI state
along with their Skyrim save can do so from the load game screen —
an option to "load paired cosave" uses the cosave alongside the
specific save file rather than the most recent one for the character.

---

## What the Cosave Stores

The cosave is organised into independent data blocks. Each block is
independently versioned and can evolve without affecting other blocks.
New blocks can be added without changing existing ones.

### Layout Profiles

All named layout profiles for this character. Each profile is a
complete snapshot of the widget canvas — every widget's position,
size, visibility, and anchor state at the time the profile was saved.
The active profile ID is stored separately.

Built-in profiles (Default, Minimal, Controller) are stored alongside
player-defined profiles. They can be overwritten. A "Restore Built-in
Defaults" action in edit mode regenerates them without touching
player-defined profiles.

### Outfit Definitions

All outfit definitions for this character and their followers. An
outfit is a named set of equipment slots with assigned items.

Item references use plugin name plus local form ID to survive load
order changes. On load, OHUI resolves references against the current
load order. Missing plugins are flagged and shown as unresolvable in
the outfit editor.

### MCM Values

Current values for all declarative MCM2 controls. Compatibility layer
MCM values are managed by their respective Papyrus scripts using
existing SkyUI storage — OHUI does not own those.

On load, stored entries are matched against currently registered MCM
definitions. Entries with no matching registration are retained as
orphaned (the mod may be temporarily uninstalled) and pruned after a
configurable number of sessions without a match (default 5).

### Message Log

Message log history for this character up to a configurable maximum
(default 500 entries, configurable to 0 for no persistence or higher).
Each entry stores the full set of fields defined by the message log
system. The log is append-only during a session and pruned to the
maximum on session end. This default is authoritative — the message
log system document defers to this value.

### Favourites Radial

All named radial configurations and the active radial ID. Each
configuration defines the contents of all eight radial segments,
including slot type and assignment per segment.

### MCM List Config

The player's hide, rename, and reorder configuration for the MCM mod
list.

### Input Bindings

The player's custom keybind and controller button assignments per
action context. Only entries that differ from registered defaults are
stored — the binding map is a delta, not a full copy.

Orphaned entries (mod uninstalled) are retained and pruned after N
sessions, matching the MCM orphan pattern.

### New Game+ Legacy

The legacy data package assembled when the player initiates a New
Game+. Present only in cosaves that are the source of a New Game+
or that have inherited legacy data. Contains selected carry-forward
categories, their associated data, and the generated legacy summary
text.

### Mod Data

An extensible key-value store for mod-registered persistence data.
Any mod can store arbitrary data here via the persistence API. OHUI
enforces namespace isolation — a mod cannot read or write another
mod's data. Size limits apply per entry and per mod. Writes that
exceed limits are rejected and logged.

---

## Standalone Read (No Active Session)

Some features need to read cosave data outside of an active game
session — the load game screen reads character portraits and metadata,
and the New Game+ configuration screen reads legacy data from a
source save the player selects.

OHUI provides a read-only access path for this purpose. It opens a
cosave file by path, validates it, and reads specific blocks. This
path is read-only — no writes, no migrations applied, no lifecycle
events fired. It exists to serve the main menu and load game screen.

---

## The Persistence API

OHUI exposes persistence to mods via a simple typed key-value API
accessible from both native and scripted code.

**Supported types:** bool, int, float, string, raw bytes.

**Operations:** set, get (with default fallback), check existence,
delete key, clear all keys for a mod, and versioned reads for mods
that need to migrate their own stored data format.

All reads return the default value if the key does not exist. Reads
never fail. Writes that exceed size limits are rejected and logged —
the mod continues to function, the data is not persisted.

### When to Read and Write

**Read:** After the cosave has finished loading. Mods should not
attempt to read persistence data before OHUI signals that the cosave
is ready.

**Write:** Any time after the cosave is loaded. Writes go to an
in-memory buffer and are flushed to disk on the next save event
(manual save, autosave, quicksave). Writes are not durable until
the next save.

**On new game:** Persistence data does not exist for a new character.
All reads return default values. Mods should initialise their data
explicitly on new game rather than relying on default returns.

---

## Versioning and Migration

### Block Versioning

Each data block has its own version number. When a block's format
evolves, its version increments and a migration is registered for
the previous version. On load, OHUI applies migrations in sequence
to bring old data up to current format. Migrations are pure
transformations with no side effects.

### Mod Data Versioning

Mods that store structured data can version their own format
independently. On read, the mod receives the stored version alongside
the data and is responsible for its own migration logic. OHUI does
not interpret mod data — it provides versioned storage.

### Forward Compatibility

A cosave written by a newer OHUI loaded by an older version is handled
gracefully. Unknown blocks are skipped. Known blocks with unrecognised
fields ignore the extras. No crash, no corruption. A warning is logged.

A player who downgrades OHUI loses access to data written by the
newer version but does not lose existing data. On next save, the
cosave is rewritten in the older format.

---

## Cosave Lifecycle

### On Game Load

OHUI identifies the character from the save, locates the most recent
cosave for that character, validates it, loads all data blocks, applies
any pending migrations, and signals that the cosave is ready. Systems
and mods may then read their data.

### On New Game

OHUI generates a new character identity, creates an empty cosave in
memory, applies built-in defaults (layout profiles, widget states),
and signals readiness. If a New Game+ package is present, legacy data
is applied before the readiness signal.

### On Save

OHUI collects changed data from all systems and writes the cosave
atomically. Atomic write prevents corruption from interrupted saves.

### On Character Delete

When a save is deleted from the load game screen, OHUI offers to
delete the associated cosave. The player can decline — they may want
to keep it for New Game+ purposes. Declined cosaves are retained and
visible as detached cosaves, available as New Game+ sources.

---

## Debug and Diagnostics

### Cosave Inspector

Available in the Maintenance section of OHUI's settings. Shows cosave
file path and size, character identity, block inventory with version
and validity state, per-block entry counts, orphaned mod data entries,
and total data usage by system and by mod.

### Export to JSON

A debug export produces a human-readable JSON representation of the
entire cosave. Useful for diagnosing unexpected state and for bug
reports. Clearly labelled as a developer tool.

### Cosave Repair

If the cosave fails validation on load, OHUI reports the failure to
the player in a non-blocking notification and continues with empty
data. The Skyrim save loads normally. OHUI state resets.

The repair tool in Maintenance attempts to recover partially valid
cosaves — reading valid blocks and discarding corrupted ones. The
player is informed of exactly which blocks were recovered and which
were lost.
