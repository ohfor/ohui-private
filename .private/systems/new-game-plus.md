# New Game+

**STATUS: PARKED**

This feature is isolated in its own document pending further design
discussion. It is not part of the 1.0 scope. It does not influence
or depend on other OHUI systems until it is taken off hold.

The feature as previously discussed:

- Available from the New Game prompt on the main menu
- Player selects a source save and chooses carry-forward categories
- Categories discussed: perk points, known spells, crafting recipes,
  shout words, map memory, books read, faction standing, LOTD museum
  knowledge
- Each category independently toggleable with concrete descriptions
- Legacy summary generated from source save data
- Ownership split: OHUI owns the configuration UI and cosave data;
  a companion plugin handles the actual gameplay mutations

Open questions when this feature is revisited:
- Heirloom items (depended on provenance stamps, which are cut)
- Faction standing carry-forward feasibility
- Cosave standalone read at main menu (needed for source save browsing)
- Companion plugin scope and boundary
