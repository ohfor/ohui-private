# Outfit System

## Overview

A named outfit is a saved equipment set — every equipped item in every slot,
captured at the moment of saving and recallable at any time. The player names
it, saves it, and can switch to it from the inventory screen or from a HUD widget.

This is a launch requirement. It is fully in scope, fully a UI feature, and
fully deliverable in 1.0.

The outfit system covers both player outfits and follower outfits. The
infrastructure is shared. Follower outfits are a launch requirement alongside
player outfits.

---

## Creating an Outfit

From the inventory screen, with a desired set of equipment worn, the player
selects "Save as Outfit" from the inventory context menu. A name input appears
(OSK on controller). The outfit is saved with the current equipment state,
including weapons, armour, jewellery, and clothing in every slot.

---

## Outfit Panel

A dedicated panel in the inventory screen lists all saved outfits. Each entry
shows the outfit name and, where available, a thumbnail from the character
preview at the time of saving — a small portrait of the character wearing that
outfit. The player can browse saved outfits, preview them in the character
preview panel in real time, and equip them in one action.

---

## Real-Time Preview

Selecting or hovering over an outfit in the panel updates the character preview
immediately — the character is shown wearing that outfit before the player commits
to equipping it. Simultaneously, the character impact panel shows the full
aggregate delta between the hovered outfit and whatever is currently worn.

The player sees the look and the complete character state consequence side by
side, before committing to anything:

"Switching to Thieves Guild Armour from current equipment:
Armour rating -84 (412 → 328) / Movement speed +6% / Encumbrance -18 /
Frost resist -20% / Sneak +15 / Pickpocket +10 / One-Handed +10"

The outfit delta uses the same calculation engine as the individual item character
impact panel. An outfit is multiple simultaneous item swaps. The delta is the
aggregate of all of them. The outfit selector is therefore not just a wardrobe
— it is a decision-making tool. The player opening it before a dungeon run sees
exactly what they are trading before they swap, not after.

---

## Equipping an Outfit

Equipping an outfit swaps all slots to the saved items. If a saved item is no
longer in the player's inventory (sold, dropped, stored in a chest) the outfit
equips everything it can find and flags the missing items clearly. The player
knows what is missing before the swap completes.

---

## Outfit Management

- Rename
- Overwrite (update saved state to current equipment)
- Duplicate
- Delete
- Reorder (drag to sort)

---

## HUD Widget Integration

Saved outfits are available as targets in the HUD action bar and quick-slot
system. A player who switches between a combat loadout and a social loadout
can swap between them without opening the inventory. One button. The character
preview on the HUD widget (if the skin supports it) updates to reflect the
active outfit.

---

## Combat vs Social Loadout Pattern

The most common use case is a combat outfit and a social outfit — heavy armour
for dungeons, fine clothes for court. OHUI ships with a built-in distinction
between combat and social outfit slots that can be toggled automatically based
on context (entering combat, entering a city). This is an optional feature,
off by default, configurable in MCM2.

---

## Follower Outfits

The outfit system is built for characters with equipment slots. Followers are
characters with equipment slots. The extension from player outfits to follower
outfits is almost free once the player-side system exists and it solves a
genuine daily-use problem.

Managing a follower's equipment is currently a manual slot-by-slot operation
through the container screen. Dressing Lydia in heavy armour for a dungeon
run and wanting her in something presentable for Dragonsreach is the same
operation repeated every time — open container, swap piece by piece, close.
With follower outfits it is one selection.

**Where it surfaces:**
The follower container screen gets an outfit panel in the header — the same
outfit panel as the player inventory screen, scoped to the follower's saved
outfits. The player can browse the follower's saved outfits, preview them
on the follower in the character preview viewport, see the full character
impact delta, and equip in one action.

Same system. Same components. Same preview. Same delta panel. Different
subject.

**Outfit management for followers:**
Follower outfits are created and managed the same way as player outfits —
from the follower container screen with the follower's current equipment
state. Named, saved, overwritable, reorderable. A player who maintains
multiple followers can save and manage outfits for each independently.

---

## NFF Integration

NFF is on the frozen giants list. Follower outfit management is one of
the things NFF players use NFF for. OHUI's follower outfit system is
native first-party functionality that covers the same use case. NFF
players with OHUI get the full outfit system through the container screen
without any NFF-specific configuration.

OHUI's version is what the feature should have always been. Visual, immediate,
preview-first. The complete design is: here is your follower, here is what
they look like in this outfit, one button to equip it.

OHUI's outfit swap is immediate. No scripts queued. No waiting. The player
opens the container screen, selects the outfit, sees it in the preview,
confirms. Done.

---

## Outfit Labels and the Trigger Mod Pattern

Outfits can be assigned a semantic label — a freeform tag the player sets
when creating or editing an outfit. "Home", "Combat", "Social", "Stealth",
or anything else. The label is metadata. OHUI stores it, exposes it via the
API, and does nothing else with it.

What acts on labels is deliberately out of scope for OHUI. A separate trigger
mod reads the label and fires the swap when a condition is met — arriving at
a home cell, entering combat, crossing a city boundary, whatever the mod
decides. That mod does not need to know anything about equipment, followers,
or previews. It asks OHUI to swap a character to their outfit with a given
label. OHUI handles the rest. Instantly, correctly, no scripts queued.

The label API is the clean seam between OHUI's responsibility and the trigger
mod's responsibility. Multiple trigger mods with different philosophies can
coexist — one for cell type, one for time of day, one for quest state, one
for MCM-configured conditions — all speaking the same language, all getting
the same execution quality from OHUI.

---

## Persistence

Outfit definitions for the player and all followers are stored in the cosave
Outfit Definitions block. Item references use plugin name plus local form ID
to survive load order changes. On load, OHUI resolves references against the
current load order. Missing plugins are flagged and shown as unresolvable in
the outfit panel.

---

## Data Model

The outfit data model includes a theme slot from day one. Manual outfit-to-UI-
theme pairing is a near-term post-1.0 feature with a clear implementation path.
Including the slot now avoids retrofitting it later.

---

## Scope

Follower outfits are a launch requirement alongside player outfits. The
infrastructure is shared. The additional cost of extending to followers
is low. Shipping player outfits without follower outfits would be a
noticeable gap for anyone who plays with companions.
