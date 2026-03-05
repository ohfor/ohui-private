# ESMA - ESP/ESM Manipulator

External CLI tool for reading and writing Bethesda plugin files (ESP/ESM/ESL).

**Location**: `D:\Git\ohfor\esp-manip\build\release-windows\Release\esma.exe`
**Version**: 0.1.0 (active development)
**Source**: `D:\Git\ohfor\esp-manip\`

## Development Status

ESMA is under active development. Read operations are stable and validated against 974 plugins (2.3M records). Write operations exist but are not yet trusted for production use.

| Capability | Status | Notes |
|------------|--------|-------|
| Read: plugin info | Stable | Headers, masters, flags, record counts |
| Read: record listing | Stable | Type filter, pagination (limit/offset) |
| Read: record details | Stable | 128/128 SSE record types parsed |
| Read: JSON output | Partial | Works for `info`, `types`, `list-records`; not yet for `get-record` |
| Write: create empty plugin | Exists | `--author`, `--master`, `--esl` flags |
| Write: roundtrip test | Exists | Level 1 (passthrough) byte-identical; Level 2 (re-serialize) TES4 only |
| Write: apply (patch/generate) | Planned | D26 unified write command not yet implemented in CLI |
| MCP server mode | Planned | `--mcp` flag designed but not implemented |

### Policy

- **Use ESMA for all read operations** — it is faster and more reliable than Python scripts
- **Do not use ESMA for write operations** until explicitly approved — fall back to Python scripts for ESP generation
- **If an ESMA command fails**, fall back to generating a Python script

## CLI Reference

### Global Options (before subcommand)

| Flag | Description |
|------|-------------|
| `--json`, `-j` | JSON output for piping |
| `--verbose`, `-v` | Verbose output |
| `--quiet`, `-q` | Suppress non-essential output |
| `--game`, `-g` | Game target (e.g., `sse`). Auto-detected on read |

**Important**: Global flags go BEFORE the subcommand. `-j` after the subcommand will error.

```cmd
esma --json info plugin.esp    # correct
esma info -j plugin.esp        # ERROR
```

### Commands

#### `info <path>`

Show plugin header information.

```cmd
esma info "D:\path\to\Plugin.esp"
```

Output:
```
Plugin: CraftingInventoryExtender.esp
  Version: 1.71
  Records: 16
  Flags: ESL
  Author: Ohfor
  Masters:
    - Skyrim.esm
  Indexed records: 10
```

JSON output (`--json`):
```json
{"filename":"CraftingInventoryExtender.esp","version":1.71,"records":16,"masters":["Skyrim.esm"]}
```

#### `types <path>`

List record types with counts.

```cmd
esma types "D:\path\to\Plugin.esp"
```

Output:
```
Type      Count
------ --------
KYWD          1
GLOB          1
MGEF          2
SPEL          2
QUST          1
EFSH          3
```

JSON: array of `{"type":"KYWD","count":1}` objects.

#### `list-records <path> [--type TYPE] [--limit N] [--offset N]`

List records with optional filtering and pagination.

```cmd
esma list-records Plugin.esp                    # all records
esma list-records Plugin.esp --type SPEL        # only SPEL records
esma list-records Plugin.esp --limit 10         # first 10
esma list-records Plugin.esp --limit 5 --offset 3   # records 4-8
```

Output:
```
Type   FormID         Offset     Size
------ ---------- ---------- --------
SPEL   01000802         1065      156
SPEL   01000804         1245      148

Showing 1-2 of 2 records
```

JSON: array of `{"type":"SPEL","formid":"01000802","offset":1065,"size":156}` objects.

#### `get-record <path> <formid>`

Get full details for a specific record by FormID.

```cmd
esma get-record Plugin.esp 01000802
```

Output varies by record type. Example for SPEL:
```
Record: 01000802 (SPEL)
  Size: 156 bytes
  Editor ID: SCIE_TogglePower
  Name: [String ID 45494353]
  Type: Lesser Power
  Magicka Cost: 0
  Cast Type: Fire and Forget
  Target: Self
  Effects (1):
    - 01000801 (mag: 0.0, dur: 0s)
```

Supports comma-separated FormIDs for batch lookup (per D27):
```cmd
esma get-record Plugin.esp 01000802,01000804
```

**Known gap**: `--json` flag does not yet produce JSON for this command.

#### `create <path> [--author TEXT] [--master TEXT...] [--esl]`

Create a new empty plugin file. Write operation — **do not use without approval**.

```cmd
esma create MyMod.esp --author "Author" --master Skyrim.esm --esl
```

#### `roundtrip <path> <output> [--level 1|2]`

Test round-trip fidelity: load plugin, save copy, compare bytes.

- Level 1: Passthrough (byte-identical copy)
- Level 2: Parse and re-serialize (TES4 header only currently)

## Invocation Pattern

ESMA must be called via `cmd.exe` from bash environments:

```bash
cmd.exe /c "D:\Git\ohfor\esp-manip\build\release-windows\Release\esma.exe info \"D:\path\to\Plugin.esp\"" 2>&1
```

Quote paths with escaped double quotes inside the `cmd.exe /c` wrapper.

## Planned Features (from PLAN.md)

These are designed but not yet implemented in the CLI:

- **`apply` command** (D26): Unified write — consume structured JSON input to create/patch plugins
- **`search` / `find-editorid`**: Fuzzy and exact EditorID lookup
- **`find-refs`** (D27): Find records referencing a given FormID
- **`diff`** (D27): Record-level diff between two plugins
- **`find-conditions`**: Find records using a specific condition function
- **MCP server mode**: JSON-RPC on stdio for AI tool integration
- **YAML output**: Via transformer layer (D25)

## When to Use ESMA vs Alternatives

| Task | Tool |
|------|------|
| Read ESP plugin info, records, types | ESMA |
| Look up a FormID in a mod's ESP | ESMA `get-record` |
| List all records of a type | ESMA `list-records --type` |
| Generate a new ESP (e.g., recipe patch) | Python script (until ESMA write is approved) |
| Check FormID across load order (in-game plugins) | Vortex MCP `vortex_find_plugins_by_formid` |
| Inspect installed mod's plugin | Vortex MCP `vortex_inspect_plugin` |
| Parse ESP for container FormIDs (INI generation) | `utilities/parse_esp.py` (SCIE-specific) or ESMA |
