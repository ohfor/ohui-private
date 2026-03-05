# Tween Menu

## The Problem

The Tween Menu is the navigation panel that appears when the player presses Tab —
the quick-access layer between gameplay and the full menu screens. It provides
five destinations: Inventory, Magic, Map, Skills, and Wait.

It has been hardcoded to these five destinations since launch. Every port, remaster,
and anniversary edition has left it untouched.

The specific failures:

- Hardcoded destinations. No way to add, remove, or reconfigure slots.
- No awareness of mod-added screens. Custom skill trees, LOTD's journal, and any
  other mod-added menu surface have no place in the Tween Menu.
- MCM is buried three levels deep. Tab → System → Mod Configuration. A screen
  players access constantly requires three menu transitions every time.
- No extensibility. Mod authors who add new major screens have no way to surface
  them at the navigation level where they belong.
- The radial layout is fixed. Players who want a different arrangement, different
  number of slots, or different directional assignments cannot change anything.

---

## Design Intent

The Tween Menu becomes a configurable quick-nav widget. It ships with a layout
familiar enough that existing players are not disoriented on day one. It is built
from the ground up as a widget so that configuration, extension, and skinning are
natural rather than bolted on.

The most important property: any screen registered with OHUI's menu system is
automatically available as a Tween Menu destination. Mod authors do nothing special.
Their screens are just there, available to be assigned to a slot.

---

## Default Layout

OHUI ships a default Tween Menu that closely resembles the vanilla radial — five
primary destinations in familiar positions. Players switching to OHUI find the
Tab menu where they expect it, doing what they expect.

**Default slots:**
- Inventory
- Magic
- Map
- Skills
- Wait

The defaults match vanilla. Players can add any destination they want —
including MCM — through the configuration system. OHUI does not impose
opinionated defaults beyond what players already know.

---

## Configuration

The Tween Menu configures itself. There is no separate settings screen, no MCM
page, no INI editing. Configuration happens inline, in edit mode, right where
the widget lives.

**The flow:**
Hold edit mode key → Tween Menu grows handles → right-click → context menu
appears showing current slots and their destinations → drag to reorder, tap a
slot to reassign its destination, tap + to add a slot, tap × to remove one.

This is the edit mode philosophy applied to its most natural use case. The Tween
Menu is the widget players are most likely to want to reconfigure — and the one
where doing it inline, immediately, without navigating elsewhere, feels most
obviously right.

**Destination picker:**
When reassigning a slot, a searchable destination picker appears — an OHUI list
component populated live from the menu registry. Every registered destination is
always present. Type to filter, scroll to browse, select to assign. The list is
always current because it reflects the actual state of the menu system, not a
static configuration.

**Slot destinations:**
- Any first-party OHUI screen (Inventory, Magic, Map, Skills, Journal, MCM, System)
- Any custom skill tree registered with OHUI
- Any mod-added screen registered with OHUI's menu system
- Wait (action, not a screen)
- Sleep (action)
- Quit to Desktop (action — OHUI has no opinion about what belongs in the Tween
  Menu. If the player wants the nuclear option one button press from gameplay,
  that is a completely valid configuration.)
- Save Game / Quick Save (action)
- Custom Papyrus function call (for mod authors who want a Tween slot that triggers
  a script action rather than opening a screen)

**Layout options:**
- Number of slots: configurable. More slots, smaller targets. Player's choice.
- Directional assignment: each slot can be assigned to a specific direction or
  position in the radial. A player who wants Map on DOWN and Wait as a separate
  DOWN-secondary assignment gets exactly that.
- Non-radial layouts: skin authors can implement grid, linear, or any other
  arrangement. The data contract is a list of slots with destinations and metadata.
  The radial is the default ControlTemplate, not the only one.

---

## MCM as Available Destination

MCM2 is available as a Tween Menu destination for players who want it.
The vanilla Tween Menu could never be extended — MCM was always buried
three levels deep. OHUI makes it assignable to a slot like any other
screen.

MCM is not a default slot. Players who want Tab → MCM add it
themselves through the configuration system. One step from gameplay
for players who choose it.

---

## Custom Skill Tree Destinations

Mod-added skill trees registered with OHUI (see Skills screen document) are
automatically available as Tween Menu destinations. A player running LOTD can
assign the LOTD skill trees to a Tween slot. A player running a Thieves Guild
progression mod can navigate to that screen directly from Tab.

No mod author work required beyond the skill tree registration that gives the
tree its skills screen presence. The Tween Menu destination follows automatically.

---

## Extension Points

Any screen that registers with OHUI's menu system is available as a Tween Menu
destination. Registration is the only requirement. The Tween Menu does not need
to know about individual mods — it asks OHUI's menu system for the list of
available destinations and presents them as assignable slots.

This means:
- A mod author who ships a new major screen gets a Tween Menu slot available
  to their users on day one
- Players with large mod lists can surface their most-used mod screens at the
  top navigation level
- The Tween Menu grows naturally with the player's mod list without any
  central coordination required

---

## Skin Support

The Tween Menu is a widget with a ControlTemplate. The default skin ships the
familiar radial. Alternative presentations are available to skin authors:

- Horizontal or vertical quick bar
- Grid layout for larger slot counts
- Minimal icon-only strip
- Full label with description on hover/focus

The slot data contract exposes destination name, icon, and a short description.
Skins implement whatever visual treatment they choose.

---

## Player Experience Goals

The Tween Menu is not a wow feature. It is a quality of life correction that
players feel immediately and positively.

The configurability is the specific moment — the first time a player adds
MCM to the Tween Menu and reaches their mod configuration in one button
press instead of three, they notice.

The extensibility is the ecosystem moment — mod authors whose screens finally
have a natural home in the navigation hierarchy rather than being buried or
unreachable will surface it in their documentation. Players will find screens
they previously didn't know how to reach.

---

## Open Questions

1. **Slot limit:** A radial with too many slots becomes unusable. What is the
   practical maximum for the default radial layout before the skin should switch
   to a different presentation? Probably 8, possibly 10. Needs a decision for
   the default skin's layout logic.

2. **Secondary directions:** The example of Map and Wait both assigned to DOWN
   as primary and secondary — how is secondary accessed? Hold vs tap distinction,
   or a sub-menu that expands on hover? The interaction model for secondary
   assignments needs design.

3. **Controller vs keyboard/mouse:** The radial is a natural controller interface.
   Keyboard users may prefer a different default layout. Does OHUI ship different
   default Tween Menu configurations per input device, or one layout that works
   for both?
