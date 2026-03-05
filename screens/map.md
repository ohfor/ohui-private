# Map Screen

## The Problem

The vanilla map is a passive viewer. Players can look at it, click markers, and fast
travel. That is essentially all it does. It has no filtering, no search, no awareness
of quests beyond displaying markers, and no way to distinguish between multiple active
quest markers except by hovering each one individually.

A late-game map with mods installed is a sea of icons with no hierarchy. Twenty active
quest markers look identical. Hundreds of location markers compete for attention. The
map answers one question — "where is this thing" — and only if you already roughly
know where to look.

The specific failures:

- No text search. Finding a named location requires scanning the map manually.
- No filtering. All marker types are always visible simultaneously.
- No quest distinction. Multiple active quest markers are visually identical.
- No player-driven highlighting. There is no way to mark a location for personal
  navigation without starting a quest or using an external tool.
- No in-world connection. The map and the game world are completely separate —
  nothing on the map has any presence in the world beyond a compass marker.
- No hold awareness in context. The map shows holds but provides no summary of
  what the player has done or can do in each one.

---

## Design Intent

The map should be a planning tool, not just a viewer. It should answer the questions
players actually have: where is this place, which quest is closest, what is in this
hold, where do I want to go next. And it should maintain a connection to the game
world — markers the player cares about should have a presence in the world, not just
on a screen.

---

## Free Text Search

Type any location name. The map centres on it and highlights the marker. Partial
matches surface a result list. Search is instant, no confirmation required.

Search covers:
- All discovered map markers by name
- All holds and major regions by name
- Active quest names (centres on the quest's current target marker)
- NPC names where a map marker is associated (quest givers, followers)
- Custom player-named highlights (see World Beacons below)

Search is the fastest path from "I know what I'm looking for" to "I'm looking at it."
No scanning, no zooming, no hovering.

---

## Filtering

The map is filterable by marker type and quest state. Filters are persistent — the
player's filter state is saved with their layout profile.

**Marker type filters:**
- Cities and towns
- Dungeons and caves
- Dwemer ruins
- Nordic ruins
- Camps and forts
- Standing stones
- Shacks and farms
- Points of interest
- Undiscovered locations
- Custom highlights

Each type is independently toggleable. A player who only wants to see dungeons and
their active quest markers can have exactly that.

**Quest filters:**
- All active quests (default)
- Selected quest only — show markers for one quest, suppress all others
- Main quest only
- Faction quests only
- Side quests only
- Misc quests only
- No quest markers — map only, no quest overlay

**Hold filter:**
- Show only markers within the selected hold
- Useful for planning a hold-clearing session or reviewing what remains in an area

Filters are additive and composable. "Dungeons in The Reach with no quest overlay"
is a valid filter state. The player builds the map view they need.

---

## Quest Marker Distinction

Multiple active quest markers are currently visually identical. Players with several
active quests cannot tell at a glance which marker belongs to which quest without
hovering each one.

OHUI distinguishes active quest markers visually:

**Colour coding** — each active quest is assigned a colour. The map marker, the
compass marker, and any associated world beacon use the same colour for that quest.
Colour assignment is automatic but player-adjustable per quest.

**Numbering** — active quests are numbered by priority (recently updated first by
default, player-reorderable). Numbers appear on map markers and compass markers.
Quest 1 is always your current focus.

**Icons** — quest type is reflected in the marker icon. Main quest, faction quest,
side quest, and misc quest markers are visually distinct beyond colour.

**Quest panel** — a collapsible side panel on the map screen lists all active quests
with their assigned colour and current objective. Selecting a quest in the panel
centres the map on its marker and optionally activates a world beacon for it.

---

## World Beacons

The headline feature of the map screen. Any map marker — quest marker, location
marker, undiscovered location, or custom pin — can be designated as a world beacon.
A beacon is a pillar of light visible in the game world, at the marked location,
from a significant distance.

This is navigation that exists in the world rather than on a screen. The player
marks a location on the map, returns to the game, and sees a light in the distance
telling them exactly where to go.

### Technical Foundation

The approach is proven. Skyshards (a widely used mod) places authored pillars of
light at fixed predetermined locations using the same engine rendering capabilities.
OHUI places them dynamically, at any location, on player demand. The technique is
established. The scope is new.

World beacons are placed as dynamic light sources anchored to world coordinates.
They are visible through terrain at distance and resolve to a precise ground-level
indicator at close range. They are not HUD elements — they exist in the world
geometry, subject to weather, time of day, and fog of war like any other world
light. They feel like part of Skyrim because they use Skyrim's own visual language.

### Beacon Properties

**Colour** — matches the quest colour assignment for quest beacons. Player-chosen
for custom beacons. Up to N simultaneous beacons (configurable, default 3) with
distinct colours.

**Visibility distance** — configurable. Default: visible from approximately the
same distance as a city's ambient glow. Scales with beacon priority.

**Height** — the pillar extends from ground level upward. Height is sufficient to
be visible over terrain at the configured distance. Does not extend infinitely —
fades at a natural height appropriate to the world scale.

**Ground indicator** — at close range (within 50m by default) the pillar resolves
to a ground-level ring or marker indicating the precise target location. Navigation
becomes precise as the player approaches.

**Persistence** — beacons persist across fast travel, interior/exterior transitions,
and save/load cycles. They are stored in the cosave and restored on load. A beacon
placed before a session ends is present when the player returns.

### Beacon Management

Beacons are placed from the map screen. Any marker can be beaconed — right-click
(or equivalent controller action) on any marker to add or remove a beacon.

Active beacons are listed in the map's quest panel. Each beacon shows its name,
colour, distance from player, and a remove button.

The player can also place a beacon at an arbitrary world position — clicking an
unmarked area of the map and selecting "place beacon here" drops a custom beacon
at the clicked coordinates, named by the player via the OSK.

### Multiple Beacons

Up to the configured maximum, multiple beacons are simultaneously active. They are
visually distinct by colour. At distance they are individually identifiable. The
player planning a route through multiple locations can beacon all of them and
navigate between them without returning to the map.

---

## Live Location Preview

Any map marker can be previewed as a live render before fast travelling or navigating
to it. Selecting a marker on the map surfaces a viewport showing the actual location
as it exists in the current load order — geometry, textures, lighting, weather, all
mod-applied visuals included. What the player sees in the preview is exactly what
they will arrive at.

This is Street View for Skyrim. It has never existed before.

### Technical Approach

The engine already supports free camera positioning and rendering into viewports.
Photomode mods do this. Kill cams do this. Scripted sequences do this. OHUI applies
the same capability to the map screen — positioning a camera at or near the selected
marker's world coordinates and rendering what it sees into a native C++ viewport.

The same viewport mechanism used for NPC portraits in the dialogue screen. Same
pattern. Different camera position. The technical foundation is established.

**Camera placement** — the preview camera is positioned at the marker's world
coordinates, slightly elevated, oriented toward the location's primary approach
angle or centre of mass. For dungeons, the camera faces the entrance. For cities,
it faces the main gate. For wilderness locations, it faces the most visually
characteristic view. Default placement is automatic. The player can override it.

**Camera control** — the player can rotate the preview camera freely using the
right stick or mouse drag. A full 360 degree look-around from the camera's position.
Not a fixed screenshot — an explorable view. This is the feature. The player
looks around the location from the map screen before they go there.

**Zoom** — the player can push the camera forward and back along its view axis.
Combined with rotation this gives meaningful spatial understanding of the location
before arrival.

**Live rendering** — the preview reflects the current load order exactly. A
location retextured by a mod looks retextured in the preview. A location affected
by a lighting overhaul looks lit accordingly. Weather at the location at the
current game time is rendered. The preview is not a static image — it is the
actual world.

### Cell Loading

The primary technical constraint is cell loading. A distant exterior location may
not be loaded in memory when the player is looking at it on the map. OHUI handles
this by requesting a cell load for the preview on marker selection — a background
operation that surfaces a loading indicator in the preview viewport until the cell
is ready. The player sees the marker selected, sees a brief loading state, then
sees the live preview.

Interior locations (dungeons, caves, buildings) require their interior cell to be
loaded. Same approach — background load on selection, loading indicator, live
preview when ready.

Cell loading for preview purposes is lightweight compared to a full game transition.
The preview does not require NPC simulation, AI, or full physics — geometry and
lighting only. The performance cost is bounded and does not affect gameplay.

The loading state is not a flaw to be minimised — it is anticipation. The player
selects a marker, the preview panel opens with a brief loading indicator, and then
the world appears. That moment of reveal may feel better than instant. Google Street
View pre-captures everything precisely because they cannot do what this feature does
— render the actual current world in real time. A photograph from three years ago
loads instantly. The living world takes a moment. That is an acceptable trade and
possibly a feature in its own right.

### Preview Panel

The preview is surfaced as a panel within the map screen — not full screen, not
a separate screen. The map remains visible. The player can see where the location
is on the map while previewing what it looks like.

Panel size is configurable. Default is a quarter of the map screen. Can be expanded
to half screen for a more immersive preview. Collapsed by default — the player
opens it intentionally.

The preview panel can be detached and repositioned within the map screen using
the same edit mode mechanics as the rest of OHUI. A player who always wants the
preview in a specific position sets it once.

### Mod Location Support

Mod-added locations participate in live preview automatically if they register
map markers through standard Skyrim marker registration. The camera is positioned
at the marker's world coordinates regardless of which mod placed the marker. A
mod-added dungeon entrance previews exactly like a vanilla one.

This extends the value of live preview beyond vanilla content to the entire mod
ecosystem without any author work required.

---

## Hold Overview Panel

A collapsible panel accessible from the map screen providing a summary of the
selected hold:

- Discovered vs total known locations
- Active quests in this hold
- Completed quests in this hold
- Faction presence
- Named NPCs of interest currently in this hold

The hold overview connects the map to the quest journal's hold view. Selecting a
quest in the hold overview panel centres the map on that quest's marker. The two
screens are aware of each other.

---

## NPC Tracking

Any NPC can be tracked and marked on the map. Tracked NPCs appear as named markers
distinct from quest markers and location markers, with the same colour coding and
world beacon system available.

The UI renders whatever the tracking system provides. Whether an NPC becomes
trackable freely, via a gameplay mechanic (spell, shout, skill threshold, item),
or through some other gate is a gameplay and balance decision that lives outside
the UI entirely. The map screen consumes a tracked NPC list and displays it. The
mechanism that populates that list is not the map screen's concern.

### NPC Markers

**Map marker** — distinct visual type from location and quest markers. Named,
coloured, optionally beaconable. Colour is player-assigned or auto-assigned from
the same pool as quest colours.

**Compass marker** — appears on the compass when the player is navigating toward
a tracked NPC, identical in treatment to quest compass markers. Uses the assigned
colour for immediate recognition.

**World beacon** — any tracked NPC marker can be beaconed using the same system
as location beacons. Navigate to an NPC in the world the same way you navigate
to a location.

### Location Quality

The map is honest about the quality of its information. NPC location has two states:

**Current** — the NPC's location is actively known. Marker is solid and positioned
accurately. Updates as the NPC moves.

**Last known** — the NPC has moved outside tracking range, the tracking has lapsed,
or the NPC's current location cannot be determined. Marker shows the last confirmed
position with a clear visual indicator that the information may be stale. The player
knows they are navigating toward where the NPC was, not necessarily where they are.

The distinction between current and last known is a UI honesty requirement. Showing
a stale marker as if it were current would erode trust in the system. Players who
navigate to a last-known marker and find the NPC gone understand why — the UI told
them the information was uncertain.

### Tracking Panel

A collapsible panel on the map screen listing all currently tracked NPCs:

- NPC name
- Current location or last known location (with staleness indicator)
- Distance from player
- Tracking status: current / last known / lost
- Assigned colour
- Beacon toggle
- Remove from tracking

The tracking panel follows the same collapsible pattern as the quest panel and
beacon list. All three panels coexist on the map screen without crowding — collapsed
by default, expanded on demand.

### Scope

NPC tracking covers any NPC — followers, quest givers, merchants, named NPCs,
generic NPCs if the tracking system supports them. The UI does not impose its own
restrictions on which NPCs are trackable. Followers are not a special case — they
are tracked NPCs whose tracking is active by default when they are assigned.

The gameplay system that gates or enables tracking for different NPC types is
out of scope for this document.

---

## Fast Travel Integration

Fast travel remains available from the map screen. OHUI adds:

**Travel time estimate** — approximate in-game hours to reach the destination on
foot, displayed alongside the fast travel option. Contextual — only shown when
fast travel is available.

**Last visited indicator** — map markers show when they were last visited (game
date). Useful for hold-clearing playthroughs and players who want to track where
they have been.

**Follower location** — if the player has an active follower, their current location
is marked on the map. Not a quest marker — a distinct follower indicator. Answers
"where did I leave Lydia" without opening a follower management mod.

---

## Player Experience Goals

The map screen has one headline wow feature: **world beacons**. The moment a player
places a beacon on the map, goes back to the game, and sees a pillar of light in the
distance — that is the moment. It is immediately understood, immediately useful,
completely novel in Skyrim's thirteen year history, and highly shareable. A screenshot
or clip of a beacon visible across a valley needs no explanation.

The secondary wow moment is **quest distinction with colour coding**. A map with five
active quests, each with a distinct colour on marker and compass simultaneously, makes
immediately obvious something that has always been confusing. Players feel the relief
of clarity the first time they see it.

**Free search and filtering** are not wow moments — they are long overdue corrections
that generate quiet satisfaction and eliminate recurring frustration. They make the
map trustworthy in a way it has never been.

---

## Open Questions

1. **Beacon rendering in interiors:** World beacons are anchored to world coordinates.
   A beacon placed on an exterior location is not meaningful when the player is in an
   interior cell. Beacons should suppress when the player is in an interior with no
   spatial relationship to the beacon location. Edge case: the beacon target IS an
   interior (a dungeon entrance) — the beacon marks the entrance in the exterior world.

2. **Undiscovered locations:** Can a beacon be placed on an undiscovered location
   marker? Probably yes — the player can see undiscovered markers on the map and
   placing a beacon to navigate to one is a valid use case. Needs a decision.

3. **Beacon count limit:** The default of 3 simultaneous beacons is a guess. Too few
   limits utility. Too many creates visual noise in the world. Needs playtesting to
   find the right default. Should be player-configurable regardless.

4. **Mod-added map markers:** Mods that add custom map markers participate in search
   and filtering automatically if they use standard marker registration. Mods using
   non-standard approaches may not be discoverable. The boundary needs defining.
