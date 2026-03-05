# Version Management

## Principle

Version bumps happen **only when preparing a release**, not after every feature or fix. Unreleased changes accumulate in the changelog under `[Unreleased]`.

## Version Files

Four source files contain version numbers and must stay in sync:

| # | File | Location | Format |
|---|------|----------|--------|
| 1 | `include/Version.h` | Lines 4-6 | `MAJOR = X`, `MINOR = Y`, `PATCH = Z` |
| 2 | `version.rc` | Lines 4-5, 17, 22 | `X,Y,Z,0` and `"X.Y.Z.0"` |
| 3 | `CMakeLists.txt` | Lines 4-6 | `VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH` |
| 4 | `vcpkg.json` | Line 4 | `"version-string": "X.Y.Z"` |

If your mod has an MCM, add a 5th file: the Papyrus MCM script (CurrentVersion integer + About page string).

## Bump Procedure

1. Update all 4 (or 5) version files
2. Move `[Unreleased]` entries in CHANGELOG.md to a new version section
3. Update `CLAUDE.md` current version and unreleased section
4. Build Release DLL
5. Commit: `Bump version to vX.Y.Z for Nexus release`
6. Tag: `git tag vX.Y.Z`

## Semantic Versioning

- **MAJOR**: Breaking changes, architecture rework
- **MINOR**: New features, new hooks, new compatibility patches
- **PATCH**: Bug fixes, performance improvements, documentation

## Version Consistency

If using a distribution build script, validate that all version files match before packaging. A mismatch means someone forgot to update one of the files. Fail the build rather than ship mismatched versions.
