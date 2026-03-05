# Repository Public/Private Model

The complete project lives in a single private repository. A public-facing repository is a filtered subset, published deliberately.

## Core Mandate

**Everything is committed. Nothing is gitignored for secrecy.**

Working documents, research, decisions, session notes, log analysis, backlog — all live in the repo under version control. The private repo is the complete history of the project.

The public repo is a **projection** — a deliberate, filtered copy of selected directories and files. Publishing is always a conscious choice, never automatic.

## Directory Structure and Visibility

| Path | Private Repo | Public Repo | Contents |
|------|-------------|-------------|----------|
| `src/` | Yes | Yes | C++ source |
| `include/` | Yes | Yes | C++ headers |
| `docs/` | Yes | Yes | User-facing documentation (Build, Architecture, Changelog) |
| `Data/` | Yes | Yes | Game data files (ESP, INI configs, translations) |
| `scripts/` | Yes | Yes | Papyrus sources and compiled scripts |
| `CMakeLists.txt` | Yes | Yes | Build configuration |
| `CMakePresets.json` | Yes | Yes | Build presets |
| `vcpkg.json` | Yes | Yes | Dependencies |
| `version.rc` | Yes | Yes | Windows version resource |
| `LICENSE` | Yes | Yes | License file |
| `README.md` | Yes | Yes | Public-facing project description |
| `CLAUDE.md` | Yes | **No** | AI session context, internal project details |
| `.private/` | Yes | **No** | All working documents (see below) |
| `utilities/` | Yes | Case-by-case | Helper scripts — some may be useful publicly, others contain internal paths |
| `.mcp.json` | Yes | **No** | MCP server config with local paths |

### What Lives in .private/

| Directory | Contents |
|-----------|----------|
| `reference/` | Portable development guides (synced from template) |
| `reference-overrides/` | Project-specific divergences from reference guidance |
| `decisions/` | Architectural decision records |
| `research/` | Investigation logs, disassembly analysis, dead ends |
| `notes/` | Session notes, scratch thinking, design intake documents |
| `backlog.md` | Project backlog (if not in CLAUDE.md) |

## The Sync Rule

The public repo receives everything **except**:

- `.private/` (entire directory)
- `CLAUDE.md`
- `.mcp.json`
- Any file explicitly marked private (project-specific additions)

This is a whitelist-by-default model: new top-level directories are assumed public unless they're under `.private/` or explicitly excluded.

## Implementation Options

The sync can be:

1. **Manual** — copy files to the public repo directory, commit, push
2. **Script** — a sync script that applies the exclusion rules and copies
3. **GitHub Action** — triggered manually or on tag, copies from private to public repo

The mechanism is secondary. What matters is that **no sync happens without deliberate action**. There is no automatic mirroring.

## Why This Model

- **Complete history**: Every document, discussion outcome, and design decision is version-controlled. No information lives only in chat logs or local files.
- **No gitignore games**: Trying to keep sensitive content out of git while working with it daily is fragile and error-prone. Commit everything, control publication separately.
- **Clean public face**: The public repo looks like a normal open-source project. Users see source, docs, and build files. They don't see internal process.
- **Conscious publishing**: Every piece of information that becomes public is a deliberate choice. Research, competitive analysis, session notes, and internal discussions stay private by default.

## CLAUDE.md and the Public Repo

The private repo's `CLAUDE.md` contains detailed project internals — hook tables, API documentation, backlog, compatibility notes, internal architecture details. This is the AI's working document and is not suitable for public consumption.

The public repo gets a `README.md` that describes the mod from a user/developer perspective. These are separate documents with different audiences and different content.

## Working With This Model

**When creating new files**, ask: "Would this be useful or appropriate in the public repo?"

- Source code, build config, user docs → top-level or `docs/`
- Internal analysis, decisions, research, notes → `.private/`
- If unsure → `.private/` (can always promote later, can't unpublish)

**When syncing to public**, review the diff. New top-level files may have appeared that need an explicit include/exclude decision.
