# Loading Screen

## Design Intent

OHUI runs the vanilla loading screen faithfully. The rotating model,
the loading bar, the tip text. OHUI does not replace or extend the
loading screen's content.

OHUI ensures that existing custom loading screen mods continue to
work correctly. Mods that replace loading screen assets through
standard mechanisms (NIF replacement, texture replacement, tip text
registration) are compatible by default.

---

## Skinning

The loading bar and tip text presentation are skinnable — styled
consistently with the active OHUI skin. The loading screen content
(3D models, backgrounds) remains vanilla or whatever replacement
mods the player has installed.

---

## Open Questions

1. **SWF override:** The loading screen is an engine-managed SWF.
   Whether OHUI's screen replacement mechanism needs special handling
   for the loading screen context (which runs during cell load, not
   during normal menu operation) needs investigation.
