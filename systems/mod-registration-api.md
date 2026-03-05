# Mod Registration API

## Purpose

The Mod Registration API is not a single API. It is a pattern — a
consistent contract that OHUI exposes at every point where the
ecosystem can extend OHUI's behaviour. The pattern is the same
everywhere it appears. The specific data being registered differs.
The mechanism, the guarantees, and the failure behaviour are identical
across all registration points.

This document defines the pattern once. Individual registration points
reference this document rather than redefining the contract each time.

---

## The Pattern

OHUI defines containers. The ecosystem fills them.

At every extensible point in OHUI, the same structure appears:

1. **A registration function** — callable from native code or
   Papyrus. The mod provides data. OHUI stores it.

2. **A registration window** — the period during which registration
   is valid. Registrations that arrive outside this window are
   rejected with a logged warning.

3. **A soft dependency guarantee** — OHUI's behaviour if no mod
   registers anything at a given point is well-defined and correct.
   The container exists. It is empty. OHUI renders the empty state
   gracefully. Nothing breaks.

4. **A validation step** — OHUI validates registered data before
   accepting it. Invalid registrations are rejected and logged.
   They never crash. They never silently corrupt state.

5. **A versioned contract** — the registration function signature
   is versioned. OHUI can evolve the contract without breaking mods
   that registered against an older version.

---

## Registration Window

There are two categories of registration point, each with its own
timing.

### Non-MCM Registration Points

All non-MCM registrations (widgets, facets, survival mods, custom
spell schools, loading screen scenes, stat groups, radial slot types)
must occur during the initial post-load phase, before the main menu
appears.

The window closes once OHUI has completed its initialisation pass.
After that point the registered data is locked for the session. This
is a deliberate constraint — OHUI does not support dynamic registration
changes mid-session. The data model is stable for the lifetime of a
game session.

### MCM Registration

MCM registrations follow a different lifecycle because the existing
SkyUI MCM ecosystem depends on re-registration on every save load.

The MCM registration window opens during the initial post-load phase
(same as all other registration points) and **re-opens on every
subsequent save load**. This matches SkyUI's behaviour: mods that
register their MCM from the standard save-load event have their
registration accepted every time.

Re-registrations from the same mod are accepted as updates to the
existing registration. The mod's MCM page structure, control
definitions, and callbacks are refreshed. MCM list ordering, hide,
and rename state applied by the player are preserved across
re-registrations.

**MCM2 Refresh Registry:** The MCM2 developer tool "Refresh Registry"
re-runs the MCM registration pass mid-session outside the normal
window. This is a developer affordance scoped exclusively to MCM —
it does not apply to any other registration point.

---

## The Registration Manifest

Every registration point accepts a manifest — a structured data
object describing what is being registered. The manifest structure
differs per registration point but always includes:

- **Mod ID** — unique identifier, reverse domain style (e.g.
  `com.mymod.survival`). Stable across versions. OHUI uses this to
  deduplicate registrations, log errors, and maintain per-mod state.
- **Display name** — human-readable name for OHUI's UI surfaces.
- **Version** — semantic version string.
- **API version** — which version of this registration contract the
  mod is targeting.

OHUI supports all previous contract versions for any given
registration point. A mod targeting an older API version works
against any newer OHUI — newer fields are populated with defaults.

---

## Validation

OHUI validates every manifest before accepting it. Validation failures
are logged at warning level with the mod ID, the registration point,
and the specific failure. They never crash. They never silently accept
invalid data.

Common validation failures: missing or empty mod ID, duplicate mod ID
at the same registration point (second registration supersedes first),
API version higher than OHUI supports (registration rejected), required
fields missing, data outside valid ranges.

A mod that fails validation at one registration point does not affect
any other registration point. Failures are isolated.

---

## Soft Dependency Guarantee

Every registration point has a defined empty state. OHUI never
requires any registration to be present.

- Sleep screen warnings: no registrations → no warning panel
- Message log custom types: no registrations → built-in types only
- Custom spell schools: no registrations → vanilla schools only
- Loading screen scenes: no registrations → vanilla loading screen
- Stats screen extensions: no registrations → vanilla stats only
- FacetedList facets: no registrations → built-in facets only
- Outfit trigger labels: no registrations → manual switching only
- Survival mod registration: no registrations → survival features absent

The empty state is never a broken state. It is always a graceful
degradation to the correct vanilla-equivalent behaviour.

---

## Registration Points

The following registration points exist in OHUI. Each is defined in
full in its respective system or screen document. Listed here for
completeness.

### Sleep Screen — Survival Mod Registration
Survival mods register their needs system (hunger, thirst, fatigue,
cold) with display names, severity thresholds, and warning copy.
OHUI's sleep screen renders registered needs in the character state
panel.

### Message Log — Custom Type Registration
Mods register custom message types with display names, icon
references, and colour tokens. Registered types appear as filter
options in the message log.

### Magic Screen — Custom Spell School Registration
Mods adding custom spell schools register them with a display name
and icon. Registered schools appear as facet options in the magic
screen and as labelled sections in spell detail panels.

### Loading Screen — Scene Registration
Mods register 2D images or 3D animated scenes for display during
loading screens. Conditions determine when registered scenes appear.

### Stats Screen — Custom Stat Registration
Mods register tracked values for display in the stats screen
alongside vanilla stats. Values are polled at display time via a
registered callback.

### FacetedList — Custom Facet Registration
Mods register additional facets for any FacetedList surface. A facet
is a named filter predicate. Registered facets appear in the
FacetPanel under a mod-defined group heading.

### Favourites Radial — Custom Slot Type Registration
Mods can register custom slot types for the favourites radial beyond
the built-in outfit / potion / spell / shout / item types.

### Outfit Labels — Trigger Contract
A query API. The outfit trigger mod pattern works by querying OHUI
for the currently active outfit label and requesting a label switch.

### HUD Widget Registration
Mod authors register new HUD widgets for inclusion on the canvas.
Covered in detail in hud-widget-runtime.md.

---

## Papyrus API Surface

Every registration point is accessible from Papyrus. The Papyrus API
is a thin wrapper over the native API — it accepts the same data and
delegates to the native implementation.

The Papyrus API validates input at the Papyrus layer before
delegating. Type mismatches and out-of-range values produce clear
Papyrus-level errors rather than propagating to the native layer.

---

## Error Handling and Logging

All registration errors are logged at warning level in a consistent
format identifying the registration point, mod ID, and specific
failure reason.

Warnings do not prevent the game from loading. They do not produce
in-game notifications to the player. They are diagnostic information
for mod authors. A player whose mod list produces registration
warnings experiences no visible difference — the registration either
succeeded with a fallback or was rejected, both of which have defined
graceful outcomes.

---

## Versioning and Compatibility

OHUI maintains backwards compatibility for all registration contract
versions indefinitely. A mod targeting API version 1 of any
registration point will work against any future version of OHUI.

When a contract evolves, the API version increments. Mods targeting
the old version receive default values for new fields. Mods targeting
the new version get full access to new capabilities.

Breaking changes to a registration contract are not permitted. If
unavoidable, a new registration point is created alongside the old
one. The old one is deprecated but continues to function. Deprecation
notices appear in the log. The old registration point is never removed
while mods in the wild use it.

This is the same commitment OHUI makes to the MCM2 compatibility
layer. The contract, once published, is honoured permanently.
