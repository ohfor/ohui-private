# Project Bootstrap

How to start a new SKSE mod project from the template.

## Sequence

### Phase 1: Skeleton

1. Create a new repo on GitHub, clone locally
2. Copy the template files in (from `skse-template` on disk or remote)
3. Customize placeholders in CLAUDE.md:
   - `{Plugin Name}`, `{PluginName}`, `{SKYRIM_DIR}`, `{ScriptName}`
   - Set initial version to v0.1.0
   - Fill in "What This Is" with a one-line description
4. Update CMakeLists.txt project name
5. Update vcpkg.json name and description
6. Build and deploy the empty DLL
7. Verify it loads — check SKSE log for `{PluginName} loaded successfully`
8. Commit: "Initial project scaffold from template vN"

**Gate**: Do not proceed to Phase 2 until the DLL loads in-game. Everything after this assumes a working build pipeline.

### Phase 2: Design Intake

1. Copy research/design documents into `.private/notes/` (not `decisions/` — nothing is decided yet)
2. Read all design documents
3. Extract a project backlog (see format below)
4. Flag any implementation assumptions embedded in the research for later validation
5. Identify major systems needed (MCM, hooks, cosave, ESP, translation, etc.)
6. Write the backlog to CLAUDE.md or a dedicated `.private/backlog.md`
7. Commit: "Add research documents and initial backlog"

### Phase 3: Implementation Planning

Pick up backlog items. For each significant piece of work:
1. Read the relevant reference docs
2. Check for any reference overrides from prior decisions
3. Investigate the actual game systems involved (Address Library IDs, VTable layouts, form types)
4. Validate or reject any implementation assumptions from the research documents
5. Plan the approach, get approval, build it

## Research Document Guidelines

Research documents describe **what the mod should achieve**, not how to build it.

**Good research content** (intent and requirements):
- What does this mod do from the player's perspective?
- What game systems does it interact with?
- Known mods it must be compatible with or inspired by
- Hard constraints (VR support, no ESP, performance requirements)
- User-facing features and behaviors

**Implementation details that may appear in research** (assumptions to validate):
- Specific hooks, function IDs, VTable offsets
- Data structure choices (JSON vs binary, cosave vs file)
- Specific APIs or libraries to use
- Class hierarchies or code organization

When implementation details appear in research, extract them as **assumptions to validate** during Phase 3 — not as decisions. They were written without seeing actual project constraints and may not survive contact with reality.

## Backlog Format

The backlog lives in CLAUDE.md (for small projects) or `.private/backlog.md` (for larger ones).

```markdown
## Backlog

| # | Priority | Item | Status | Notes |
|---|----------|------|--------|-------|
| 1 | Goal | Brief description of what, not how | Pending | |
| 2 | Goal | Another core requirement | Pending | |
| 3 | Target | Intended but descopeable | Pending | |
| 4 | Optional | Nice to have | Pending | |
```

### Priority Levels

- **Goal**: The mod does not ship without this. Core functionality that defines what the mod is.
- **Target**: We intend to do this and it's part of the vision, but it can be descoped if the architecture doesn't support it cleanly or if it introduces disproportionate complexity.
- **Optional**: Nice to have. Pursue if it falls out naturally from the architecture. Don't force it.

### Status Values

- **Pending**: Not started
- **In Progress**: Actively being worked on
- **Done**: Complete and tested
- **Cut**: Explicitly descoped, with reason
- **Blocked**: Waiting on external dependency or information

### Rules

- Items describe outcomes, not implementations ("Players can craft from nearby containers" not "Hook GetContainerItemCount")
- New items can be added at any time as we discover scope
- Priority can shift — a Target can become a Goal or get Cut based on what we learn
- When an item is completed, add a brief note about the approach taken (for future reference)
- When an item is cut, always record why

## Template Version Tracking

When bootstrapping, record the template version in CLAUDE.md:

```markdown
**Template reference version: N**
```

This allows future syncs of reference documentation. See the Template Reference System section in CLAUDE.md for the full workflow.
