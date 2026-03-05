# Git on Windows (from Claude Code)

Claude Code runs bash commands through a Linux-style shell on Windows. This creates quoting and path issues that silently fail. These patterns are learned from repeated real failures.

## The Core Problem

Claude Code's Bash tool runs through `/usr/bin/bash`. Windows paths with backslashes and `cmd.exe` quoting don't mix cleanly with bash's own quoting. Every git operation in a non-CWD repo hits this.

## Pattern 1: Use git -C (Preferred)

`git -C <path>` changes to the directory before running the command. No `cd`, no `cmd.exe`, no quoting hell.

```bash
git -C D:/git/ohfor/skse-template status
git -C D:/git/ohfor/skse-template log --oneline -5
git -C D:/git/ohfor/skse-template add CLAUDE.md docs/Build.md
git -C D:/git/ohfor/skse-template diff
```

**Use forward slashes.** Bash interprets backslashes as escape characters. `D:/git/ohfor/skse-template` works; `D:\git\ohfor\skse-template` does not.

This handles all read operations and most write operations (add, status, diff, log, etc).

## Pattern 2: Commit Messages with -F (For Commits)

Commit messages with spaces break in every quoting scheme when passing through cmd.exe or nested bash. Write the message to a temp file and use `-F`:

```bash
# Write message to file first (use Write tool or echo with single quotes)
git -C D:/git/ohfor/skse-template commit -F /path/to/commitmsg.txt
```

For the scratchpad path, use forward slashes and the short username path.

## Pattern 3: cmd.exe Fallback (When Needed)

Some operations genuinely need cmd.exe (e.g., running Windows-only tools). When you must use it:

```bash
# Use & not && (cmd.exe parses && differently with nested quotes)
cmd.exe /c "cd /d D:\git\ohfor\skse-template & git status"

# NEVER nest quotes for commit messages — they get eaten
# BAD: cmd.exe /c "cd /d path & git commit -m "message with spaces""
# The inner quotes terminate the outer quotes

# Use -F for commits instead
cmd.exe /c "cd /d D:\git\ohfor\skse-template & git commit -F C:\path\to\msg.txt"
```

## What Fails and Why

| Attempt | Failure |
|---------|---------|
| `cd D:\path && git status` (bare bash) | Bash can't parse backslash paths; `cd` fails silently or interprets `\g` as escape |
| `cmd.exe /c "... && git commit -m "msg""` | `&&` inside outer quotes breaks cmd.exe parsing; second `"` terminates the command string |
| `cmd.exe /c "... & git commit -m "words here""` | Inner quotes with spaces still get eaten by cmd.exe |
| `git -C D:\path\to\repo status` | Backslashes interpreted as escapes by bash; `\s`, `\t`, `\r` become special characters |

## Quick Reference

| Operation | Command |
|-----------|---------|
| Status | `git -C D:/git/ohfor/project status` |
| Log | `git -C D:/git/ohfor/project log --oneline -5` |
| Diff | `git -C D:/git/ohfor/project diff` |
| Add files | `git -C D:/git/ohfor/project add file1 file2` |
| Commit | Write msg to file, then `git -C D:/git/ohfor/project commit -F /path/to/msg.txt` |
| Amend | Write msg to file, then `git -C D:/git/ohfor/project commit --amend -F /path/to/msg.txt` |

## CWD Operations

When the repo IS the current working directory, none of this applies. Plain `git status`, `git commit -m "message"` etc. work fine because there's no path escaping involved. The problems only arise when operating on a repo that isn't the CWD.
