# Dialogue Screen

## The Problem

Vanilla Skyrim's dialogue screen is a plain list of topic strings. It has no awareness
of who is speaking, no sense of relationship or history, no visual hierarchy between
topic types, and no way to understand where any given dialogue entry comes from. Every
conversation looks identical regardless of whether you are speaking to a beggar in
Riften or the High King of Skyrim.

SkyUI never touched it. Thirteen years of ports, remasters, and anniversary editions
have not touched it either.

The specific failures:

- No speaker identity. The NPC's name appears in the subtitle but nothing else about
  them is surfaced — relationship, faction, disposition, history with the player.
- No dialogue source attribution. A mod-added dialogue option is visually identical
  to a vanilla one. Players cannot tell what is adding options or why.
- No topic type hierarchy. Quest-critical dialogue, ambient conversation topics, and
  vendor options sit in the same undifferentiated list.
- No conversation history within the session. Once a topic is exhausted it disappears.
  There is no way to re-read what was just said.
- No full sentence preview. Topic strings are truncated summaries, not the actual words
  the player character will say. What you select is not what you hear.
- The presentation is identical for every NPC, every conversation, every context.
  Skinnable differentiation is structurally impossible under the current system.

---

## Design Intent

A conversation should feel like a conversation — with a specific person, in a specific
context, with history and weight. The dialogue screen should communicate who this
person is, what your relationship with them is, and what this exchange means. It should
get out of the way when nothing is happening and be present and informative when it is.

The data contract must be rich enough for skin authors to implement genuinely different
presentation styles — a dialogue wheel, a full cinematic view, a minimalist subtitle
system, a classic RPG topic tree. Same data. Different ControlTemplate. This is the
correct abstraction.

---

## Data Contract

The dialogue screen receives the following data. This is the complete contract skin
authors implement against. All presentation decisions follow from this.

**Speaker:**
- Name
- Race
- Faction affiliations (primary faction label for display)
- Disposition toward player (0–1, normalised)
- Relationship rank (enemy / neutral / friendly / ally)
- Is follower
- Is quest-giver for any active quest
- Portrait / face render (native C++ widget, live 3D render — real or nothing, see Speaker Identity)

**Player:**
- Name (for subtitle attribution)
- Disposition modifiers active (speech perks, charm spells, bribe state)

**Dialogue options (per entry):**
- Full sentence text (the actual words the player character will say, not the topic
  summary string)
- Topic summary string (the short label, for compact presentation modes)
- Type: quest-critical / ambient / vendor / training / bribe / persuade / intimidate /
  farewell / misc
- Is new (not previously selected in this conversation)
- Is exhausted (has been selected, response received)
- Speech challenge: none / persuade / intimidate / bribe — with difficulty indicator
  where applicable
- Source: originating ESP/ESM filename, associated quest FormID and name if any,
  mod display name if available
- Condition result: why this option is visible (always visible, quest stage condition,
  skill condition, item condition — for attribution display)

**Conversation state:**
- History of this session's exchanges (player selections and NPC responses, in order)
- Active quest context (if this conversation is advancing a quest, which one)

---

## Source Attribution

Mod-added dialogue options are visually identical to vanilla options in every current
Skyrim UI. Players who run large mod lists frequently encounter dialogue options they
do not recognise and have no way to identify the source without external research.

OHUI surfaces source attribution on demand. It is never shown by default — it does
not clutter the conversation for players who don't need it. It is available instantly
for players who do.

**Hold to reveal (per option)**
Holding a configurable button while hovering or focusing a dialogue option surfaces
a tooltip showing:
- Source mod display name
- Source ESP/ESM filename
- Associated quest name and current stage, if any
- Condition summary: why this option is currently visible

**Inspect mode (all options)**
A toggle (configurable binding, off by default) that adds a source indicator to all
visible dialogue options simultaneously. For auditing a full conversation. Useful for
mod list curation, debugging unexpected dialogue, and understanding what a mod is
actually adding to an NPC.

**Dialogue history attribution**
In the quest journal's dialogue history view, every line is attributed to its source.
This is the passive version of the same feature — it requires no action during the
conversation and surfaces the information in context after the fact.

Source attribution is one of the features of this screen that no other Skyrim UI has
provided. It is genuinely useful to a significant portion of the player base and
architecturally free given OHUI's access to form data.

---

## Conversation History

Within a session, the full transcript of the current conversation is accessible.
Not just the most recent exchange — the entire conversation from the opening line.

Surfaced as a scrollable history panel, collapsed by default, expandable during the
conversation. Useful for conversations with many branches, long quest dialogue, or
any time the player wants to re-read what was just said before making a choice.

Player lines and NPC lines are visually distinguished. The history is read-only.
It feeds directly into the quest journal's dialogue history system — runtime capture
of the exact choices made.

---

## Full Sentence Preview

Topic summary strings are not the words the player character says. "Persuade him to
leave" becomes "I know you don't want to be here. Neither do I. Let's both walk away."
These are different things. Players selecting dialogue options deserve to know what
their character will actually say before they say it.

OHUI surfaces the full sentence as the primary text of each dialogue option. The topic
summary string remains available for compact presentation modes and as a secondary
label. Default presentation shows the full sentence.

This is a meaningful change to how players engage with dialogue — particularly for
players who care about roleplaying their character consistently, and for players
whose first language is not English who benefit from seeing the full text before
committing.

---

## Speech Challenges

Persuade, intimidate, and bribe options are currently presented as bare topic strings
with no indication of difficulty, success probability, or the factors affecting the
outcome. OHUI surfaces this information inline.

**Per speech challenge option:**
- Challenge type (persuade / intimidate / bribe)
- Difficulty indicator — not a percentage, a meaningful signal. Easy / Moderate /
  Hard / Very Hard. Derived from speech skill, perks, disposition, and relevant
  modifiers.
- Active modifiers — what is currently affecting the outcome. Relevant speech perks,
  active charm effects, disposition state. Surfaced on hold-to-reveal, not by default.
- Bribe cost — shown inline for bribe options

Skin authors implement the visual language for difficulty — colour, icon, bar, or
any other treatment. The data contract exposes a normalised difficulty value (0–1)
and the challenge type. The presentation is the skin's concern.

---

## Speaker Identity

Every conversation is with a specific person. The dialogue screen should reflect that.

**Speaker panel** — present during conversation, not intrusive. Shows:
- NPC name
- Primary faction affiliation or role (Guard, Merchant, Jarl's Steward, etc.)
- Disposition indicator — subtle, not a number. A tonal signal, not a meter.
  Skin implements the visual language. Data contract exposes normalised value (0–1).
- Portrait — live 3D render of the actual NPC as they currently appear — equipped,
injured, transformed if applicable. Native C++ widget. If a meaningful render is
not available for this NPC, the portrait is absent. No placeholder, no silhouette,
no faction icon standing in for a face. Real or nothing. Skins handle the absent
  state by collapsing the portrait area gracefully.
- Active quest marker — if this NPC is a quest giver or key NPC for any active quest,
  indicated clearly.

The same principle applies if the player character is ever surfaced in a dialogue
context — reaction shot, two-shot framing, or any cinematic presentation. It is the
actual player character as currently rendered. No avatar approximations. Real or nothing.

The speaker panel is the element most likely to vary dramatically between skins.
A cinematic skin might make the portrait large and central. A minimalist skin might
reduce it to a name and disposition dot. Both are valid implementations of the same
data contract.

---

## Skin Differentiation

The data contract is deliberately rich to support genuinely different presentation
styles. First-party OHUI skin ships a clean, modern take on the classic topic list
with full sentence preview and speaker panel. Beyond that, skin authors can implement:

**Dialogue wheel** — radial layout of options around a central speaker portrait.
Mass Effect style. The component library's radial layout component supports this
directly.

**Cinematic view** — full speaker portrait, options overlaid at bottom, subtitle
presentation. Emphasises the conversation as a scene.

**Classic RPG tree** — hierarchical topic list with category headers. For players
who prefer the traditional model with better information density.

**Minimalist** — subtitles only, options as compact labels, nothing else on screen.
For immersion-focused players who want the UI to disappear.

All of these are ControlTemplate implementations of the same data contract. Switching
between them is a skin selection. No data changes. No configuration required.

---

## Player Experience Goals

The dialogue screen is one of four surfaces explicitly identified as high-impact wow
moments for OHUI's launch.

**Source attribution** is the feature that will generate community conversation.
"You can see which mod is adding this dialogue option" is immediately understood and
immediately useful to a large portion of the modding community. It gets posted about.

**Full sentence preview** is the feature that changes how players engage with dialogue.
It is noticed within the first conversation and not forgotten.

**The skin differentiation potential** — specifically the dialogue wheel — is the
feature most likely to produce a shareable screenshot or clip. A well-executed
dialogue wheel in Skyrim, never before seen, is a viral moment waiting for a skin
author to build it.

---

## Open Questions

1. **Voiced player lines:** Skyrim does not voice the player character's dialogue
   in most cases. Full sentence preview shows the text. Does it also surface the
   voice type / delivery note for modded voiced dialogue? Probably out of scope
   but worth noting.

2. **Dialogue wheel input:** A dialogue wheel skin requires radial controller
   navigation. The input context stack handles this but the wheel geometry and
   selection model needs design when a skin author builds one.

3. **Companion/follower conversations:** Follower dialogue has a different structure
   to NPC dialogue — more context-sensitive, less topic-based. Does the data contract
   handle both cleanly? Needs validation against the follower dialogue system.

4. **NPC portrait for generic NPCs:** The real-or-nothing principle applies. Generic
   NPCs (bandits, guards, merchants with no unique identity) get a live face render
   if one is available and nothing if not. The data contract exposes a portrait
   availability flag. Skins collapse the portrait area when unavailable. No
   faction/race icon fallback — that would be worse than nothing.
