# MCM Development Patterns

## Overview

SkyUI's Mod Configuration Menu (MCM) is how most SKSE plugins expose settings to users. Your MCM script extends `SKI_ConfigBase` and runs in Papyrus, while your settings live in C++ (exposed via native functions). This doc covers the structural patterns, not SkyUI's API basics.

## Architecture: Stateless MCM, DLL as Source of Truth

The MCM script should be **stateless**. It never caches settings — it always reads from and writes to the DLL via native functions. This eliminates an entire class of stale-state bugs.

```
User clicks toggle → MCM calls SetSomething(newValue) → DLL saves to INI
User opens page     → MCM calls GetSomething() → DLL returns current value
```

The DLL auto-saves to INI on every `Set*` call. Papyrus never touches the INI file directly.

## Option ID Management

### The Problem

SkyUI assigns integer IDs when you call `AddToggleOption()`, `AddSliderOption()`, etc. These IDs are only valid for the current page render. If you store them in module-level variables and the user switches pages, stale IDs from the previous page can collide with new ones.

### The Pattern: Reset All IDs at Page Entry

```papyrus
; Module-level option ID variables
int oidEnabled
int oidDistance
int[] oidFilterToggles

Function ResetOptionIDs()
    oidEnabled = -1
    oidDistance = -1
    oidFilterToggles = new int[12]
    ; Reset EVERY option ID variable to -1 (or reinitialize arrays)
EndFunction

Event OnPageReset(string page)
    ResetOptionIDs()   ; Always first
    ; ... render page
EndEvent
```

Every option ID variable must be reset at the start of every `OnPageReset`. No exceptions.

## Page Dispatch

### Structure

```papyrus
Event OnPageReset(string page)
    ResetOptionIDs()
    SetCursorFillMode(TOP_TO_BOTTOM)

    ; Check plugin loaded before rendering anything
    if !MyPlugin_NativeFunctions.IsPluginLoaded()
        AddTextOption("Plugin not loaded", "", OPTION_FLAG_DISABLED)
        return
    endif

    ; Dispatch to page render functions
    if page == "" || page == Pages[0]
        RenderSettingsPage()
    elseif page == Pages[1]
        RenderFilteringPage()
    elseif page == Pages[2]
        RenderCompatibilityPage()
    endif
EndEvent
```

### Page Array Initialization

Define page names as translated keys in an array. Compare against this array, not raw `$KEY` strings — SkyUI translates the array contents.

```papyrus
string[] Pages

Function InitializePages()
    Pages = new string[4]
    Pages[0] = "$MYPLUGIN_PageSettings"
    Pages[1] = "$MYPLUGIN_PageFiltering"
    Pages[2] = "$MYPLUGIN_PageCompatibility"
    Pages[3] = "$MYPLUGIN_PageAbout"
EndFunction

Event OnConfigInit()
    ModName = "MyPlugin"
    CurrentVersion = 100  ; v1.0.0
    InitializePages()
EndEvent
```

## Get/Set Pattern for Options

### Toggle

```papyrus
Event OnOptionSelect(int option)
    if option == oidEnabled
        bool newValue = !MyPlugin_NativeFunctions.GetEnabled()
        MyPlugin_NativeFunctions.SetEnabled(newValue)
        SetToggleOptionValue(option, newValue)
    endif
EndEvent
```

### Slider

```papyrus
Event OnOptionSliderOpen(int option)
    if option == oidDistance
        float current = MyPlugin_NativeFunctions.GetMaxDistance()

        ; Dynamic range expansion — if current value exceeds bounds
        ; (e.g., user edited INI manually), expand range to fit
        float minVal = 500.0
        float maxVal = 5000.0
        if current < minVal
            minVal = current
        endif
        if current > maxVal
            maxVal = current
        endif

        SetSliderDialogStartValue(current)
        SetSliderDialogDefaultValue(3000.0)
        SetSliderDialogRange(minVal, maxVal)
        SetSliderDialogInterval(100.0)
    endif
EndEvent

Event OnOptionSliderAccept(int option, float value)
    if option == oidDistance
        MyPlugin_NativeFunctions.SetMaxDistance(value)
        SetSliderOptionValue(option, value, "{0} units")
        ForcePageReset()  ; Refresh dependent display elements
    endif
EndEvent
```

### Confirmation Dialog

```papyrus
elseif option == oidClearButton
    bool confirm = ShowMessage("$MYPLUGIN_MsgConfirmClear", true)
    if confirm
        int cleared = MyPlugin_NativeFunctions.ClearData()
        ShowMessage("$MYPLUGIN_FmtCleared{" + cleared + "}", false)
        ForcePageReset()
    endif
```

## Pagination for Large Lists

When a page displays more items than fit on screen (containers, spells, NPCs), paginate with parallel arrays.

### Data Structure

Three parallel arrays map display position to data:

```papyrus
int[] oidItemOptions        ; Option IDs returned by AddTextOption
int[] itemGlobalIndices     ; Maps option index → global data index
int itemPageNum = 0         ; Current page
int ITEMS_PAGE_SIZE = 20    ; Items per page
```

### Rendering

```papyrus
Function RenderItemsPage()
    int totalCount = MyPlugin_NativeFunctions.GetItemCount()
    int totalPages = ((totalCount - 1) / ITEMS_PAGE_SIZE) + 1

    ; Clamp page to valid range
    if itemPageNum >= totalPages
        itemPageNum = totalPages - 1
    endif

    ; Fetch one page of data from DLL
    string[] names = MyPlugin_NativeFunctions.GetItemNames(itemPageNum, ITEMS_PAGE_SIZE)
    int[] states = MyPlugin_NativeFunctions.GetItemStates(itemPageNum, ITEMS_PAGE_SIZE)

    ; Render with index mapping
    int optIdx = 0
    int i = 0
    while i < names.Length
        oidItemOptions[optIdx] = AddTextOption(names[i], GetStateLabel(states[i]))
        itemGlobalIndices[optIdx] = (itemPageNum * ITEMS_PAGE_SIZE) + i
        optIdx += 1
        i += 1
    endwhile

    ; Navigation
    if totalPages > 1
        AddHeaderOption("$MYPLUGIN_FmtPage{" + (itemPageNum + 1) + "}")
        if itemPageNum > 0
            oidPrevPage = AddTextOption("$MYPLUGIN_PrevPage", "")
        endif
        if itemPageNum < totalPages - 1
            oidNextPage = AddTextOption("$MYPLUGIN_NextPage", "")
        endif
    endif
EndFunction
```

### Click Handling

```papyrus
; In OnOptionSelect:
int ci = 0
while ci < oidItemOptions.Length
    if option == oidItemOptions[ci] && oidItemOptions[ci] != 0
        int globalIdx = itemGlobalIndices[ci]
        MyPlugin_NativeFunctions.CycleItemState(globalIdx)
        ForcePageReset()  ; Re-fetch and redraw
        return
    endif
    ci += 1
endwhile
```

Always re-fetch data on click — don't cache page data across interactions.

## Bulk Actions

For pages with many toggles (e.g., form type filters per station):

```papyrus
; Station selector dropdown
oidStationMenu = AddMenuOption("$MYPLUGIN_Station", stationNames[selectedStation])

; Bulk buttons
oidSelectAll = AddTextOption("$MYPLUGIN_SelectAll", "$MYPLUGIN_Apply")
oidSelectNone = AddTextOption("$MYPLUGIN_SelectNone", "$MYPLUGIN_Apply")
oidDefaults = AddTextOption("$MYPLUGIN_Defaults", "$MYPLUGIN_Apply")

; Handler
Event OnOptionMenuAccept(int option, int index)
    if option == oidStationMenu
        selectedStation = index
        ForcePageReset()
    endif
EndEvent

; In OnOptionSelect:
if option == oidSelectAll
    MyPlugin_NativeFunctions.SetAllFilters(selectedStation, true)
    ForcePageReset()
elseif option == oidSelectNone
    MyPlugin_NativeFunctions.SetAllFilters(selectedStation, false)
    ForcePageReset()
elseif option == oidDefaults
    MyPlugin_NativeFunctions.ResetFilterDefaults(selectedStation)
    ForcePageReset()
endif
```

The DLL handles the bulk operation atomically. MCM just triggers and re-renders.

## Locked/Constrained Options

Some options are invalid in certain contexts (e.g., weapon filters disabled at enchanting stations because enchanting requires items in player inventory).

### Rendering

```papyrus
bool locked = MyPlugin_NativeFunctions.IsOptionLocked(station, formType)
if locked
    oidFilterToggles[i] = AddToggleOption(label, false, OPTION_FLAG_DISABLED)
else
    oidFilterToggles[i] = AddToggleOption(label, currentValue)
endif
```

### Highlight Text

```papyrus
Event OnOptionHighlight(int option)
    ; Show why the option is locked
    if MyPlugin_NativeFunctions.IsOptionLocked(station, formType)
        SetInfoText("$MYPLUGIN_InfoLocked")
    else
        SetInfoText("$MYPLUGIN_InfoNormal")
    endif
EndEvent
```

### Multi-Layer Enforcement

Don't rely solely on the MCM grey-out. Enforce constraints at every layer:

1. **MCM render**: `OPTION_FLAG_DISABLED` (visual)
2. **MCM click handler**: Check lock status, ignore click if locked
3. **MCM highlight**: Show lock explanation
4. **DLL Set function**: Reject invalid values
5. **DLL bulk function**: Skip locked options in Select All/None

## Dynamic Text with Translation Keys

SkyUI supports `$KEY{value}` for inline substitution:

```papyrus
; Translation file: $MYPLUGIN_FmtCount	Items: {0}
AddTextOption("$MYPLUGIN_FmtCount{" + count + "}", "")
```

Papyrus concatenates the value into the key string at runtime. SkyUI resolves `$MYPLUGIN_FmtCount` from the translation file and substitutes `{0}` with the embedded value.

Limitation: One substitution per key with this syntax. For multiple values, use separate keys or the DLL-side `TranslateFormat` pattern (see `translation-support.md`).

## Version Migration

### OnVersionUpdate

SkyUI calls `OnVersionUpdate` when your `CurrentVersion` integer increases:

```papyrus
int Property CurrentVersion Auto  ; Set in OnConfigInit

Event OnConfigInit()
    ModName = "MyPlugin"
    CurrentVersion = 120  ; v1.2.0 → 120
    InitializePages()
    InitializeArrays()
EndEvent

Event OnVersionUpdate(int a_version)
    InitializePages()
    InitializeArrays()
EndEvent
```

### Defensive Guards

Some saves may miss `OnVersionUpdate` (edge cases with save/load timing). Guard at page entry:

```papyrus
Event OnPageReset(string page)
    ; Defensive: rebuild arrays if they look wrong
    if Pages.Length != 4 || !stationNames || stationNames.Length != 4
        InitializePages()
        InitializeArrays()
    endif

    ResetOptionIDs()
    ; ... render
EndEvent
```

This handles saves that were created before an update and loaded without triggering the version event.
