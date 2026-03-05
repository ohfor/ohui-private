# Claude Session Context

Quick reference for AI assistants working on this project.

## Repository Model

> Full guide: `.private/reference/repo-public-private.md`

**This is a private repository.** Everything is committed — source code, working documents, research, decisions, session notes. Nothing is gitignored for secrecy.

A separate public repository receives a **filtered subset** of this repo (source, docs, build files). Publishing is always a deliberate, manual action. The `.private/` directory and `CLAUDE.md` are never published.

**When creating files:** if unsure whether something belongs in `.private/` or a public directory, default to `.private/`. Content can be promoted to public later; it cannot be unpublished.

## What This Is

**OHUI** — A complete UI framework for Skyrim SE/AE/VR. Not a SkyUI reskin — a full replacement of the infrastructure, with backwards compatibility for SkyUI-dependent mods on day one.

## Build

> Full setup guide: `docs/Build.md`

**Use `deploy.ps1` for all builds.** Never run cmake commands directly.

```powershell
# Build + deploy DLL
.\scripts\deploy.ps1

# Build + deploy DLL + compile all Papyrus
.\scripts\deploy.ps1 -Papyrus

# Compile specific Papyrus scripts only (skip C++ build)
.\scripts\deploy.ps1 -PapyrusOnly -Scripts OHUI_MCM
```

**First-time setup only** (generates build files — run once, not on every build):
```cmd
cmake --preset release
```

**NEVER reconfigure or rebuild VCPKG** unless there is a specific, confirmed reason (e.g., new dependency added to `vcpkg.json`, corrupted build cache). A full VCPKG rebuild takes 15+ minutes. If a build fails, the cause is almost always a quoting error, missing file, or code error — not a stale VCPKG cache.

## Build Checklist (MANDATORY)

After code changes, ALWAYS complete the full cycle:

1. **C++ changes** -> `.\scripts\deploy.ps1` -> verify DLL timestamp
2. **Papyrus changes** -> `.\scripts\deploy.ps1 -PapyrusOnly` -> verify .pex timestamps updated
3. **Both** -> `.\scripts\deploy.ps1 -Papyrus` (C++ first, then Papyrus)

NEVER assume previous session left things deployed. ALWAYS verify timestamps before asking user to test.

## Release Packaging

```powershell
# Full release build + package
.\scripts\build_package.ps1 -Version 1.0.0

# Skip DLL rebuild (use existing)
.\scripts\build_package.ps1 -SkipBuild
```

Output: `assets/OHUI-Main-v{VERSION}.zip` — upload to Nexus Main Files.

### Release Pipeline (GitHub Actions)

Three-workflow pipeline in `.github/workflows/`:

1. **Prepare Release** (`prepare-release.yml`) — creates orphan `releases/review/X.Y.Z` branch with only public content, verifies versions, validates no private content leaked, builds DLL from staged content
2. **Publish Release** (`publish-release.yml`) — pushes reviewed branch to public repo `ohfor/ohui`, creates version tag
3. **Release** (`release.yml`) — runs in public repo on tag push, builds DLL, packages zip, creates GitHub Release with assets

Requires `PUBLIC_REPO_PAT` secret in private repo settings.

## Git Commits

**No co-author lines.** Do not add `Co-Authored-By` or similar attribution to commit messages.

## Versioning

> Full guide: `.private/reference/versioning.md`

**Current released version on Nexus: v1.0.0**

### Version Files (update all when bumping)
1. `include/Version.h` - MAJOR, MINOR, PATCH
2. `version.rc` - FILEVERSION, PRODUCTVERSION, string versions
3. `CMakeLists.txt` - VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH
4. `vcpkg.json` - version-string

Version bumps happen ONLY when preparing a Nexus release, not after every feature/fix.

### Unreleased Changes
(none)

## Key Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | Plugin entry point, logging, message handling |
| `include/Version.h` | Version constants |
| `include/PCH.h` | Precompiled header |
| `scripts/deploy.ps1` | Build + deploy (DLL and/or Papyrus) |
| `scripts/build_package.ps1` | Release packaging for Nexus |
| `scripts/check_versions.py` | Version consistency across 4 files |
| `DESIGN.md` | Architectural vision and layer model |
| `TASKS.md` | Complete task breakdown (68 tasks, 15 phases) |

## Directory Structure

| Directory | Contents |
|-----------|----------|
| `src/` | C++ source files |
| `include/` | C++ header files |
| `scripts/` | Build, deploy, and utility scripts |
| `docs/` | Public documentation (Build, Architecture, Changelog) |
| `dist/` | Nexus archive layout (game data files shipped to users) |
| `assets/` | Release archives (versioned zips for Nexus upload) |

## Log Location

```
%USERPROFILE%\Documents\My Games\Skyrim Special Edition\SKSE\OHUI.log
```

## Template Reference System

**Template version: 1**

Reference docs in `.private/reference/` are synced from the [skse-template](D:\git\ohfor\skse-template) repo. They represent our current best understanding of SKSE development patterns, learned across all projects.

### Rules

1. **Never edit files in `.private/reference/`** — they are owned by the template
2. **Intentional divergences** go in `.private/reference-overrides/` (see below)
3. **Syncing**: copy `.private/reference/` from the template, bump the version number above
4. **New guidance applies going forward** — a sync does not trigger rework of existing code
5. **If existing code conflicts with updated guidance**, decide: adapt or override

### Override Format

When a project intentionally breaks from reference guidance, create a file in `.private/reference-overrides/` named after the reference doc:

```markdown
# Override: hooking-patterns.md

## VTable hook initialization

Reference recommends lazy init on first menu open.
We initialize at plugin load because [reason].

Decided: 2026-01-28
```

Before applying guidance from any reference doc, check for a corresponding override. **The override wins.**

Overrides should be deleted when they no longer apply (reference caught up, reason evaporated, or project adapted).

## Private Directories

| Directory | Purpose |
|-----------|---------|
| `.private/reference/` | Portable SKSE development guides — **synced from template, do not edit** |
| `.private/reference-overrides/` | Intentional divergences from reference guidance, with rationale |
| `.private/decisions/` | Architectural decision records for this project |
| `.private/notes/` | Session notes, scratch thinking, temporary context |
| `.private/research/` | Investigation logs, disassembly analysis, dead-end documentation |
| `.private/screens/` | 17 screen design docs (inventory, dialogue, skills, etc.) |
| `.private/systems/` | 16 system architecture docs (DSL, data binding, MCM, etc.) |

## Reference Docs

These portable guides live in `.private/reference/` and cover patterns learned from prior projects:

| Doc | Covers |
|-----|--------|
| `mcm-development.md` | Page dispatch, option IDs, pagination, bulk actions, locked options, get/set pattern |
| `mod-detection.md` | ESP lookup, DLL filesystem check, FormID deep integration, compatibility page |
| `distribution-automation.md` | Version consistency, rebuild detection, staging, archive validation |
| `papyrus-workflow.md` | Compilation, .psc/.pex locations, compile loop, SkyUI SDK setup |
| `esp-workflow.md` | When you need an ESP, xEdit, script attachment, ESL flag, FormID table |
| `ini-settings.md` | Custom parser, SimpleIni, config directory pattern, user overrides |
| `hooking-patterns.md` | MinHook, VTable, ABI gotchas, lazy init, loading guards |
| `vr-compatibility.md` | EditorID broken on VR, Address Library gaps, ESL, testing tiers |
| `commonlibsse-ng-gotchas.md` | Version pinning, form lookups, threading, RTTI, event sinks |
| `cosave-handling.md` | Save/load/revert callbacks, versioned format, FormID resolution |
| `papyrus-native-functions.md` | Registration, type marshalling, threading |
| `performance-patterns.md` | Caching, lazy init, hot-path discipline, profiling |
| `translation-support.md` | SkyUI $KEY system, UTF-16 LE files, DLL TranslationService, placeholders |
| `versioning.md` | Version files, bump procedure, semver, consistency enforcement |
| `packaging.md` | Nexus archive structure, verification, common mistakes |
| `esma-tool.md` | ESMA CLI reference, development status, invocation pattern, read/write policy |
| `git-on-windows.md` | git -C pattern, forward slashes, commit -F, cmd.exe pitfalls |
| `powershell-from-bash.md` | Log reading, file search, env vars, Select-String, timestamp checks |
| `project-bootstrap.md` | New project setup sequence, design intake, backlog format, research guidelines |
| `repo-public-private.md` | Private/public repo model, sync rules, directory visibility, publishing mandate |

## Public Docs

| Doc | Covers |
|-----|--------|
| `docs/Build.md` | Prerequisites, VCPKG, configure, build, deploy |
| `docs/Architecture.md` | Plugin design, hooks, data structures |
| `docs/CHANGELOG.md` | Version history |

## What NOT To Try

Document failed approaches here so they aren't re-attempted:

<!-- Example:
- Entry-point hooks on GetItemCount (IDs 16565/50372) -- never called during crafting
- LookupByEditorID on VR -- silently returns null (see vr-compatibility.md)
-->
