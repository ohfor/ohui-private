# Performance Patterns

## The Problem

SKSE hooks can sit in hot paths -- functions called hundreds of times per frame during menu rendering. A naive hook implementation can cause visible frame drops when the player opens a crafting menu.

## Inventory Caching

The most impactful optimization for plugins that intercept inventory queries.

### Pattern

Build a complete merged inventory on session start. Serve all subsequent queries from cache.

```cpp
struct CachedInventory {
    std::unordered_map<RE::FormID, int32_t> counts;
    std::vector<RE::InventoryEntryData*> entries;
    bool valid = false;
};

CachedInventory g_cache;

// Build cache once when crafting session starts
void BuildCache() {
    g_cache.counts.clear();
    g_cache.entries.clear();

    // Enumerate all sources (player, containers, followers, etc.)
    for (auto* source : activeSources) {
        for (auto& [form, data] : source->GetInventory()) {
            g_cache.counts[form->GetFormID()] += data.first;
            // ... build merged entry list
        }
    }

    g_cache.valid = true;
}

// Hook returns cached data instead of recalculating
int32_t Hook_GetItemCount(RE::TESObjectREFR* a_ref, ...) {
    if (g_cache.valid) {
        return g_cache.counts[requestedFormID];
    }
    return originalFunc(a_ref, ...);
}
```

### Invalidation

Invalidate the cache when items are added or removed:

```cpp
void Hook_RemoveItem(...) {
    // Do the removal
    actuallyRemoveItem(...);
    // Invalidate cache
    g_cache.valid = false;
}
```

A flag-based approach (set dirty, rebuild on next query) is simpler and just as fast as incremental updates for most use cases.

## Lazy Initialization

Don't compute anything until it's actually needed.

```cpp
bool EnsureSessionActive() {
    if (g_session.active) return true;

    // Check if player is actually at a crafting station
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto handle = player->GetOccupiedFurniture();
    if (handle.native_handle() == 0) return false;

    // NOW initialize -- not before
    g_session.active = true;
    ScanContainers();
    BuildCache();
    return true;
}
```

This matters because menu events don't always fire before the first inventory query.

## Hot-Path Discipline

In functions called per-item during menu rendering:

- **No allocations**: Avoid `std::string`, `std::vector`, `new`, `make_shared` in the hook body
- **No logging**: `logger::trace()` costs microseconds per call; multiply by 500 items
- **No container scanning**: Do this once at session start, not per query
- **Short-circuit early**: Check `g_session.active` as the first thing

```cpp
int32_t Hook_GetItemCount(RE::TESObjectREFR* a_ref, ...) {
    // Fast path: not in a crafting session
    if (!g_session.active) return originalFunc(a_ref, ...);

    // Fast path: not the player
    if (!a_ref->IsPlayerRef()) return originalFunc(a_ref, ...);

    // Now do the actual work
    return GetMergedCount(...);
}
```

## Loading Menu Guard

Not strictly a performance pattern, but critical for stability. Save/load invalidates all cached references:

```cpp
auto* ui = RE::UI::GetSingleton();
if (!ui || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) {
    g_session.Reset();
    return originalFunc(args...);
}
```

Without this guard, stale pointers cause heap corruption that manifests as CTD on the next save reload -- not immediately, making it hard to diagnose.

## Profiling

Scope-based timing for identifying bottlenecks:

```cpp
struct ScopeTimer {
    const char* name;
    std::chrono::high_resolution_clock::time_point start;

    ScopeTimer(const char* n) : name(n), start(std::chrono::high_resolution_clock::now()) {}
    ~ScopeTimer() {
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        if (us > 1000) {  // Only log if > 1ms
            logger::info("[PERF] {} took {}us", name, us);
        }
    }
};

// Usage:
void BuildCache() {
    ScopeTimer timer("BuildCache");
    // ... expensive work ...
}
```

Use this during development to find which hook or initialization step dominates. Remove or gate behind debug logging for release.
