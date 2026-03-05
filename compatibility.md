# Compatibility

## Philosophy

OHUI's compatibility strategy is built on two principles: the player's existing mod
list should keep working, and nothing should break catastrophically when it doesn't.
Authors should not be forced to update their mods for OHUI to be usable. Where OHUI
supersedes existing infrastructure, it provides compatibility layers that route legacy
calls through OHUI's own systems transparently.

Compatibility layers are installed by default. Players who know what they are doing
can disable individual layers in OHUI settings. The default experience is seamless.

This is not a courtesy. It is the adoption prerequisite. A player whose favourite
mods break on installing OHUI uninstalls OHUI. The quest journal, the skins, the
accessibility features — none of it matters if the mod list is disrupted. The mod
list is sacred. OHUI respects it on day one, unconditionally.

---

## Soft Dependencies and Graceful Degradation

Every integration point in OHUI — frozen giant native support, registration API
consumers, compatibility shims, and any other external dependency — is a soft
dependency. OHUI never hard-requires any mod to be present. It checks, uses what
it finds, and backs down cleanly when something is absent or incompatible.

**The pattern for every integration point:**
1. Check if the plugin is loaded
2. Check if the expected API surface or data is present and matches what OHUI
   needs
3. If yes — activate the integration, log it
4. If no — disable the feature that depended on it, log it, continue normally

No crashes. No broken screens. No missing functionality the player didn't ask
for. A player without Sunhelm installed gets no survival context panel and no
error. A player whose Sunhelm version changed in a way OHUI no longer recognises
gets the same outcome — the panel is absent, not broken.

**The log is a first-class diagnostic tool.** On startup OHUI checks every known
integration point and logs the result explicitly:

```
[OHUI] Sunhelm detected (v1.0.6) — survival context panel active
[OHUI] NFF detected (v3.9.2) — follower tracking active
[OHUI] Frostfall not detected — temperature integration disabled
[OHUI] Dirt and Blood detected but GetDirtLevel not found (version mismatch?) —
        status indicator disabled
```

A player troubleshooting a missing feature opens the log and immediately sees
why. No guessing. No "does this work with X" questions on the Nexus page. The
log answers the question before it is asked.

OHUI system log messages are published into the message log stream as a
`system_ohui` type. The player never needs to find a text file in their Skyrim
directory — they open the message log panel in-game, switch to the OHUI tab,
and read the same startup diagnostics formatted consistently with everything
else. Filterable, searchable, present where the player already is.

The log file on disk still exists as an export of the same data. Players who
want a text file to paste into a bug report or share on Discord have one. The
message stream is authoritative. The file is a mirror.

**Version resilience.** Soft dependencies mean a frozen giant that changes
unexpectedly — a game update forces a recompile, something shifts in the data
model — produces a graceful fallback rather than a catastrophic failure. The
maintenance burden of native support is real but the failure mode is benign.
The feature goes absent. The game keeps working. The log explains what happened.

**The API documentation is the other half of this.** A well documented, stable,
publicly available API means mod authors implementing against OHUI know exactly
what to provide. OHUI implementing against their APIs knows exactly what to
expect. The contract is explicit on both sides. Soft dependencies and clear
documentation together make the entire integration ecosystem trustworthy.

---

## The Viewport Floor

Every mod that renders any UI element gets a managed viewport in OHUI's layout
system. This is the viewport floor — the minimum guarantee OHUI makes to every
mod in the ecosystem, requiring zero author work.

OHUI does not need to understand what is inside the viewport. It does not need to
know the mod's data model, its update loop, its rendering approach, or its feature
set. It gives the viewport a home in the layout system and manages it like any
other widget:

- Positioned and sized in edit mode
- Included in layout profiles
- Show/hide with the rest of the HUD
- Opacity control
- Saved and restored with the player's layout

The mod keeps working exactly as it did. The player gets edit mode on it immediately.
No author update required. No migration period. No gap.

This is Tier 1 of a two-tier integration model:

**Tier 1 — Viewport floor (automatic, zero author work)**
The mod renders into a managed OHUI viewport. OHUI handles positioning, layout,
profiles, and edit mode. The mod's internal behaviour is unchanged. The player
experience is immediately better — their mod fits into their layout and is managed
consistently with everything else.

**Tier 2 — Native integration (opt-in, author-driven)**
The mod author implements the OHUI widget data contract. The widget gets skin
support, reactive data binding, widget metadata, MCM2 configuration, and full
ecosystem participation. The author invests when their users are already in the
OHUI ecosystem and the value of native integration is clear.

Tier 1 is what makes adoption possible. Tier 2 is what makes the ecosystem grow.
Neither is required for the other. A mod can live at Tier 1 indefinitely and deliver
real value to players. A mod that moves to Tier 2 gets a substantially better
experience. The author chooses on their own timeline.

The viewport floor applies universally — iEquip, STB Widgets, TrueHUD, Wheeler,
Detection Meter, any mod that renders UI. On day one of OHUI, every one of these
mods sits in a managed viewport. The player can drag it, resize it, include it in
a layout profile, and show or hide it with everything else. That is already better
than today. The rest follows.

---

## SkyUI MCM

SkyUI's Mod Configuration Menu is the most widely used mod author API in the
ecosystem. Thousands of mods register MCM pages via Papyrus. OHUI ships MCM2 as
its native configuration system, which supersedes SkyUI MCM entirely.

**Compatibility coverage:** Complete. OHUI intercepts SkyUI MCM registration calls
and renders the resulting configuration pages as MCM2 panels. Existing MCM mods
appear in OHUI's configuration screen without author changes. Visual language is
OHUI's — not SkyUI's. Behaviour is preserved exactly.

**Default state:** On.

**Details:** See DESIGN.md — Backwards Compatibility section.

---

## UIExtensions

UIExtensions by Expired6978 provides custom menus, input dialogs, selection lists,
and scroll menus that mod authors use when the vanilla UI cannot meet their needs.
It predates SkyUI MCM as general-purpose UI infrastructure and is deeply embedded
across a large number of mods via Papyrus script calls.

OHUI's component library supersedes everything UIExtensions provides — input
dialogs, selection lists, scroll menus — with full skin support, controller support,
OSK integration, and consistent visual language. The UIExtensions compatibility
layer routes legacy calls through OHUI components so existing mods benefit from
OHUI's presentation without author changes.

### Papyrus API Shim

The shim intercepts UIExtensions Papyrus function calls and maps them to OHUI
component equivalents. The calling mod's script does not change. Call signatures
are preserved. Return values come back in the expected format. The UI that appears
is an OHUI component.

**Covered calls:**

| UIExtensions API | OHUI Component |
|---|---|
| OpenMenu (ItemMenu) | Selection list |
| OpenMenu (ScrollMenu) | Scroll selection list |
| OpenMenu (CustomMenu) | Generic panel |
| OpenInputMenu | Input dialog + OSK |
| GetMenuResultString | Input dialog result |
| GetMenuResultInt | Selection list result |
| AddEntryItem | List entry |
| AddEntryText | List entry (text) |
| AddEntrySprite | List entry (icon) |
| SetMenuPropertyString | Component property |
| SetMenuPropertyInt | Component property |

**Not covered:**

UIExtensions has native SKSE components beyond the Papyrus API surface. These
cannot be intercepted at the Papyrus level and fall through to native UIExtensions
behaviour when UIExtensions is present in the load order. Visual consistency is
not guaranteed for these cases. The shim documents which calls fall through so
authors who want full OHUI integration know what to update.

### Behaviour When UIExtensions Is Not Installed

The shim functions as a standalone implementation. Mods that depend on UIExtensions
do not require UIExtensions to be installed when OHUI is present — the shim handles
their calls natively. UIExtensions becomes an optional dependency for any mod
that previously required it, as long as that mod's usage falls within the covered
API surface.

This is a meaningful quality of life improvement for mod list management. A mod
that previously required UIExtensions as a hard dependency can now run with OHUI
alone.

### Behaviour When UIExtensions Is Installed

The shim takes priority for covered calls. UIExtensions handles uncovered calls.
The player sees OHUI components for the majority of UIExtensions interactions and
native UIExtensions UI for the remainder. Not perfectly consistent but substantially
better than UIExtensions alone.

**Default state:** On.

**Disable if:** The player explicitly wants native UIExtensions behaviour for all
calls, or is troubleshooting a compatibility issue where the shim is suspected.

---

## RaceMenu

RaceMenu by Expired6978 is the character creation and appearance editing screen.
It ships a plugin architecture for custom sliders, morph systems, and overlay
integration that has accumulated significant ecosystem depth.

**OHUI does not touch RaceMenu.** Character creation is not a session-to-session
screen. The political and technical cost of engaging with RaceMenu's plugin
ecosystem outweighs any benefit. RaceMenu runs alongside OHUI without conflict.
If a future successor to RaceMenu wishes to implement OHUI's viewport contract,
the architecture supports it. OHUI has no opinion on character creation screens
and ships none.

---

## Native Notification System

Skyrim's native notification function (`Debug.Notification` in Papyrus,
`REL::Relocation` equivalent in SKSE plugins) is intercepted and published into
OHUI's message log stream as a `notification` typed message. Existing mods that
call the native notification function get their messages surfaced through OHUI's
HUD message widget automatically — correctly typed, attributed to the calling mod
where determinable, and permanently accessible in the message log panel.

No mod author changes required. Native notification calls keep working. They just
look better and are no longer lost after three seconds.

**Default state:** On. Cannot be disabled — native notification interception is
core OHUI infrastructure, not an optional compatibility layer.

---

## SkyUI Inventory and Other Screens

SkyUI's inventory, magic, barter, container, and crafting screen replacements are
superseded by OHUI's own screen implementations. When OHUI is installed, OHUI's
screens take precedence.

SkyUI's screens do not need to be uninstalled — OHUI's screen registration takes
priority in the menu stack. SkyUI's screens simply never activate. Players who
want to run SkyUI screens instead of OHUI screens should not install OHUI.

There is no compatibility shim for SkyUI screens. They are replaced, not wrapped.
The MCM registration system is shimmed (see above). The screens are not.

---

## Frozen Giants

Some mods are so deeply embedded in the ecosystem, so widely installed, and so
completely inactive that OHUI must support them natively. These mods will never
implement a registration API. They will never update to meet OHUI halfway. They
have hundreds of thousands of installs and they are frozen in time. OHUI goes
to them because they are never coming to OHUI.

This is a proven approach. SCIE and SLID implement native support for LOTD,
General Stores, NFF, and others on the same basis — the install counts are too
high and the mods too inactive to expect anything else.

Native support for a frozen giant means OHUI ships with specific knowledge of
that mod's data model baked in. Their indicators, states, and data appear in
OHUI correctly, styled consistently, without any author action. Players with
these mods installed get full integration on day one.

**This is a permanent maintenance commitment.** If native support for a frozen
giant breaks — due to a game update, an SKSE update, or any other change —
it is OHUI's problem to fix. The list must be kept deliberately short. Every
addition is an obligation that does not expire.

### The List

**Survival and Immersion**
- **Sunhelm** — survival needs (hunger, thirst, fatigue, cold). Last updated
  August 2022. Enormous install base. Sleep screen warnings, HUD status
  indicators, and survival context panel all benefit from native Sunhelm support.
- **Frostfall** — hypothermia and cold weather survival. Predates Sunhelm,
  still widely used, completely inactive. Temperature and warmth source data
  feeds the sleep screen survival context panel.
- **iNeed** — hunger, thirst, and sleep needs. Frozen. High install count.
  Overlaps with Sunhelm but distinct user base.
- **Dirt and Blood** — visual dirt and blood accumulation, bathing mechanic.
  Not quite as inactive as Sunhelm but effectively frozen. High install count.
  Status indicators surface in the HUD status layer.
- **Campfire** — camping and fire mechanics, warmth source detection. Feeds
  the sleep screen warmth source panel. Frozen.

**Followers**
- **Nether's Follower Framework (NFF)** — the de facto follower management
  standard for a significant portion of mod lists. Follower location, state,
  and assignment data feeds the map screen NPC tracking feature. Without native
  NFF support, follower tracking is significantly degraded for the majority of
  players who use it.
- **AFT (Amazing Follower Tweaks)** — older follower framework, still widely
  installed, largely frozen. Smaller current user base than NFF but significant
  enough to warrant native support.

**Content**
- **Legacy of the Dragonborn (LOTD)** — affects the map screen (LOTD map
  markers), custom skill trees (LOTD progression systems), quest journal
  (LOTD quest volume), and the Tween Menu (LOTD screens as destinations).
  One of the most installed content mods in the ecosystem. Actively developed
  but too important to leave to a registration API alone — native support
  ensures day one compatibility regardless of whether the LOTD team engages
  with OHUI's APIs.

**Economy**
- **General Stores** — merchant and economy framework. Native support warranted
  for the same reasons as LOTD — high install count, data OHUI's screens
  genuinely benefit from.

### What Native Support Means Per Mod

Native support is not identical for every frozen giant. What OHUI implements
depends on what data the mod exposes and which OHUI surfaces benefit from it:

- **Sleep screen** — Sunhelm, Frostfall, iNeed for survival context and
  character state warnings. Campfire for warmth source detection.
- **HUD status indicators** — Sunhelm, Frostfall, iNeed, Dirt and Blood
  for status widget data.
- **Map screen NPC tracking** — NFF, AFT for follower location data.
- **Tween Menu destinations** — LOTD for custom screen registration.
- **Custom skill trees** — LOTD for tree layout and perk data.
- **Quest journal** — LOTD for quest volume and dialogue history coverage.

### Adding to the List

A mod qualifies for the frozen giants list when:
1. Install count is high enough that a significant portion of OHUI's player
   base will have it installed
2. The mod is inactive enough that expecting author engagement is unrealistic
3. OHUI has surfaces that are meaningfully better with native support
4. The data model is stable enough to implement against reliably

The list is reviewed at each major OHUI release. Mods that resume active
development and implement OHUI's registration APIs graduate off the native
support list — their integration becomes their own responsibility. Mods that
no longer meet the install count threshold may be retired from native support
with appropriate deprecation notice.

---

## Future Compatibility Layers

As the ecosystem evolves, additional compatibility layers may be warranted. Candidates:

- **Completionist / Achievement tracker APIs** — mods that hook into quest and
  item tracking may benefit from OHUI message log integration
- **iEquip** — equipment switching UI with its own presentation layer, potentially
  shimable into OHUI's hotkey widget system
- **Favorite Things / Extended Favorites** — favorites menu extensions that could
  be routed through OHUI's action wheel component

These are not commitments. They are noted here so the pattern is established and
future compatibility work has a home.
