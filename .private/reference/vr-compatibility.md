# VR Compatibility

Building a single DLL for SE/AE/VR is the recommended approach. Here are the pitfalls.

## CMake Configuration

Enable all three runtimes:
```cmake
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL "" FORCE)
```

Runtime detection:
```cpp
if (REL::Module::IsVR()) { ... }
```

## EditorID Lookups Are Broken on VR

`RE::TESForm::LookupByEditorID()` silently returns `nullptr` on VR for every call.

**Root cause**: The EditorID-to-Form hash map uses `RELOCATION_ID(514352, 400509)`, which has no VR offset in CommonLibSSE-NG. The relocation resolves to garbage, so the map is never found.

**Solution**: Always use `TESDataHandler::LookupForm` with hardcoded FormIDs:

```cpp
// BAD - works on SE/AE, silently fails on VR
auto* form = RE::TESForm::LookupByEditorID("MyEditorID");

// GOOD - works on all runtimes
auto* dh = RE::TESDataHandler::GetSingleton();
auto* form = dh->LookupForm(0x800, "MyPlugin.esp");
```

This is the single most common cause of "works on SE but not VR" reports. If your plugin does ANY form lookups, audit every one.

## VR Address Library Gaps

The VR Address Library CSV (shipped with [VR Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/58101)) is community-maintained and incomplete. Some SE/AE Address Library IDs have no VR equivalent.

**Workaround**: Use `REL::VariantID` with hardcoded VR offsets:

```cpp
// SE ID 19274, AE ID 19700, VR offset 0x29f980
auto addr = REL::VariantID(19274, 19700, 0x29f980).address();
```

Find VR offsets in the [skyrim_vr_address_library database](https://github.com/alandtse/skyrim_vr_address_library). Check the status field:
- Status 4: Bit-for-bit identical to SE, safe to use
- Status 3: Manually verified, likely safe
- Status 1-2: Needs verification

## ESL-Flagged Plugins

VR does not natively support ESL-flagged plugins (`.esp` files with the ESL flag). Loading an ESL-flagged plugin on VR without the support mod will crash at startup.

**Requirement for VR users**: [Skyrim VR ESL Support](https://www.nexusmods.com/skyrimspecialedition/mods/106712)

Document this clearly in your mod's requirements section if your ESP uses the ESL flag.

## VTable Offsets

CommonLibSSE-NG assumes VTable offsets are consistent across SE/AE/VR runtimes. This has held true in practice but is an assumption, not a guarantee.

## What "VR Support" Means

Be precise in your claims:

| Claim | Meaning |
|-------|---------|
| "VR compatible" | DLL loads, hooks install, forms resolve on VR. Not gameplay-tested. |
| "VR tested" | Actual gameplay verified by someone on a VR headset. |
| "VR supported" | Bugs reported by VR users will be investigated and fixed. |

Most new plugins should start at "VR compatible" and upgrade to "VR tested" once community feedback arrives.

## VR User Requirements Checklist

Document these in your mod page:
- [ ] SKSE VR (matching VR game version)
- [ ] VR Address Library for SKSE Plugins
- [ ] Skyrim VR ESL Support (if ESPs are ESL-flagged)
- [ ] SkyUI VR (if MCM is used)
