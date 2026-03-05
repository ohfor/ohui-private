# OHUI Project Conventions

Project-specific conventions that supplement (and where noted, override)
the portable reference guides in `.private/reference/`.

Decided: 2026-03-05

---

## Error handling

Use `ohui::Result<T>` — a type alias for `std::expected<T, Error>`.
No exceptions in core logic. No silent failures. Every function that
can fail returns a Result.

```cpp
namespace ohui {

enum class ErrorCode {
    ParseError,
    FileNotFound,
    InvalidFormat,
    SizeLimitExceeded,
    DuplicateRegistration,
    // ... extend per subsystem
};

struct Error {
    ErrorCode code;
    std::string message;
};

template<typename T>
using Result = std::expected<T, Error>;

}  // namespace ohui
```

At the DLL boundary (`main.cpp`, `GameBridge.cpp`), Results are
unwrapped, errors are logged, and sensible defaults are used.
Core code returns errors for failures. Core code may use
`ohui::log::debug()` / `ohui::log::trace()` for development
diagnostics. Core code never calls `logger::info/warn/error` —
those are boundary-layer concerns.

**Note:** `std::expected` requires C++23 (MSVC 17.6+, confirmed
available with our toolchain).

---

## Logging

- `info` — plugin lifecycle events (load, data loaded, game loaded/saved)
- `warn` — recoverable issues (unknown USS property, missing token, stale cosave block)
- `error` — unrecoverable issues that disable a subsystem (hook failed, cosave corrupt)
- `debug` — development diagnostics (parsed N rules, resolved N tokens)
- `trace` — per-frame or per-item detail (NEVER in release builds)

Guard trace output behind `#ifdef _DEBUG` or a runtime flag. Never
log in hot paths (see `.private/reference/performance-patterns.md`).

---

## Memory ownership

- **Widget instances** owned by `WidgetRegistry`. Subsystems hold
  non-owning references.
- **USS rule sets** owned by `SkinManager`. Widgets hold `const&`
  to resolved style.
- **Cosave blocks** owned by `CosaveManager`. Subsystems read via
  accessor, write via `CosaveManager::QueueWrite`.
- **Input contexts** owned by `InputContextStack`. Screens push/pop
  but don't own.

General rule: `std::unique_ptr` for owning, raw pointer for
non-owning. `std::shared_ptr` only when lifetime genuinely cannot
be determined statically (expected to be rare).

---

## Naming

- **Namespaces:** `ohui`, `ohui::dsl`, `ohui::widget`, etc.
- **Files:** PascalCase for classes (`USSParser.h`), lowercase for
  non-class headers (`Types.h`)
- **Classes:** PascalCase (`WidgetRegistry`)
- **Methods:** PascalCase (`GetWidget`, `ParseFile`)
- **Member variables:** `m_` prefix (`m_widgets`, `m_tokenStore`)
- **Local variables:** camelCase (`parsedRules`, `blockData`)
- **Constants:** `k` prefix (`kMaxModDataSize`, `kDefaultUpdateRate`)
- **Enums:** PascalCase type, PascalCase values (`ErrorCode::ParseError`)
- **Test files:** `{Class}Tests.cpp`

**Note:** The `m_` member prefix diverges from CommonLibSSE-NG's
bare-name convention. This is an intentional project-level choice
for readability in a large codebase.

---

## String handling

- `std::string` and `std::string_view` for all internal strings
- `RE::BSFixedString` only at the GameBridge boundary (DLL target)
- USS property names, binding keys stored as `std::string`
- Never hold raw `const char*` from BSFixedString
  (see `.private/reference/commonlibsse-ng-gotchas.md`)

---

## Include order

1. Corresponding header (`#include "ohui/dsl/USSParser.h"`)
2. Project headers (`#include "ohui/core/Result.h"`)
3. Third-party headers (`#include <yoga/Yoga.h>`)
4. Standard library headers (`#include <vector>`)

Blank line between each group. CorePCH/PCH covers standard library
and spdlog; don't re-include those in individual files unless needed
for clarity in headers that may be read standalone.

---

## RE:: types boundary

- **RE:: value types are allowed in core headers.** `RE::FormID`
  (a `uint32_t` typedef), `RE::FormType` (an enum), and similar
  data-carrying types may appear in `ohui_core` data structures.

- **RE:: pointer-to-game-object types are banned from core interfaces.**
  `RE::TESForm*`, `RE::Actor*`, etc. must not appear in `ohui_core`
  public APIs. At the bridge boundary, translate game pointers into
  OHUI-owned descriptors.

- **RE:: function calls are banned from ohui_core entirely.** No
  `RE::UI::GetSingleton()`, `RE::PlayerCharacter::GetSingleton()`, etc.
  All such calls live in `GameBridge.cpp` or `main.cpp`.

This is enforced structurally: `ohui_core` uses `CorePCH.h` which does
not include CommonLibSSE headers.
