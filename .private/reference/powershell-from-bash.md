# PowerShell from Bash (Claude Code on Windows)

Claude Code's Bash tool runs through a Linux-style shell. For operations involving Windows paths with spaces, environment variable expansion, or file searching, PowerShell is often the cleanest option.

## The Core Pattern

```bash
powershell.exe -NoProfile -Command '...powershell here...'
```

**Single quotes around the command are critical.** They prevent bash from expanding PowerShell's `$` variables (`$env:USERPROFILE`, `$false`, etc.). PowerShell then handles its own variable expansion inside the single-quoted string.

Double quotes inside the PowerShell command are fine — they're PowerShell's own quotes, invisible to bash.

## Environment Variables

| Shell | Syntax | Example |
|-------|--------|---------|
| cmd.exe | `%USERPROFILE%` | `%USERPROFILE%\Documents` |
| PowerShell | `$env:USERPROFILE` | `"$env:USERPROFILE\Documents"` |
| Bash (on Windows) | `$USERPROFILE` | Available but no space handling |

PowerShell's `$env:USERPROFILE` expands inside double-quoted strings, handling spaces in paths (e.g., `C:\Users\Mark Cassidy`) without issues.

## Log File Operations

The SKSE log path pattern appears constantly. Here are the operations that matter:

### Read last N lines (tail)

```bash
powershell.exe -NoProfile -Command 'Get-Content "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" -Tail 30'
```

### Read first N lines (head)

```bash
powershell.exe -NoProfile -Command 'Get-Content "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" | Select-Object -First 20'
```

### Line count

```bash
powershell.exe -NoProfile -Command '(Get-Content "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log").Count'
```

### Search for pattern (grep)

```bash
powershell.exe -NoProfile -Command 'Select-String -Path "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" -Pattern "error" -CaseSensitive:$false'
```

### Search with result limiting

```bash
# Last 10 matches
powershell.exe -NoProfile -Command 'Select-String -Path "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" -Pattern "session" -CaseSensitive:$false | Select-Object -Last 10'

# First 5 matches
powershell.exe -NoProfile -Command 'Select-String -Path "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" -Pattern "hook" | Select-Object -First 5'
```

### Search with context lines (grep -C)

```bash
powershell.exe -NoProfile -Command 'Select-String -Path "$env:USERPROFILE\Documents\My Games\Skyrim Special Edition\SKSE\{PluginName}.log" -Pattern "RemoveItem" -Context 2,2'
```

## File Operations

### Find a file by name

```bash
powershell.exe -NoProfile -Command 'Get-ChildItem -Path "D:\SteamLibrary" -Filter "CraftingInventoryExtender.dll" -Recurse -ErrorAction SilentlyContinue | Select-Object FullName, LastWriteTime'
```

### Check file timestamp (for deploy verification)

```bash
powershell.exe -NoProfile -Command 'Get-Item "$env:SKYRIM_DIR\Data\SKSE\Plugins\{PluginName}.dll" | Select-Object FullName, LastWriteTime, Length'
```

### Compare timestamps (build vs deployed)

```bash
powershell.exe -NoProfile -Command '$src = (Get-Item "build\release\Release\{PluginName}.dll").LastWriteTime; $dst = (Get-Item "D:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\SKSE\Plugins\{PluginName}.dll").LastWriteTime; if ($src -gt $dst) { "BUILD IS NEWER - redeploy needed" } else { "Deployed copy is current" }'
```

## What Fails and Why

| Attempt | Failure |
|---------|---------|
| `powershell.exe -Command "... $env:USERPROFILE ..."` | Bash expands `$env` to empty string before PowerShell sees it |
| `cmd.exe /c "type %USERPROFILE%\...\file.log"` | Works but can't handle patterns, tail, or line limiting |
| Using Read tool with `%USERPROFILE%` | Tool doesn't expand Windows env vars |
| `grep` on Windows paths with spaces | Quoting nightmare; PowerShell's `Select-String` is cleaner |
| `Get-ChildItem` without `-ErrorAction SilentlyContinue` | Permission errors on system folders flood output |

## When to Use What

| Task | Tool |
|------|------|
| Read a file with known absolute path (no env vars, no spaces) | Read tool |
| Read SKSE log (env var in path, spaces) | PowerShell `Get-Content` |
| Search in log file | PowerShell `Select-String` |
| Search codebase files | Grep tool (works fine for repo files) |
| Check file existence/timestamps | PowerShell `Get-Item` / `Test-Path` |
| Find files by name on disk | PowerShell `Get-ChildItem -Recurse` |
| Git operations | `git -C` with forward slashes (see `git-on-windows.md`) |
