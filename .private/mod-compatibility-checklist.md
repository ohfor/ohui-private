# OHUI — Community Mod Compatibility Checklist

Source: vule1991's comment on Prisma UI Nexus page (27 Jan 2026)
> A real-world snapshot of what the modding community depends on daily.
> This list is not exhaustive but is a representative cross-section of daily-driver mods
> used by a significant portion of the modding community.

---

## Category: First-Party Widgets
*These should be absorbed directly into the core widget runtime. No third-party mod required.*

| Mod | Notes | Status |
|-----|-------|--------|
| **SkyUI** | The replacement target itself. Inventory, barter, container, magic, map, crafting menus + MCM | 🎯 Core |
| **TrueHUD** | Enemy health bars, boss bars, player HUD elements | 🎯 Core |
| **SkyHUD** | HUD layout and configuration | 🎯 Core — absorbed into widget edit mode |
| **moreHUD SE** | Item information in world and inventory, enemy level display | 🎯 Core |
| **Wheeler** | Action wheel / hotkey ring | 🎯 Core widget |
| **OxygenMeter2** | Stamina/oxygen meter widget | 🎯 Core widget |
| **STB Widgets** | Various HUD widgets (hunger, thirst etc.) | 🎯 Core widget candidates |
| **STB Active Effects** | Active magic effects HUD display | 🎯 Core widget |
| **Detection Meter** | Stealth detection indicator | 🎯 Core widget |
| **Compass Navigation Overhaul** | Extended compass with locations, quest markers | 🎯 Core widget |
| **QuickLoot IE / QuickLoot EE Fork** | Quick loot panel on container interaction | 🎯 Core widget |
| **Oblivion Interaction Icons** | Contextual interaction prompts | 🎯 Core widget |

---

## Category: MCM Compatibility
*These mods use MCM for configuration. Must work via the MCM compatibility layer
without mod authors needing to update.*

| Mod | Notes | Status |
|-----|-------|--------|
| **Equipment Durability System NG** | MCM-configured durability system | 🔄 MCM compat layer |
| **Better Third Person Selection (AE/SE)** | MCM-configured interaction system | 🔄 MCM compat layer |
| **Swiftly Order Squad** | Follower command UI + MCM config | 🔄 MCM compat layer |
| **Constructible Object Custom Keyword System** | Crafting keyword system, MCM config | 🔄 MCM compat layer |

---

## Category: Menu Replacements
*Full menu replacements that interact directly with the engine's menu stack.
Require deeper integration thinking.*

| Mod | Notes | Status |
|-----|-------|--------|
| **Modern Wait Menu** | Replaces the T-key wait menu. Mod author (Fallen01135) flagged this as a hard case — engine always opens original SWF, relies on internal AS2 engine calls that cannot be replicated in a web/overlay framework. Needs investigation for OHUI. | ⚠️ Needs investigation |

---

## Notes

- Items in **First-Party Widgets** represent the fragmentation problem directly. Each currently
  ships as a separate mod with its own visual language and config system. In OHUI these become
  core widgets with swappable skins.
- The **MCM compat layer** items should largely resolve themselves if the Papyrus contract is
  honoured correctly per Design Feature 1.
- **Modern Wait Menu** is the most technically interesting edge case — worth a dedicated
  investigation into how the engine binds the T-key to a specific SWF and whether that
  can be intercepted cleanly via the SWF override mechanism.
- Fallen01135 (Modern Wait Menu author) confirmed in the Prisma UI comments that Prisma cannot
  solve this problem. His comment also confirmed that Prisma is not a true UI replacement —
  it cannot intercept menus the engine opens natively. OHUI must solve this differently.

---

*Last updated: March 2026*
