# Skills Screen

## The Problem

The skills screen is Skyrim's most visually distinctive UI surface. The constellation
view is iconic — preserved through every port, remaster, and anniversary edition
without meaningful change. It is also one of the most beloved screens in the game
and the one most likely to generate backlash if handled poorly.

The visual experience is largely fine. The usability experience has real problems
that the visuals have always obscured:

- The perk description panel is a static text dump. Name and a paragraph. For vanilla
  perks this is barely adequate. For complex mod perks with multiple ranks, conditional
  effects, and synergies it is genuinely insufficient.
- No structured navigation of perk information. Everything is a flat read.
- No perk comparison or planning. There is no way to see a path through a tree before
  committing points.
- No support for entirely new skill trees added by mods. Custom trees from LOTD,
  Thieves Guild progression mods, and similar have no native UI support and rely on
  workarounds.
- The skill XP bar and level information is minimal — no context for how far away
  the next level is or what is driving XP gain.

---

## Design Intent

The skills screen is the one OHUI approaches most conservatively. The constellation
is load-bearing identity — it is so strongly associated with Skyrim that replacing
it, even with something objectively better, would generate immediate backlash that
follows the project permanently. OHUI does not lead with a replacement.

The first-party skin is a faithful, high quality implementation of the vanilla
constellation aesthetic. Players who install OHUI see something they recognise.
The work happens beneath the visuals — richer perk information, better navigation,
support for complex mod perk trees, and proper architecture for custom skill trees.

Skin authors can build alternatives — a flat functional grid, a card-based layout,
anything — but that is the community's choice to make on their own timeline.

---

## Constellation Skin (First-Party)

OHUI ships a constellation skin that matches the vanilla aesthetic faithfully.
Star positions, connection lines, the nebula backgrounds, the camera animation
on tree selection — all preserved. Players switching from vanilla or SkyUI to
OHUI see no visual disruption on this screen.

The constellation skin is a ControlTemplate implementation of the skills screen
data contract. It is not hardcoded. A skin author can replace it entirely. The
default experience is familiar because the constellation is right, not because
OHUI has locked it in.

---

## Ordinator and Vanilla Perk Tree Support

Ordinator replaces vanilla perks within the existing vanilla perk tree structure.
It does not add new trees or restructure constellation positions — it swaps what
the nodes contain while the structural skeleton remains vanilla's. From the skills
screen's perspective Ordinator is transparent. The constellation renders correctly
because the node positions are inherited from vanilla.

The data contract must be rich enough to surface Ordinator's complexity well.
Ordinator perks are significantly more involved than vanilla perks — multiple ranks,
conditional effects, interactions with other perks, extended descriptions. The perk
description panel (see below) handles this. The constellation itself requires no
special Ordinator handling.

Both vanilla and Ordinator work out of the box on day one. This is a hard requirement.

---

## The Perk Description Panel

The bottom panel is where the usability work lives. Currently a static name and
paragraph. OHUI replaces it with a structured, navigable information surface.

### Structured Perk Information

**Perk name and rank** — current rank and maximum rank shown clearly.
"Armsman 2 / 5" not "Armsman."

**Effect per rank** — each rank's effect listed separately and clearly. The player
can see what rank 3 gives them before they have rank 2. Planning requires knowing
the destination.

**Requirements** — skill level requirement and perk prerequisites shown explicitly.
What do I need to take this perk. Not buried in hover text — presented as primary
information.

**Conditional effects** — perks with conditions (Ordinator perks frequently have
these) surface their conditions clearly. "While dual wielding" or "against targets
below 30% health" is part of the perk's information, not a footnote.

**Synergies** — where a perk explicitly interacts with another perk in the same or
different tree, that relationship is noted. Not a comprehensive graph — just the
explicit interactions the perk itself references.

**Source** — which mod added this perk. Vanilla, Ordinator, a custom perk mod.
Same source attribution principle as dialogue options. On demand, not by default.

### Navigation

The description panel is navigable without leaving the constellation. From a
selected perk the player can:

- Move to prerequisite perks directly
- Move to perks that require this one as a prerequisite
- Navigate the description of a multi-rank perk through its ranks without taking it

This is the menu-like structure below the constellation. The constellation handles
spatial navigation of the tree. The description panel handles deep navigation of
the selected perk's information. Both are active simultaneously.

---

## Custom Skill Trees

Mods like Legacy of the Dragonborn, Thieves Guild progression overhauls, and
standalone skill tree frameworks add entirely new skill trees with no vanilla
skeleton to inherit constellation positions from. These trees need UI support.
Currently they rely on workarounds — MCM panels, custom Scaleform implementations,
or simply no UI at all.

OHUI supports custom skill trees as a first-class concept. A mod-added skill tree
appears in the skills screen alongside vanilla trees, uses the same constellation
skin, and participates in all the same features — perk description panel, path
planning, perk point management.

### Layout Generation

Custom trees do not have authored constellation positions. OHUI generates a layout
automatically from the tree's node relationship graph.

Given a directed graph of perks and their prerequisites, OHUI calculates a
constellation layout using a hierarchical layout algorithm — root perks (no
prerequisites) at the bottom, terminal perks at the top, prerequisite relationships
determining vertical position, related perks clustered horizontally. The result
is a readable constellation that communicates the tree's structure visually.

The generated layout is not as artistically crafted as vanilla's authored
constellations. It is functional, readable, and consistent. A skin author who wants
to hand-craft a constellation layout for a specific mod tree can provide authored
positions — OHUI uses them when present and falls back to generation when not.

### Mod Author Registration

Custom skill trees register with OHUI via a data format:

- Tree name and description
- Associated skill (ActorValue)
- Perk list with prerequisite relationships
- Optional: authored constellation positions per perk
- Optional: custom nebula background asset
- Optional: custom star assets

OHUI uses whatever the mod author provides and fills gaps automatically. A mod
author who provides only the perk list and relationships gets a fully functional
generated constellation. A mod author who provides full authored positions gets
exactly what they designed.

---

## Skill Information

The skill level display alongside the constellation is expanded beyond the vanilla
minimal presentation.

**Current level and XP bar** — vanilla. Preserved.

**XP to next level** — exact number or a meaningful range. Not just a bar.

**Active XP multipliers** — perks, standing stones, blessings, or other effects
currently affecting XP gain for this skill. Surfaced as a collapsible list.
Answers "why am I levelling this skill faster than expected" and "what should I
activate before training."

**Training count** — how many times the player has paid for training in this skill
this level. The vanilla cap is 5 per level. Showing the count removes the need
to remember or track it externally.

**Skill books read** — optional. Whether the player has found and read all skill
books for this skill. Not a checklist — just a count. Useful for completionist
playthroughs.

---

## Perk Point Display

Perk points available are shown prominently. Not buried — the primary action
resource for this screen deserves primary visual weight.

For mod frameworks that expand perk point acquisition (Ordinator's Perkus Maximus
compatibility, SimonMagus frameworks, etc.) the perk point display reflects
whatever the game's current perk point count is. No special-casing required.

---

## Player Experience Goals

The skills screen has one directive above all others: do not break what works.
The constellation is beloved. OHUI preserves it as the default and invests in
what it surrounds, not what replaces it.

The wow moment on this screen is not visual — it is informational. The first time
a player selects an Ordinator perk and sees its ranks, conditions, and synergies
laid out clearly in a structured panel rather than a paragraph wall — that is the
moment. It does not require a new skin. It requires a better panel.

The custom skill tree support is the ecosystem wow moment. LOTD and Thieves Guild
progression mods that currently have no native UI surface getting first-class
constellation treatment is something those mod communities will notice and share.

---

## Open Questions

1. **Perk refunding:** Some mod frameworks allow perk refunds. Does the skills
   screen surface this? Probably as a context menu option on owned perks when
   refunding is enabled by the active framework — not a default feature, not OHUI's
   decision to make, but the UI should support it when the system allows it.

2. **Generated layout quality:** The automatic constellation layout algorithm needs
   real perk tree data to validate against. LOTD and Thieves Guild trees are the
   obvious test cases. The algorithm should be validated against these before launch.

4. **Legendary skills:** Vanilla allows skills to be made Legendary, resetting them
   for perk points. The skills screen should surface this option clearly with its
   consequences explained — what resets, what is kept. Currently the legendary
   option is easy to trigger accidentally with no clear information about what it does.
