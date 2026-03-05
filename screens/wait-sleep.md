# Wait and Sleep Screen

## The Problem

The Wait and Sleep screens are two of the most utilitarian and unloved surfaces in
Skyrim. A time slider, a confirm button, and nothing else. They have been exactly
that since Morrowind.

They sit at a uniquely valuable moment in the player's session — the player is
paused, not doing anything else, and about to make a decision with real gameplay
consequences. The game already knows everything relevant to that decision. None
of it is surfaced.

The specific failures:

- No danger awareness. The game knows the threat level of the current location.
  The player does not, at the moment they need to.
- No survival context. Temperature, fatigue, hunger — all the survival variables
  affected by sleeping here for this long — are invisible at the point of decision.
- No upcoming event awareness. Sleeping past a timed quest window or scheduled
  event is silent. The player discovers the consequence after the fact.
- No extended wait. Vanilla caps the wait slider at 24 hours. Waiting three days
  for a respawn, advancing to a specific date, or skipping to a different season
  requires repeated 24-hour waits.
- No location context. Whether this is an owned bed, a bedroll, a tavern, or a
  patch of ground affects the rest bonus received. This is never communicated
  at the sleep screen.

---

## Design Intent

The Wait and Sleep screens surface what the game already knows about the player's
current situation before they commit to a decision with consequences. No new
gameplay systems are introduced. The information exists in the engine. OHUI
presents it at the moment it is relevant.

---

## Danger Indicator

The game tracks threat level for every location. A just-cleared bandit camp has
a respawn risk. An owned house in a city has none. A wilderness cell has ambient
encounter chances. An inn has a safety rating. None of this is communicated at
the sleep screen.

OHUI surfaces a danger indicator as a primary element of the sleep screen —
visible before the player confirms sleep, not after they wake up to find a
bandit standing over them.

**Indicator levels:**
- **Safe** — owned property, inn with paid room, fully secured location
- **Low Risk** — cleared location, friendly camp, low encounter probability
- **Caution** — wilderness cell, unowned shelter, moderate encounter probability
- **Dangerous** — active enemy proximity, high respawn risk, known hostile location
- **Do Not Sleep Here** — actively hostile location, enemies nearby, near-certain
  interruption

The indicator is a signal, not a precise probability. The game's internal encounter
systems are complex and not reducible to a clean percentage. A meaningful qualitative
signal is more honest and more useful than a spuriously precise number.

For locations where the risk is non-trivial, a brief context note accompanies the
indicator — "recently cleared location, respawn possible" or "wilderness cell,
ambient encounters active." Enough to understand why, not so much that it becomes
noise.

---

## Survival Mode Context

Survival mode tracks temperature, fatigue, hunger, and other variables that are
directly affected by where and how long the player sleeps. This information is
relevant at the exact moment the player opens the sleep screen. It is currently
invisible there.

OHUI surfaces survival state as a contextual panel on the sleep screen:

**Temperature** — current location temperature and projected temperature during
the sleep period. If sleeping here for the requested duration will result in
cold damage or hypothermia, the screen says so before the player confirms.

The canonical example: sleeping on the Throat of the World in a minidress with
no campfire active. The game knows this is going to end badly. OHUI tells the
player before they commit, not after they wake up frozen.

**Fatigue and hunger** — current state and projected state after sleeping for
the requested duration. Will this sleep fully restore fatigue? Will hunger
become critical during the sleep period?

**Bed quality** — bedroll, owned bed, inn room, noble bed. Each provides
different rest quality in survival mode. Surfaced clearly so the player
understands what they are getting before they sleep.

**Active warmth sources** — campfire nearby, warmth enchantment active, warmth
spell active. Whether these will persist through the sleep duration. If a campfire
will burn out before the sleep ends, the screen notes it.

Survival context is only shown when survival mode is active. Non-survival players
see none of this panel. The screen adapts to the player's active game mode.

---

## Upcoming Events

The game schedules events, quest triggers, and NPC appointments on the game
calendar. Sleeping or waiting past these moments is silent — the player discovers
missed events after the fact, if at all.

OHUI surfaces upcoming events that fall within the requested wait or sleep period:

**Quest time windows** — if a timed quest requires action before the sleep period
ends, the screen flags it. "Sleeping this long will advance past the deadline for
[quest name]." The player can shorten the duration or cancel.

**Scheduled NPC appointments** — if a quest NPC is expected somewhere at a
specific time during the sleep period, and missing that window has consequences,
the screen notes it.

**Game calendar events** — festivals, scheduled occurrences, and similar events
that fall within the sleep period. Informational rather than warning — the player
may want to sleep through them or may want to be awake for them.

Upcoming events are surfaced as a collapsible list — present but not intrusive.
The player who never looks at it is not inconvenienced. The player who glances
at it before confirming gets information that was previously invisible.

---

## Extended Wait Duration

Vanilla caps the wait slider at 24 hours. Players who need to wait longer — for
a location to respawn, to advance to a specific game date, to skip to a different
season, to synchronise with an NPC schedule — must make repeated 24-hour waits
with no way to target a specific time.

OHUI removes the 24-hour cap. The wait screen offers:

**Extended slider** — beyond 24 hours, the slider extends to cover multiple days.
Configurable maximum (default 72 hours, player-adjustable).

**Time picker** — a direct input for target time. "Wait until 8am tomorrow."
"Wait until the 15th of Hearthfire." The player specifies a destination rather
than a duration. Calculated automatically from current game time.

**Quick increments** — one-tap buttons for common wait durations. 1 hour, 8 hours,
24 hours, 3 days. Configurable. The most common case should be the fastest.

The time picker uses the OSK on controller. On keyboard/mouse it is a direct
text input field.

---

## Character State Warnings

The sleep screen is character-state-aware. Whatever the player's current condition
— vampire, werewolf, diseased, survival-stressed, cursed — the screen surfaces
warnings relevant to that state before the player confirms sleep. The game already
knows all of this. OHUI presents it at the moment it matters.

**Vampire — blood thirst:**
A vampire at critical hunger who sleeps wakes up in the same or worse state.
Sleeping does not reduce blood thirst. A stage 4 vampire sleeping 8 hours wakes
at stage 4 plus additional starvation — potentially unable to interact with NPCs,
potentially hostile on sight to everyone nearby.

"You are critically blood starved. Sleeping will not reduce your hunger. You may
be unable to interact with NPCs when you wake."

The warning scales with hunger state. A well-fed vampire sees nothing. A hungry
vampire sees a caution. A critically starved vampire sees a prominent warning
that cannot be missed.

**Vampire — sun exposure on wake:**
This is the cruel one. The player sets an 8 hour sleep duration, sleeps through
the night safely, wakes at 10am, steps outside, and dies. The screen knew this
was going to happen. It said nothing.

OHUI calculates sun exposure risk from available information: current game time,
requested sleep duration, sunrise and sunset times, current location, and whether
the wake location has exterior exposure. If the player will wake during daylight
hours in or near an exterior location without sun protection active, the screen
warns them before they confirm.

"Warning: you will wake at 10:14am. Exterior locations will be lethal without
sun protection active."

A vampire player seeing this warning for the first time will feel genuinely
understood by the UI. It is a small thing that demonstrates the screen knows
who they are and what their situation means.

**Werewolf — lunar cycle:**
For mod frameworks that tie werewolf transformation to lunar cycles (Moonlight
Tales and similar), sleeping through a full moon may trigger an involuntary
transformation. The screen flags this when the sleep duration covers a lunar
event that the active framework considers significant.

**Disease state:**
Active diseases that worsen over time may progress during sleep. If sleeping
the requested duration will advance a disease to a more severe stage, the screen
notes it. Not a reason not to sleep — just information the player should have.

**The principle:**
Character state warnings are a category, not a fixed list. The specific warnings
shipped at launch reflect vanilla states and the most common mod frameworks.
The underlying system is extensible — mod authors running custom character state
systems can register warnings that surface on the sleep screen when their
conditions are met. A hunger mod, a sanity mod, a curse framework — any system
that has a "sleeping will make this worse" condition can surface it here.

---

## Rest Bonus Information

The type of bed or resting location determines the rest bonus received. This is
a game mechanic that affects character progression but is never communicated at
the moment of decision.

OHUI surfaces rest bonus information on the sleep screen. Rest bonuses depend
on both bed quality and sleep duration — sleeping for too few hours grants no
bonus regardless of bed quality. The exact hour thresholds should be verified
against game data but approximate requirements are 6 hours for Rested and 8
hours for Well Rested and Lover's Comfort.

The rest bonus indicator and the duration slider are a live feedback loop.
As the player adjusts duration, the indicator updates in real time to reflect
what bonus they will actually receive. A player who sets 3 hours sees no bonus.
A player who sets 6 hours in a quality bed sees Rested. A player who sets 8
hours in their home with their spouse present sees Lover's Comfort. The feedback
is immediate and accurate before the player confirms.

- **Well Rested** — quality owned bed or inn room, minimum hours met
- **Rested** — bedroll or basic shelter, minimum hours met
- **No bonus** — insufficient hours, or unowned/low-quality location
- **Lover's Comfort** — spouse present in home, minimum hours met. The screen
  notes this when applicable — a small touch that acknowledges the player's
  in-game relationships without being intrusive.

The quick increment buttons reflect the bonus they will deliver in context:
"8 hours — Well Rested" or "8 hours — Lover's Comfort" depending on the
current location and whether a spouse is present. The label is live, not static.

For players running survival mode the rest quality also maps to recovery rates
for survival stats. Surfaced as part of the survival context panel.

---

## Wait vs Sleep Distinction

Wait and Sleep are functionally different in Skyrim — Wait passes time without
the player character resting, Sleep restores health and grants rest bonuses.
The two screens are currently nearly identical in presentation despite this
difference.

OHUI presents them distinctly:

**Wait screen** — focused on time advancement. Extended duration controls,
upcoming events, and danger indicator (can you safely wait here?). No rest
bonus information — waiting does not grant rest bonuses.

**Sleep screen** — focused on recovery and consequences. Danger indicator,
survival context, rest bonus information, upcoming events. Time control is
secondary to the recovery decision.

The distinction is clear from the first moment each screen opens. Players who
have always treated them as the same thing will understand the difference
immediately from the screen's emphasis.

---

## Player Experience Goals

The Wait and Sleep screens are not wow features. They are quality of life
corrections that make two very frequently used screens trustworthy for the
first time.

The danger indicator is the feature players will notice and share — specifically
the survival mode version. "The sleep screen told me I was going to freeze to
death on the Throat of the World before I confirmed" is a screenshot moment.
It is also the kind of thoughtful detail that signals OHUI understands how
Skyrim is actually played.

The extended wait duration is the long-requested correction that generates
quiet appreciation. Players who have been doing repeated 24-hour waits for
years will feel the relief immediately.

---

## Open Questions

1. **Danger indicator data sources:** The game's encounter and respawn systems
   are not fully exposed via SKSE. The danger indicator may be approximated
   from available data — cell flags, cleared status, nearby actor detection,
   location keywords — rather than reading internal encounter probability directly.
   The signal needs to be reliable enough to be trusted. If it cannot be reliable,
   it should not ship. A wrong danger indicator erodes trust more than no indicator.

2. **Survival mode detection:** Survival mode may be vanilla or mod-implemented
   (Frostfall, iNeed, Sunhelm, etc.). The survival context panel should adapt to
   whichever survival system is active. A registration API for survival mod authors
   to surface their data into the sleep screen context panel is probably warranted.

3. **Time picker date format:** Game date format varies by localisation. The time
   picker input must respect the active language's date conventions.

4. **Sleep interruption:** If the danger indicator warns of risk and the player
   sleeps anyway, sleep interruption (being woken by an attack) should feel
   connected to the warning that was given. Not a design requirement — a note
   for whoever implements the danger indicator to consider.
