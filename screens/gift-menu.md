# Gift Menu

The gift menu is the container screen in a single-direction gift
configuration. The full `TabbedListPanel` foundation applies — search,
sort, filter, item detail panel, inline item intelligence. The barter
screen's disposition header adapts to relationship context. One staging
panel. Player to NPC only.

## Delta from Container Screen

**Single direction** — items move from player inventory to NPC only.
No taking. The container panel is absent. The player's inventory is
the only list.

**Relationship header** — disposition toward the player, relationship
status, and faction standing where relevant. Simpler than the barter
disposition header — no price modifier, no Speech skill calculation.
The question is not "how much will they pay" but "will they accept this
and will they like it."

**Acceptance filter** — items the NPC will refuse are flagged clearly
and filtered out of the gift list by default. Stolen items, items below
their interest threshold, items the NPC has no use for. The player is
not building a gift from items that will be rejected. Filter toggle
available for players who want to see everything.

**Preference indicator** — items the NPC will particularly appreciate
are highlighted. Known preferences, favourite item types, items with
relationship relevance. A player gifting Lydia a weapon she can use
sees that highlighted. A player gifting Camilla Valerius a book sees
that highlighted.

**Provenance stamp visibility** — dedicated items show their dedication
in the gift staging area. "Crafted by Valdris for Lydia" is visible
before the gift is given. The moment of giving a dedicated item is
confirmed before it is committed.

## What Is Not Here

No transaction footer. No gold balance. No confirm/cancel trade flow.
Gifting is a one-way action — select item, give it. The staging panel
holds the item until the player confirms. Cancel returns it to inventory.
The simplicity is the point.
