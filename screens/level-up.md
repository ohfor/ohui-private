# Level Up Screen

## The Problem

The level up screen asks the player to choose between Health, Magicka, and
Stamina and provides almost no information to support that choice. "+10 to
the selected attribute" is the entirety of the decision context. Each choice
has consequences beyond the primary value that the screen never mentions.
Players discover them through the wiki, through experimentation, or not at all.

OHUI's level up screen tells the full truth about each choice at the moment
the choice is being made.

---

## Attribute Choice Panel

Each attribute option expands to show the complete consequence of choosing it
— not just the primary value increase but every downstream effect, expressed
as concrete before/after numbers for this specific player right now.

### Health

```
HEALTH  +10
────────────────────────────────────────
Max health:          340 → 350
Regen per second:    unchanged
```

Health is the straightforward choice. The screen confirms the new total and
notes any active effects or perks that scale with max health where relevant.
For most players at most levels this is a simple confirmation. For players
with specific health-scaling builds it surfaces the relevant consequences.

### Magicka

```
MAGICKA  +10
────────────────────────────────────────
Max magicka:         180 → 190
Fireball (cost 152): ×1 cast → ×1 cast   (no change at this increment)
Flames (cost 14):    ×12 casts → ×13 casts
Novice Conjuration:  cost 91 → ×2 casts
Active ward cap:     unchanged
```

The spell cast count examples are drawn from the player's favourited or
recently used spells — the ones most likely to be relevant to this player's
current build. Not every spell in the magic list. The two or three spells
they are actually casting. The screen knows what spells the player uses.

### Stamina

```
STAMINA  +10
────────────────────────────────────────
Max stamina:         200 → 210
Carry weight:        300 → 305   (+5)
Sprint duration:     ~14s → ~15s  (approx, varies with active effects)
Power attack cost:   unchanged
```

The carry weight consequence is the one the screen has never surfaced. It
is a real and significant secondary effect of every Stamina point — 5 carry
weight per 10 Stamina — and it is completely invisible in the vanilla screen.
Players who level Stamina specifically for carry weight management deserve
to have the screen confirm that is what they are doing. Players who did not
know this existed discover it here at the moment it is relevant.

Sprint duration is approximate — it varies with active effects, perks, and
encumbrance. The screen surfaces it as an indication rather than a precise
figure and labels it accordingly.

---

## Perk Point Notification

Every level grants a perk point. The level up screen surfaces what new
perks just became available as a result of skill levels reached since the
last level up.

Not a full perk tree. Not a comprehensive list of every perk the player
could take. Just the perks that unlocked this level — skill thresholds
crossed, prerequisites now met — presented as a brief notification:

```
New perks available this level:
  Smithing 50 reached — Orcish Smithing now available
  Archery 40 reached  — Eagle Eye now available
```

Tapping any entry navigates directly to that perk in the Skills screen.
The player can go spend their perk point immediately without closing the
level up screen, navigating to the journal, finding the skills screen,
finding the right constellation, and locating the perk.

If no new skill thresholds were crossed this level the notification is
absent. It appears only when there is something worth saying.

---

## Skill Progress Summary

A secondary panel showing which skills contributed XP toward this level —
the skills the player actually used since the last level up, with a brief
indication of how much they progressed.

```
Skills that contributed this level:
  One-Handed    62 → 67   ████████████████░░░░  (next perk: 70)
  Smithing      48 → 51   ████████████░░░░░░░░  (Orcish Smithing at 50 ✓)
  Sneak         33 → 35   ████████░░░░░░░░░░░░
```

This is not a full skill sheet — that lives in the Skills screen. It is
a level recap. What the player did this level, what it moved, what they
are approaching. The "next perk" indicator for each skill makes the
progress meaningful — not just a number going up but a concrete milestone
in view.

Skills that did not progress this level are not shown. The panel is a
summary of activity, not a complete skill list.

---

## Active Effects at Level Up

If the player has active effects that scale with the chosen attribute —
enchantments, standing stone bonuses, blessings, perk effects — the
screen notes the updated value after the attribute choice is applied.

```
Stamina selected.
  Atronach Stone (magicka absorb):  unchanged
  Fortify Carry Weight enchantment: unchanged
  Stamina regen (Respite perk):     now restores 15 health per sprint
```

Only relevant effects are noted. Effects that do not interact with the
chosen attribute are not listed. The screen does not produce a wall of
active effects on every level up — it surfaces only what the level up
decision actually changed.

---

## Skill Level Up Notifications

Skills that levelled during normal gameplay already generate notification
messages via the message log system. The level up screen does not
duplicate these — they are already in the log. What the level up screen
adds is the perk availability signal, which is a level-up-moment concern
rather than a mid-gameplay notification.

---

## Design Principle

The level up screen is a decision surface. The player is making a choice
with permanent consequences for their character. The screen should give
them everything they need to make that choice informed — not everything
that could possibly be said about their character, but the specific delta
between the options in front of them, expressed in concrete terms that
connect to how they actually play.

Stamina increases carry weight. The screen says so. Magicka lets you cast
your favourite spell one more time before resting. The screen shows which
spell and how many times. Health moves your death threshold. The screen
shows the new number.

That is the complete design ambition for this screen. It is modest in
scope and significant in impact.
