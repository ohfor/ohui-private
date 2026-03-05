# Mod Registration API

## Purpose

The Mod Registration API is not a single API. It is a pattern — a
consistent contract that OHUI exposes at every point where the ecosystem
can extend OHUI's behaviour. The pattern is the same everywhere it
appears. The specific data being registered differs. The mechanism,
the guarantees, and the failure behaviour are identical across all
registration points.

This document defines the pattern once. Individual registration points
reference this document rather than redefining the contract each time.

---

## The Pattern

OHUI defines containers. The ecosystem fills them.

At every extensible point in OHUI, the same structure appears:

1. **A registration function** — callable from Papyrus or C++/SKSE.
   The mod provides data. OHUI stores it.

2. **A registration window** — the period during which registration
   is valid. Always SKSE's post-load phase. Registrations that arrive
   outside this window are rejected with a logged warning.

3. **A soft dependency guarantee** — OHUI's behaviour if no mod
   registers anything at a given point is well-defined and correct.
   The container exists. It is empty. OHUI renders the empty state
   gracefully. Nothing breaks.

4. **A validation step** — OHUI validates registered data before
   accepting it. Invalid registrations are rejected and logged. They
   do not crash. They do not silently corrupt state.

5. **A versioned contract** — the registration function signature
   is versioned. OHUI can evolve the contract without breaking mods
   that registered against an older version.

---

## Registration Window

All registrations must occur during SKSE's post-load phase —
specifically in response to the `kPostLoad` or `kDataLoaded` SKSE
messaging events, before the main menu appears.

```cpp
// C++ registration — correct timing
SKSEMessagingInterface::Message* msg) {
    if (msg->type == SKSEMessagingInterface::kDataLoaded) {
        OHUI::Register::SurvivalMod(manifest);
    }
}

// Too late — will be rejected
void OnMenuOpen() {
    OHUI::Register::SurvivalMod(manifest); // REJECTED — logged warning
}
```

Papyrus registrations fire from `OnInit` or `OnPlayerLoadGame` events
in a quest script. Both are within the valid window. Registrations
from MCM callbacks or other late-firing events are outside the window
and are rejected.

The window closes once OHUI has completed its initialisation pass.
After that point the registered data is locked for the session. This
is a deliberate constraint — OHUI does not support dynamic registration
changes mid-session. The data model is stable for the lifetime of a
game session.

---

## The Registration Manifest

Every registration point accepts a manifest — a structured data object
describing what is being registered. The manifest structure differs per
registration point but always includes:

```cpp
struct BaseManifest {
    std::string modId;       // Unique identifier. Reverse domain style.
                             // e.g. "com.mymod.survival"
                             // Stable across versions.
    std::string displayName; // Human-readable name for OHUI's UI surfaces
    std::string version;     // Semantic version string "1.0.0"
    int         apiVersion;  // Which version of this registration contract
                             // the mod is targeting
};
```

`modId` is the stability anchor. OHUI uses it to deduplicate registrations
across load/save cycles, to log errors clearly, and to maintain per-mod
state. It must be unique and must never change between mod versions.

`apiVersion` is the contract version. OHUI supports all previous contract
versions for any given registration point. A mod compiled against API
version 1 works against OHUI shipping API version 3. The newer fields
are populated with defaults. Nothing breaks.

---

## Validation

OHUI validates every manifest before accepting it. Validation failures
are logged at warning level with the mod ID, the registration point, and
the specific validation failure. They never crash. They never silently
accept invalid data.

**Common validation failures:**
- Missing or empty `modId`
- Duplicate `modId` at the same registration point (second registration
  wins, first is logged as superseded)
- `apiVersion` higher than OHUI supports (logged, registration rejected)
- Required fields missing for the specific registration point
- Data outside valid ranges (e.g. negative timer durations, empty
  string arrays)

A mod that fails validation at one registration point does not affect
any other registration point. Failures are isolated.

---

## Soft Dependency Guarantee

Every registration point has a defined empty state. OHUI never requires
any registration to be present.

```
Sleep screen warnings:     no registrations → no warning panel shown
Message log custom types:  no registrations → built-in types only
Custom spell schools:      no registrations → vanilla schools only
Loading screen scenes:     no registrations → vanilla loading screen
Stats screen extensions:   no registrations → vanilla stats only
FacetedList facets:        no registrations → built-in facets only
Outfit trigger labels:     no registrations → manual switching only
Survival mod registration: no registrations → survival features absent
```

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

```cpp
OHUI::Register::SurvivalMod(SurvivalManifest manifest)
```

### Message Log — Custom Type Registration
Mods register custom message types with display names, icon references,
and colour tokens. Registered types appear as filter options in the
message log.

```cpp
OHUI::Register::MessageType(MessageTypeManifest manifest)
```

### Magic Screen — Custom Spell School Registration
Mods adding custom spell schools register them with a display name
and icon. Registered schools appear as filter options in the magic
screen's FacetedList and as labelled sections in spell detail panels.

```cpp
OHUI::Register::SpellSchool(SpellSchoolManifest manifest)
```

### Loading Screen — Scene Registration
Mods register 2D images or 3D animated scenes for display during
loading screens. Conditions determine when registered scenes appear.

```cpp
OHUI::Register::LoadingImage(LoadingImageManifest manifest)
OHUI::Register::LoadingScene(LoadingSceneManifest manifest)
```

### Stats Screen — Custom Stat Registration
Mods register tracked values for display in the stats screen alongside
vanilla stats. Values are polled at display time via a registered
callback.

```cpp
OHUI::Register::StatGroup(StatGroupManifest manifest)
```

### FacetedList — Custom Facet Registration
Mods register additional facets for any FacetedList surface. A facet
is a named filter predicate. Registered facets appear in the FacetPanel
under a mod-defined group heading.

```cpp
OHUI::Register::Facet(FacetManifest manifest, FacetPredicate predicate)
```

### Favourites Radial — Custom Slot Type Registration
Mods can register custom slot types for the favourites radial beyond
the built-in outfit / potion / spell / shout / item types.

```cpp
OHUI::Register::RadialSlotType(RadialSlotManifest manifest)
```

### Outfit Labels — Trigger Contract
Not a registration in the strict sense — a query API. The outfit
trigger mod pattern works by querying OHUI for the currently active
outfit label and requesting a label switch. Defined here for
completeness.

```cpp
// Query current outfit label for a character
std::string OHUI::Outfits::GetActiveLabel(RE::Actor* actor)

// Request outfit switch to label
bool OHUI::Outfits::SwitchToLabel(RE::Actor* actor, std::string label)
```

### HUD Widget Registration
Mod authors register new HUD widgets for inclusion on the canvas.
Covered in detail in hud-widget-runtime.md.

```cpp
OHUI::Register::Widget(WidgetManifest manifest, IWidget* impl)
```

---

## Papyrus API Surface

Every registration point is accessible from Papyrus via the
`OHUI_Register` script. The Papyrus API is a thin wrapper over the
C++ API — it accepts the same data as named parameters and delegates
to the C++ implementation.

```papyrus
; Survival mod registration from Papyrus
OHUI_Register.SurvivalMod( \
    modId = "com.sunhelm.survival", \
    displayName = "Sunhelm", \
    needsHunger = true, \
    needsThirst = true, \
    needsFatigue = true, \
    needsCold = false \
)

; Message type registration from Papyrus  
OHUI_Register.MessageType( \
    modId = "com.mymod.messagetype", \
    displayName = "Crafting", \
    iconPath = "Icons/crafting.dds", \
    colorToken = "color.accent.primary" \
)
```

The Papyrus API validates input at the Papyrus layer before delegating
to C++. Type mismatches and out-of-range values produce Papyrus-level
errors with clear messages rather than propagating to the C++ layer.

---

## Error Handling and Logging

All registration errors are written to OHUI's log at warning level.
The log entry format is consistent:

```
[OHUI][Register][{RegistrationPoint}] WARNING: {modId} — {reason}
[OHUI][Register][SpellSchool] WARNING: com.arcanum.schools — 
    duplicate modId, superseding previous registration
[OHUI][Register][LoadingScene] WARNING: com.mymod.loading — 
    apiVersion 4 not supported (max: 3), registration rejected
[OHUI][Register][SurvivalMod] WARNING: com.mymod.survival — 
    displayName is empty, using modId as fallback
```

Warnings do not prevent the game from loading. They do not produce
in-game notifications to the player. They are diagnostic information
for mod authors. A player whose mod list produces registration warnings
experiences no visible difference — the registration either succeeded
with a fallback or was rejected, both of which have defined graceful
outcomes.

---

## Versioning and Compatibility

OHUI maintains backwards compatibility for all registration contract
versions indefinitely. A mod compiled against API version 1 of any
registration point will work against any future version of OHUI.

When a registration contract evolves — new optional fields, new
capabilities — the `apiVersion` increments. Mods targeting the old
version receive default values for new fields. Mods targeting the new
version get full access to new capabilities.

Breaking changes to a registration contract are not permitted. If a
breaking change is unavoidable a new registration point is created
alongside the old one. The old one is deprecated but continues to
function. Deprecation notices appear in the log. The old registration
point is never removed while mods in the wild use it.

This is the same commitment OHUI makes to the MCM2 compatibility
layer. The contract, once published, is honoured permanently.
