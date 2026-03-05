# Message Log

## The Problem

Skyrim has multiple completely separate transient text channels with no awareness of
each other and no persistence beyond their display duration:

- **Subtitles** — dialogue lines, disappear when the line advances
- **Notifications** — top-left corner, disappear after a few seconds
- **Quest update banners** — their own presentation, their own timing
- **Skill increase messages** — yet another channel, yet another presentation
- **System messages** — save confirmations, loading notices, debug output

Each channel is independent. Each has its own presentation, its own timing, its own
display rules. None of them are recoverable once they disappear. A notification that
fires while the player is in dialogue, looking away, or momentarily distracted is
gone. There is no log. There is no history. There is no way to know what you missed.

The practical consequences are felt every session:

- A quest update fires exactly when an NPC says something important. You miss both.
- A skill increase message appears during combat. Gone before you read it.
- A debug or system notification stays on screen for three seconds while you look
  away. Gone. You spend the next ten minutes wondering if you imagined it.
- You want to know what an NPC said two lines ago. It no longer exists.

---

## Design Intent

Every transient text event in the game flows through one infrastructure. The message
log is a single persistent stream that captures everything — subtitles, notifications,
quest updates, skill increases, system messages — typed, ordered, and permanently
accessible for the duration of the session.

Nothing is lost because you looked away. The log was capturing it regardless.

---

## Architecture

### The Stream

A single append-only message stream. Every transient text event in the game is a
publisher into this stream. The stream is the source of truth for all transient
text in OHUI.

**Message properties:**
- Content (text, formatted)
- Type: subtitle_interactive / subtitle_ambient / notification / quest / skill / system / system_ohui / combat / custom
- Timestamp (game time and real time)
- Source: NPC name for subtitles, mod/system for notifications, quest name for
  quest updates
- Priority: low / normal / high / critical
- Lifetime hint: the display duration the message requests (used by HUD widget,
  not by the log itself — the log retains everything)
- Speaker attribution for subtitles (NPC or player)
- Quest attribution where applicable

**Message types are extensible.** Mod authors can publish custom typed messages
into the stream. Custom types participate in the log, the HUD widget filter system,
and the tab structure identically to first-party types.

### Publishers

Every transient text channel in the game is re-implemented as a publisher into the
message stream:

- Skyrim's subtitle system → subtitle messages
- Skyrim's notification system → notification messages
- Quest stage advancement → quest messages
- Skill and level increases → skill messages
- OHUI system events → system messages
- Mod authors via API → custom typed messages

Publishers are fire-and-forget. They post to the stream and are done. The stream
handles persistence, ordering, and delivery to all subscribers.

### Subscribers

Any OHUI component can subscribe to the stream with a type filter. Current
subscribers:

**HUD message widget** — displays recent messages on screen, filtered and styled
by type. The player-facing transient display. Replaces vanilla's notification
system, subtitle display, and quest banner as the unified on-screen channel.

**Quest journal dialogue history** — subscribes to subtitle messages filtered by
active quest context. The stream is the data source for the quest journal's per-quest
conversation record.

**Message log panel** — the full persistent log, accessible on demand. Every message,
every type, fully scrollable, tabbed by type, searchable. Never loses anything.

---

## HUD Message Widget

The on-screen presence of the message log. A configurable viewport into the stream
showing recent messages, fading over time, scrollable on demand.

**Configuration (via edit mode and widget context menu):**
- Position, size, opacity — standard widget properties
- Message types shown (filter by type — show subtitles only, hide combat, etc.)
- Maximum visible messages
- Fade timing per type
- Combat mode / change mode visibility rules (standard widget behaviour)

**Display behaviour:**
- New messages appear at the configured anchor point (top or bottom of widget)
- Messages fade after their lifetime hint expires
- Scrolling back reveals older messages without clearing the feed
- Faded messages are still in the stream — scrolling back reveals them

The HUD message widget is a first-party OHUI widget. It replaces vanilla's
notification display and subtitle positioning. Skin authors can implement entirely
different visual treatments — a chatbox style, a minimal fade overlay, a parchment
scroll aesthetic — all against the same data contract.

---

## Message Log Panel

The full persistent log. Accessible from any menu context via a consistent binding
— the "what did I miss" button.

**Structure:**
- Tabbed by message type: All / Subtitles / Notifications / Quests / Skills /
  System / OHUI / Custom (custom tabs appear when mods register custom types)
- Chronological, newest first by default, reversible
- Searchable across all types
- Each entry shows content, source attribution, and timestamp
- Quest-attributed subtitle entries link directly to the relevant quest journal entry

The message log panel is not a HUD element — it is a UI layer panel, modal, input-
capturing. It can be opened during or after any activity. It is never open by default.
It exists entirely for "what did I miss."

**Session scope:** The log retains all messages for the current play session. On
game exit and reload it retains the last N messages (configurable, default 200) as
a rolling buffer. A new session does not start with a blank log — the tail of the
previous session is present for context.

**Copy to clipboard:**
The message log panel ships with clipboard copy as a first-class affordance. The
two overlapping squares icon — universally understood, no label needed — appears
in three places:

- **Per entry** — copies that single log line. One click, paste directly into
  a Discord message or bug report. No finding files, no opening directories.

- **Panel level** — copies the entire current tab's visible content, or the
  full filtered view if a filter is active. "Copy all OHUI tab entries" gives
  the complete startup diagnostic in a single action.

- **Selection copy** — standard text selection across multiple entries, copied
  as a group. Highlight what you want, copy it. Nothing exotic.

The copy icon is a standard icon token in the component library — skinnable like
everything else, defaulting to the two-square convention because it is what
everyone already knows. The clipboard and the log file on disk are parallel
paths to the same content. The file exists for players who want it. The copy
button exists for everyone else. Neither requires knowing where Skyrim stores
its logs.

**Share to web:**
Adjacent to the copy icon, a share affordance publishes the current tab's content
to a web service and returns a URL. The URL is copied to clipboard automatically.
The player pastes it in Discord, a Nexus comment, or a bug report. One click from
"I have a problem" to "here is my log for someone to look at."

Targets (selectable via a small popover on the share button):
- **Pastebin** — public API, no account required, paste expires after 7 days
  by default. Privacy-respecting, no permanent storage.
- **GitHub Gist** — for players with a GitHub account who prefer it.
- **Copy raw text** — fallback, same as the copy icon but surfaced here for
  players who want to paste manually to a service of their choice.

The share button makes an HTTP call via SKSE networking, receives a URL, copies
it to clipboard. No browser required. No navigating to an external site. The
entire flow happens inside the game.

A future OHUI community paste service is the ideal long-term answer — OHUI-
specific, searchable by the development team, useful for pattern recognition
across support requests. The share button infrastructure supports adding it as
a target when it exists.

**This is not telemetry.**
Telemetry is passive collection without the player's knowledge or explicit action.
The share button is the opposite of that. Nothing leaves the player's machine
until they open the log panel, decide what to share, and click the button.
Nothing is sent automatically. Nothing is collected in the background. Nothing
happens without explicit player initiation.

The player who never clicks the share button has no data leave their machine.
Ever. The player who clicks it has made a deliberate choice to share a specific
piece of content with a specific destination they selected. Those are
categorically different things and the Skyrim community is right to treat them
as such.

OHUI's position is simple and airtight: no data is ever transmitted without
explicit player action. The share button is a clipboard action with an optional
network step. That is all it is.

---

## Relationship to Other Systems

**Dialogue screen** — the conversation history panel in the dialogue screen is a
filtered view of the subtitle stream for the current conversation. The stream is
the data source. The dialogue screen panel is a subscriber.

**Quest journal** — per-quest dialogue history draws from the subtitle stream,
filtered by quest attribution. The stream is the data source. The quest journal
is a subscriber.

**OSK and text input** — text input events are not message stream events. The
OSK is input infrastructure, not output infrastructure. No relationship.

**Notification API for mod authors** — mod authors currently call Skyrim's native
notification function to surface messages. OHUI provides a richer API that publishes
typed, attributed, prioritised messages into the stream. The native notification
function continues to work — it publishes into the notification channel automatically.
Mod authors who want richer messages opt into the OHUI API.

---

## Mod Author API

Mod authors can post messages to the stream with content, type, source
attribution, priority, and lifetime hint. Messages posted via the API
are indistinguishable from first-party messages in the log.

Custom types can be registered with an ID, display label, default
visibility, default lifetime, and colour. Once registered, a custom
type appears as a tab in the message log panel. Players can filter
custom types in the HUD widget. They are first-class participants in
the stream.

---

## Player Experience

The message log is not a headline wow feature. It is infrastructure that makes
everything else more trustworthy. Players stop missing things. Players stop
wondering what just happened. Players stop losing dialogue they wanted to re-read.

The specific moment that lands for every player: something important fires while
they are distracted. They reach for the log. It's there. That's the moment.

Mod authors who adopt the typed API get their messages surfaced consistently,
attributed correctly, and permanently accessible. Their notifications are no longer
competing with vanilla's three-second fire-and-forget system.

---

## Open Questions

1. **Combat floating text:** Damage numbers, hit indicators, and similar combat
   text are a different category — spatially anchored to actors, not screen-space
   notifications. Do they participate in the stream as a combat type, or are they
   handled by a separate system entirely? Probably separate, but the boundary needs
   defining.

2. **Session buffer size:** 200 messages as a default rolling buffer across sessions
   is a guess. Needs validation against typical session message volume. A long
   dialogue-heavy session could generate significantly more than 200 subtitle entries
   alone.

3. ~~**Subtitle stream and rapid dialogue:**~~ Resolved. Ambient NPC barks, ambient
   NPC-to-NPC conversation, and combat taunts are typed as `subtitle_ambient`,
   distinct from `subtitle_interactive` (player-initiated dialogue). Ambient subtitles
   are off by default in the HUD widget and appear under their own tab in the log
   panel. Present for anyone who wants them. Invisible to everyone else. Same
   solution every MMO has used for fifteen years.
