# Localisation

## Purpose

Visual consistency and feature completeness mean nothing to a player
who cannot read the UI. Localisation is a day-one requirement, not
a post-launch consideration.

**Start:** the component library and DSL exist. Strings need to be
displayable in any language, including RTL languages, with correct
plural forms, correct glyph rendering, and without any component
needing to know what language is active.

**End:** any string in any OHUI component or mod-authored widget
renders correctly in all of Skyrim's 13 official languages, with
correct pluralisation, correct bidirectional text, and a guaranteed
fallback so no player ever sees a tofu box or a raw string key.

---

## String Definition Format

Strings are defined with a namespaced key, a default value, and
optional positional parameters for runtime substitution. Based on
the approach established in SCIE and SLID, proven in production.

```
$OHUI_ConfirmDelete        "Are you sure you want to delete {0}?"
$OHUI_ItemCountRange       "You need between {0} and {1} items"
$OHUI_HealthLabel          "Health"
$OHUI_CarryWeightDisplay   "{0} / {1}"
```

Mod authors define their own strings in the same format, namespaced
to their mod:

```
$MYMOD_LimitWarning        "You need between {0} and {1} of these"
$MYMOD_FeatureEnabled      "{0} is now active"
```

OHUI resolves string keys at render time, substituting positional
parameters and returning the appropriate translation for the active
language. The default value in the definition is the English text
and the authoring reference.

---

## Fallback Chain

If a string key has no translation for the active language, OHUI
falls back to the default value in the string definition. This means:

- Untranslated mods are always readable in the definition language
- Partially translated mods show translated strings where available,
  default text elsewhere
- No string key ever renders as a raw key or an empty string

The fallback is silent — no visual indicator that a string is
untranslated. Players see text. Translators can find untranslated
strings via the debug export (see Diagnostics).

---

## Pluralisation

Positional parameters alone cannot handle pluralisation across
languages. "You have {0} item" vs "You have {0} items" is a
two-form problem in English. Some languages have up to six plural
forms. OHUI supports plural variants via plural form syntax:

```
$OHUI_ItemCount {
    one   "You have {0} item"
    other "You have {0} items"
}
```

Translation files supply as many plural forms as the target language
requires. OHUI selects the correct form at runtime based on the
numeric value and the plural rules for the active language. Plural
rules are defined per language in OHUI's localisation data — the
same CLDR plural rules used by every major i18n library.

Languages with complex plural systems (Russian, Polish, Arabic) are
handled correctly without mod authors needing to know the rules. The
author declares the semantic variants. OHUI applies the rules.

---

## Language File Format

Translation files are plain text, one string per line, following the
key/value format established in SCIE/SLID:

```
$OHUI_HealthLabel          "Gesundheit"
$OHUI_ConfirmDelete        "Möchten Sie {0} wirklich löschen?"
$OHUI_ItemCount_one        "Du hast {0} Gegenstand"
$OHUI_ItemCount_other      "Du hast {0} Gegenstände"
```

Language files are discovered and loaded automatically based on the
active language setting. No code changes are required to add a new
language to any mod that uses OHUI's localisation system.

OHUI's own strings ship with English translations. Community
translators produce additional language files as separate mod assets.
Mod authors ship their default language alongside their mod.

---

## Bidirectional Text — LTR and RTL

LTR/RTL support is designed in from day one. Retrofitting
bidirectional support onto a system that assumed left-to-right
everywhere is a nightmare. OHUI avoids it by treating text direction
as a layout primitive.

### Structural Mirroring

Yoga, OHUI's layout engine, has a native `direction` property that
mirrors the entire layout tree for RTL languages. This handles
structural mirroring automatically:
- Columns that flow right-to-left
- Elements that anchor to the opposite edge
- Flex containers that reverse their main axis

Skin authors use a semantic directional model (`start` / `end` rather
than `left` / `right`). OHUI resolves the physical direction at
render time based on the active language.

```css
/* Correct — semantic direction */
padding-inline-start: 16px;
margin-inline-end: 8px;

/* Avoid — hardcoded direction */
padding-left: 16px;
margin-right: 8px;
```

UI elements that are inherently directional — arrows, chevrons,
progress bars that fill in a direction — are automatically mirrored
in RTL contexts.

### Text Rendering

Arabic and Hebrew require:
- Right-to-left glyph ordering
- Bidirectional text runs (mixed LTR/RTL content in one string)
- Arabic ligature and contextual glyph shaping

OHUI's text renderer supports Unicode Bidirectional Algorithm (UBA)
processing. This is a hard requirement for Arabic and Hebrew support.
Implementing the UBA correctly is Code's responsibility. The design
requirement is stated here and is non-negotiable.

---

## Font System

### Functional Base Font — Noto Sans

OHUI ships **Noto Sans** as its functional base font. Google's
"No Tofu" typeface, designed to provide complete Unicode coverage
across every script and language. Licensed under the SIL Open Font
License — freely redistributable without restriction.

OHUI ships the **complete** Noto Sans family — Regular and Bold
weights, all scripts, CJK included. No subsetting. Google made the
inclusion decisions carefully. OHUI has no basis for second-guessing
them. A player whose language requires a glyph that is in Noto will
see it rendered correctly.

The complete two-weight bundle is approximately 110–115MB. In the
context of a mod sitting at the bottom of a 100GB collection, this
is not a meaningful concern.

**Why OHUI owns its font stack rather than delegating to Skyrim's
engine fonts:** SkyUI ships no fonts. It draws on Skyrim's pre-
rasterised engine fonts, baked into BSA archives. Every SkyUI
localisation is bounded by Bethesda's font choices. The cottage
industry of Skyrim font replacement mods exists precisely because
those choices are limited. All of those mods work within the engine's
constraints. OHUI replaces the stack entirely.

### Scripts Required for Official Language Coverage

| Language | Script |
|----------|--------|
| English, French, German, Spanish, Italian, Polish, Brazilian Portuguese | Latin |
| Russian | Cyrillic |
| Japanese | Hiragana, Katakana, CJK |
| Simplified Chinese | CJK |
| Traditional Chinese | CJK |
| Korean | Hangul, CJK |

Noto Sans covers every script in this table completely.

### Skin Display Fonts

Skins may define their own display fonts for decorative typography —
headings, labels, UI chrome. A Norse-inspired skin will not use the
same typeface as the default. Skin authors bundle their chosen font
and declare it in their USS.

Skin display fonts are decorative only. They are not required to
provide full Unicode coverage. For any glyph a skin font cannot
render, OHUI falls back to Noto Sans automatically. The player always
sees text. Never a tofu box.

Skin font licensing is the skin author's responsibility.

### Emoji — Noto Emoji

OHUI ships **Noto Emoji** — both monochrome outline and full colour
variants. Licensed under SIL Open Font License. Freely redistributable.

Mod authors will use emoji in strings — notification text, MCM
descriptions, item names. If a string contains an emoji codepoint,
OHUI renders it correctly. Supporting emoji is a localisation
correctness requirement, not a cosmetic feature.

Colour emoji rendering requires CBDT/CBLC or COLRv1 glyph table
support in the text renderer. Whether and how this is implemented
is a Code concern. Both monochrome and colour are required. Monochrome
remains available as a fallback regardless of colour rendering capability.

### Font Stack Resolution

At render time OHUI resolves glyphs through a three-level stack:

1. **Skin display font** — if defined and glyph is present
2. **Noto Sans** — fallback for any glyph the skin font cannot render,
   and the sole font when no skin display font is defined
3. **Noto Emoji** — fallback for emoji codepoints

Full coverage is guaranteed regardless of active skin. Always text.
Never a tofu box.

---

## Resolution and Scaling

### Reference Resolution

OHUI authors against **1920×1080**. All widgets, menus, and components
are designed in this coordinate space. This is a meaningful upgrade
over SkyUI's implicit 1280×720 baseline. The vast majority of the
player base is at or above 1080p. Scaling down from 1080p remains
clean. Scaling up from 720p does not.

### Two-Stage Scaling

**Stage 1 — Resolution scaling:** OHUI's 1080p canvas scales to
the player's actual display resolution. At 1080p the factor is 1.0.
At 4K it is 2.0. At 720p it is 0.75. Applied automatically.

**Stage 2 — UI scale:** A player-controlled slider, 50%–200%, applied
on top of resolution scaling. The standard model used by most modern
games. Default 100%.

### Aspect Ratios

Ultrawide and non-standard aspect ratios are solved by the layout
profile system, not the renderer. OHUI ships first-party layout
presets for common aspect ratios:

- **16:9** — default reference layout
- **21:9** — ultrawide
- **32:9** — super ultrawide
- **16:10** — Steam Deck and some monitors

On first launch OHUI detects the display aspect ratio and activates
the appropriate preset. Community presets can be shared as mod assets.
Ultrawide is not a special case — it is a layout configuration that
ships in the box.

---

## Diagnostics

### Untranslated String Export

A debug export available in MCM2 Maintenance produces a list of all
string keys in the current mod list that have no translation for the
active language. Output is a plain text file listing key, default
value, and which mod owns it. Tool for translators, not players.

### Missing Glyph Report

If a glyph cannot be rendered even after the full font stack fallback
(which should not happen with Noto), OHUI logs the codepoint and the
string key at error level. A tofu box appearing in the UI is a bug
and is treated as one.
