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

This document defines the cosave format, the read/write contract,
versioning, migration, and the API surface available to OHUI systems
and to mods.

---

## The Cosave File

OHUI's cosave file lives alongside the Skyrim save file with a
predictable naming convention:

```
Saves/
  Save1_CharacterName_Location.ess        ← Skyrim save
  Save1_CharacterName_Location.ohui       ← OHUI cosave
  Save2_CharacterName_Location.ess
  Save2_CharacterName_Location.ohui
```

The `.ohui` extension is OHUI's. No other mod writes to it. No other
mod reads from it directly — they read through OHUI's persistence API.

The cosave is a binary file with a structured header and typed data
blocks. It is not human-readable by design — it is not a config file,
it is a save state. The format prioritises read/write performance and
forward/backward compatibility over human readability. A separate
export function produces a human-readable JSON representation for
debugging purposes.

### File Structure

```
[Header]
  magic:       "OHUI"  (4 bytes)
  version:     uint32  (cosave format version)
  timestamp:   uint64  (unix timestamp of last write)
  characterId: uint64  (unique character identifier)
  blockCount:  uint32  (number of data blocks)

[Block Directory]
  For each block:
    blockId:   uint32  (block type identifier)
    offset:    uint64  (byte offset from file start)
    size:      uint32  (block size in bytes)
    version:   uint32  (block format version)
    checksum:  uint32  (CRC32 of block data)

[Data Blocks]
  [Block: Layout Profiles]
  [Block: Widget States]
  [Block: Outfit Definitions]
  [Block: Provenance Stamps]
  [Block: MCM Values]
  [Block: Message Log]
  [Block: Favourites Radial]
  [Block: MCM List Config]
  [Block: New Game+ Legacy]
  [Block: Mod Data]     ← extensible block for mod-registered data
```

Each block is independently versioned. A block can be updated without
changing the overall cosave format version. A block can be added without
changing any existing block. The block directory allows OHUI to locate
any block by ID without reading the entire file.

---

## Character Identity

The cosave is per-character. Each character has a stable `characterId`
generated on character creation and stored in both the Skyrim save and
the OHUI cosave. This ID is used to associate cosaves with characters
across save files.

**Why this matters:** A player with multiple save files for the same
character — manual saves, autosaves, quicksaves — gets one set of
OHUI data, not one set per save file. Layout changes made on save 47
are present on save 46 when loaded. The OHUI data is character state,
not save-point state.

**The association rule:** When loading any save, OHUI reads the
`characterId` from the Skyrim save, finds the most recent `.ohui`
file with that `characterId`, and loads it. The cosave does not need
to be the one paired with this specific `.ess` file.

**Override:** A player who explicitly wants to roll back OHUI state
along with their Skyrim save can do so from the load game screen —
an option to "load paired cosave" uses the `.ohui` file alongside
the specific `.ess` file rather than the most recent one for this
character.

---

## Data Blocks

### Layout Profiles Block

Stores all named layout profiles for this character. Each profile is
a complete snapshot of the widget canvas — every widget's position,
size, and visibility state at the time the profile was saved.

```
[Profile]
  id:          string   (stable identifier)
  displayName: string   (player-defined name)
  timestamp:   uint64   (when last saved)
  widgetCount: uint32
  [Widget States]
    widgetId:  string   (matches WidgetManifest.id)
    posX:      float    (canvas X in 1920×1080 space)
    posY:      float    (canvas Y)
    width:     float
    height:    float
    visible:   bool
    anchorX:   uint8    (0=left, 1=center, 2=right)
    anchorY:   uint8    (0=top, 1=center, 2=bottom)
```

The active profile ID is stored separately — a single string
identifying which profile is currently applied to the canvas.

Built-in profiles (Default, Minimal, Controller) are stored here
alongside player-defined profiles. They can be overwritten by the
player. A "Restore Built-in Defaults" action in edit mode regenerates
them from OHUI's compiled defaults without touching player-defined
profiles.

### Outfit Definitions Block

Stores all outfit definitions for this character and their follower
outfits. An outfit definition is a named set of equipment slots with
assigned item form IDs.

```
[Outfit]
  id:               string
  displayName:      string
  label:            string   (Combat, Stealth, Home, Social, custom)
  showInRadial:     bool
  subjectType:      uint8    (0=player, 1=follower)
  subjectFormId:    uint32   (follower form ID if subjectType=1)
  slotCount:        uint32
  [Slots]
    slot:           uint8    (equipment slot index)
    itemFormId:     uint32
    itemPluginName: string   (for cross-plugin form ID stability)
  thumbnail:        bytes    (cached portrait render, optional)
```

Item references use plugin name + local form ID rather than the raw
form ID to survive load order changes. On load, OHUI resolves the
reference against the current load order. If the item's plugin is
not present, the slot is flagged as unresolvable and shown as missing
in the outfit editor.

### MCM Values Block

Stores current values for all declarative MCM controls registered
via the native MCM2 API. Compatibility layer MCM values are managed
by their respective Papyrus scripts using existing SkyUI storage
mechanisms — OHUI does not own those.

```
[MCMEntry]
  mcmId:      string   (com.mymod.settings)
  controlId:  string   (enableFeature)
  valueType:  uint8    (0=bool, 1=float, 2=string, 3=int)
  value:      bytes    (type-appropriate encoding)
```

On load, OHUI matches stored entries against currently registered
MCM definitions. Entries with no matching current registration are
retained in the block (the mod may be temporarily uninstalled) but
flagged as orphaned. Orphaned entries are pruned after N sessions
without a matching registration — configurable, default 5 sessions.

### Message Log Block

Stores message log history for this character. The full log is stored
up to a configurable maximum entry count (default 500, configurable
to 0 for no persistence or unlimited).

Each entry stores the message text, type, timestamp, source mod ID,
and read state. The log is append-only during a session. On session
end the current session's entries are merged into the persisted log
and the oldest entries pruned to the maximum.

### Favourites Radial Block

Stores all named radial configurations and the active radial ID.

```
[Radial]
  id:           string
  displayName:  string
  segmentCount: uint8   (always 8, reserved slots are inactive)
  [Segments]
    index:      uint8
    active:     bool
    slotType:   uint8   (0=outfit, 1=potion, 2=spell, 3=shout, 4=item)
    [SlotData]  (varies by slotType)
```

### MCM List Config Block

Stores the player's hide, rename, and reorder configuration for the
MCM mod list.

```
[MCMListConfig]
  entryCount: uint32
  [Entries]
    modId:       string   (the MCM's registered mod ID)
    hidden:      bool
    displayName: string   (empty = use mod's registered display name)
    sortOrder:   int32    (manual sort position, -1 = unset)
```

### New Game+ Legacy Block

Stores the legacy data package assembled when the player initiates
a New Game+. Present only in saves that are the source of a New Game+
configuration or that have inherited legacy data.

```
[NGPlusLegacy]
  sourceCharacterId: uint64
  sourceCharName:    string
  packageTimestamp:  uint64
  [SelectedCategories]
    category:   uint8   (bitmask of selected carry-forward types)
  [PerkPoints]
    count:      uint32
  [KnownSpells]
    count:      uint32
    formIds[]:  (plugin + localId pairs)
  [HeirloomItems]
    count:      uint32
    [Items]     (full item data including stamp history)
  [MapDiscoveries]
    count:      uint32
    formIds[]:  (location form IDs)
  [LegacySummary]
    text:       string  (generated narrative, stored for display)
```

### Mod Data Block

An extensible key-value store for mod-registered persistence data.
Any mod can write arbitrary data here via the persistence API. The
block is a flat namespace of typed entries keyed by a compound key
of mod ID and data key.

```
[ModDataEntry]
  modId:    string
  key:      string
  version:  uint32   (mod-defined data version for migration)
  size:     uint32
  data:     bytes
```

Mods read and write their own data only. OHUI enforces namespace
isolation — a mod cannot read or write another mod's data. Size
limits apply per entry (max 64KB) and per mod (max 1MB total).
Mods that exceed their limit are logged at warning level and the
write is rejected.

---

## The Persistence API

OHUI exposes persistence to mods via a simple typed key-value API.
Both C++ and Papyrus surfaces are provided.

### C++ API

```cpp
// Write
OHUI::Persist::SetBool  ("com.mymod", "key", value);
OHUI::Persist::SetInt   ("com.mymod", "key", value);
OHUI::Persist::SetFloat ("com.mymod", "key", value);
OHUI::Persist::SetString("com.mymod", "key", value);
OHUI::Persist::SetBytes ("com.mymod", "key", data, size);

// Read
bool        OHUI::Persist::GetBool  ("com.mymod", "key", defaultValue);
int32_t     OHUI::Persist::GetInt   ("com.mymod", "key", defaultValue);
float       OHUI::Persist::GetFloat ("com.mymod", "key", defaultValue);
std::string OHUI::Persist::GetString("com.mymod", "key", defaultValue);
bool        OHUI::Persist::GetBytes ("com.mymod", "key", outBuffer, outSize);

// Existence and management
bool OHUI::Persist::Has   ("com.mymod", "key");
void OHUI::Persist::Delete("com.mymod", "key");
void OHUI::Persist::Clear ("com.mymod");  // delete all keys for this mod
```

### Papyrus API

```papyrus
OHUI_Persist.SetBool  ("com.mymod", "key", value)
OHUI_Persist.SetInt   ("com.mymod", "key", value)
OHUI_Persist.SetFloat ("com.mymod", "key", value)
OHUI_Persist.SetString("com.mymod", "key", value)

Bool   OHUI_Persist.GetBool  ("com.mymod", "key", defaultValue)
Int    OHUI_Persist.GetInt   ("com.mymod", "key", defaultValue)
Float  OHUI_Persist.GetFloat ("com.mymod", "key", defaultValue)
String OHUI_Persist.GetString("com.mymod", "key", defaultValue)

Bool OHUI_Persist.Has   ("com.mymod", "key")
     OHUI_Persist.Delete("com.mymod", "key")
```

All reads return the default value if the key does not exist. Reads
never throw. Writes that exceed size limits are rejected silently and
logged at warning level — the mod continues to function, the data
is not persisted.

### When to Read and Write

**Read:** On `kDataLoaded` (C++) or `OnPlayerLoadGame` (Papyrus).
Data is available immediately after the cosave is loaded. Do not
read persistence data before this event — the cosave may not be
loaded yet.

**Write:** Any time after `kDataLoaded`. Writes go to an in-memory
write buffer. The buffer is flushed to disk when OHUI saves the
cosave — on Skyrim save, on autosave, on quicksave. Do not assume
writes are durable until the next save event.

**On new game:** Persistence data does not exist for a new character.
All reads return default values. The first write creates the entry.
Mods should initialise their data explicitly on new game rather
than relying on default returns — use `OnNewGame` or check
`OHUI_Persist.Has()` and initialise if false.

---

## Versioning and Migration

### Cosave Format Versioning

The top-level cosave format version increments when the file structure
changes — the header format, the block directory format, or the block
ID assignments. This has been version 1 since OHUI launched and is
expected to remain so indefinitely. Block-level versioning handles
the evolution of individual data types.

### Block Versioning

Each block has its own version number. When a block's data format
changes, its version increments and a migration function is registered
for the previous version. On load, OHUI reads the block version, finds
the migration chain from that version to current, and applies migrations
in sequence before the block data is used.

```cpp
// Migration registration (internal to OHUI)
OHUI::Cosave::RegisterMigration(
    BlockId::OutfitDefinitions,
    fromVersion: 1,
    toVersion:   2,
    migrate: [](BlockData& data) {
        // v1 outfits did not have the showInRadial field
        // add it with default value false for all existing outfits
        for (auto& outfit : data.outfits) {
            outfit.showInRadial = false;
        }
    }
);
```

Migrations are pure functions. They take old data and produce new
data. They have no side effects. They are unit-testable in isolation.

### Mod Data Versioning

Mods that store structured data in the Mod Data block can version
their own data independently. The `version` field in each mod data
entry is mod-defined. On read, the mod receives both the stored version
and the data bytes, and is responsible for migrating its own format:

```cpp
OHUI::Persist::GetVersionedBytes(
    "com.mymod", "mydata",
    [](uint32_t storedVersion, const bytes& data) -> ModData {
        if (storedVersion == 1) return migrateV1(data);
        if (storedVersion == 2) return migrateV2(data);
        return deserializeCurrent(data);
    }
);
```

OHUI does not know what the data means. The mod knows. The mod
provides the migration logic. OHUI provides the versioned storage.

### Forward Compatibility

A cosave written by a newer version of OHUI loaded by an older version
is handled gracefully. Unknown blocks are skipped — the block directory
allows OHUI to seek past blocks it does not recognise. Known blocks with
a higher version than the current OHUI supports are read with the
current parser — fields beyond what the current version understands
are ignored. No crash. No corruption. A warning is logged noting the
version mismatch.

A player who downgrades OHUI loses access to data written by the newer
version but does not lose their existing data. On next save with the
older version, the cosave is rewritten in the older format. Data from
unknown blocks is dropped. This is expected and documented behaviour.

---

## Cosave Lifecycle

### On Game Load

1. OHUI reads `characterId` from the Skyrim save via SKSE
2. OHUI locates the most recent `.ohui` file for this character
3. OHUI validates the cosave header and checksum
4. OHUI reads the block directory
5. OHUI loads blocks in dependency order — widget states before
   layout profiles, outfit definitions before radial config, etc.
6. OHUI applies any pending migrations
7. OHUI fires `kCosaveLoaded` — systems and mods may now read data
8. Game continues to main menu or into the loaded save

### On New Game

1. OHUI generates a new `characterId`
2. OHUI creates an empty cosave in memory
3. OHUI applies built-in default layout profiles and widget states
4. OHUI fires `kCosaveCreated` — systems and mods initialise their data
5. The cosave is written to disk on first save

If a New Game+ package is present (player selected New Game+ from
main menu), the legacy data block is applied after step 3 before
`kCosaveCreated` fires. Systems that participate in New Game+ read
the legacy block and initialise their state accordingly.

### On Save

1. OHUI collects dirty flags from all data systems
2. OHUI serialises changed blocks to their binary format
3. OHUI updates block checksums
4. OHUI writes the cosave file atomically — writes to a temp file,
   validates the temp file, then renames over the existing cosave
5. OHUI logs the save duration and cosave file size at debug level

Atomic write prevents cosave corruption on interrupted saves —
a power loss or crash during save leaves the previous cosave intact.

### On Character Delete

When a Skyrim save is deleted from the load game screen, OHUI offers
to delete the associated cosave. The player can decline — they may
want to keep the cosave for New Game+ purposes. If declined, the
cosave is retained and visible in a "detached cosaves" section of
the load game screen, available for selection as a New Game+ source.

---

## Debug and Diagnostics

### Cosave Inspector

Available in the Maintenance section of MCM2's settings. Shows:
- Cosave file path and size
- Character ID
- Block inventory — each block, its version, its size, its checksum
  state (valid / invalid / missing)
- Per-block entry counts where applicable
- Orphaned mod data entries (mod no longer loaded)
- Total data usage by system and by mod

### Export to JSON

A debug export produces a human-readable JSON representation of the
entire cosave. Useful for diagnosing unexpected state, for bug reports,
and for understanding what OHUI is persisting on behalf of the player.

Sensitive data (item form IDs, character names) is included. The export
is a developer tool and is clearly labelled as such. Players sharing
bug reports are advised to review the export before sharing.

### Cosave Repair

If the cosave fails validation on load — bad checksum, truncated file,
format error — OHUI does not crash. It logs the error at error level,
reports the failure to the player in a non-blocking notification, and
continues with empty data (as if new game). The player's Skyrim save
loads normally. OHUI state is reset.

The repair tool in MCM2 Maintenance attempts to recover partially
valid cosaves — reading blocks whose checksums are valid and discarding
corrupted blocks. Recovered data is partial but better than nothing.
The player is informed of exactly which blocks were recovered and which
were lost.
