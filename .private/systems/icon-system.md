# Icon System

## Purpose

SkyUI introduced item type icons to Skyrim's inventory — visual type
indicators on every list entry that the vanilla UI never had. This is
now baseline player expectation. OHUI must ship its own icon system
that covers at minimum what SkyUI provides, and is extensible for
mod-added content.

The icon system is a resource layer — it sits below the component
library alongside tokens and fonts. Components that have icon slots
(ListEntry, StatusBadge, ButtonPrompt, etc.) consume icons from this
system. They do not know where the icons come from or how they are
resolved.

---

## Scope

The icon system covers every context in OHUI where a visual type
indicator appears:

- **Items** — weapons, armour, potions, food, ingredients, books,
  scrolls, soul gems, keys, misc items, ammunition. Resolved from
  item type, keywords, and form data.
- **Spells** — spell school icons, spell type indicators (concentration,
  fire-and-forget, ritual). Resolved from magic effect data.
- **Perks** — perk icons in the skills screen constellation and
  description panel.
- **Shouts** — shout icons in the magic screen and favourites radial.
- **Status indicators** — stolen, quest item, enchanted, favourited,
  tagged (junk/keep/crafting). Resolved from item flags and OHUI
  tag state.
- **UI chrome** — navigation icons, button prompt glyphs, edit mode
  handles, context menu icons. These are skin-provided, not
  game-data-resolved.

---

## Resolution

An icon is resolved from game data, not assigned manually. The system
maps item properties (type, keywords, subtype) to an icon in the
active atlas. The mapping must handle:

- Vanilla item types comprehensively
- DLC items (Dawnguard, Hearthfire, Dragonborn)
- Mod-added items that use standard keywords (automatic)
- Mod-added items with custom keywords (extensible)

The goal is zero-configuration for the vast majority of mod content.
A mod that adds a new sword using standard weapon keywords gets the
correct weapon icon automatically. A mod that introduces an entirely
new item category may need icon support added — either through OHUI
updates or through the extensibility mechanism.

---

## Icon Atlases

Icons are shipped as atlases — sprite sheets that the renderer samples
from. OHUI ships a first-party atlas covering all vanilla and DLC
icon needs.

Skin authors can ship replacement atlases with their own visual style.
The atlas format and icon key mapping are part of the skinning contract
— a skin that ships an atlas replaces the default icons for all
components that consume them.

---

## Extensibility

Mod authors who add genuinely new item categories (not coverable by
existing keyword-to-icon mappings) can ship icon additions. The
mechanism for this — whether it's additional atlas entries, a mapping
file, or integration with the frozen giant support — is an open design
question.

---

## Open Questions

1. **SkyUI icon compatibility:** SkyUI's icon system uses a specific
   atlas format and keyword mapping. Whether OHUI adopts the same
   format (for compatibility with mods that ship SkyUI icon patches)
   or defines its own (cleaner, but breaks existing icon mods) needs
   a decision.

2. **Icon resolution performance:** Large mod lists can have thousands
   of unique items. Icon resolution must be fast enough to not impact
   list scrolling performance.

3. **Animated icons:** Whether any icon contexts warrant animation
   (active effect icons ticking down, enchantment charge indicators)
   or whether all icons are static.
