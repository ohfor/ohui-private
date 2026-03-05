# Cosave Handling

SKSE's cosave system lets you persist data alongside the player's save game. Data is stored in `.skse` files next to the `.ess` save.

## Registration

Register during `SKSEPluginLoad` (before any save/load can occur):

```cpp
auto serialization = SKSE::GetSerializationInterface();
serialization->SetUniqueID('MYPL');  // 4-char unique ID (must be globally unique)
serialization->SetSaveCallback(OnGameSaved);
serialization->SetLoadCallback(OnGameLoaded);
serialization->SetRevertCallback(OnRevert);
```

The 4-character ID is a FourCC code. Choose something unique to your plugin. Collisions with other mods will corrupt save data.

## Save Callback

```cpp
void OnGameSaved(SKSE::SerializationInterface* a_intfc) {
    constexpr uint32_t kDataVersion = 1;
    constexpr uint32_t kRecordType = 'DATA';

    if (!a_intfc->OpenRecord(kRecordType, kDataVersion)) {
        logger::error("Failed to open cosave record");
        return;
    }

    // Write your data
    uint32_t count = myData.size();
    a_intfc->WriteRecordData(&count, sizeof(count));

    for (auto& entry : myData) {
        a_intfc->WriteRecordData(&entry.formID, sizeof(entry.formID));
        a_intfc->WriteRecordData(&entry.state, sizeof(entry.state));
    }
}
```

## Load Callback

```cpp
void OnGameLoaded(SKSE::SerializationInterface* a_intfc) {
    uint32_t type, version, length;

    while (a_intfc->GetNextRecordInfo(type, version, length)) {
        switch (type) {
            case 'DATA':
                ReadData(a_intfc, version);
                break;
            default:
                logger::warn("Unknown cosave record type: {:X}", type);
                break;
        }
    }
}

void ReadData(SKSE::SerializationInterface* a_intfc, uint32_t a_version) {
    if (a_version > kCurrentDataVersion) {
        logger::warn("Cosave data version {} is newer than supported {}, skipping",
                     a_version, kCurrentDataVersion);
        return;  // Forward compatibility: don't crash on unknown future versions
    }

    uint32_t count = 0;
    a_intfc->ReadRecordData(&count, sizeof(count));

    for (uint32_t i = 0; i < count; i++) {
        RE::FormID savedFormID = 0;
        a_intfc->ReadRecordData(&savedFormID, sizeof(savedFormID));

        // CRITICAL: Resolve FormID through serialization interface
        // Load order may have changed since the save was created
        RE::FormID resolvedFormID = 0;
        if (!a_intfc->ResolveFormID(savedFormID, resolvedFormID)) {
            logger::warn("Failed to resolve FormID {:08X}, skipping", savedFormID);
            // Still read remaining fields to keep stream position correct
            uint8_t dummy;
            a_intfc->ReadRecordData(&dummy, sizeof(dummy));
            continue;
        }

        uint8_t state = 0;
        a_intfc->ReadRecordData(&state, sizeof(state));

        myData.emplace_back(resolvedFormID, state);
    }
}
```

## Revert Callback

**Critical.** Called when the player loads a save or starts a new game. Without this, your state accumulates across save loads.

```cpp
void OnRevert(SKSE::SerializationInterface*) {
    myData.clear();
    // Reset ALL persistent state to defaults
}
```

Test this by: saving, changing state, loading the earlier save. Your state should match the earlier save, not carry over.

## Versioned Format Pattern

Design your save format to handle upgrades:

```cpp
// v1 format: just FormID + state byte
// v2 format: FormID + state byte + name string

void ReadData(SKSE::SerializationInterface* a_intfc, uint32_t a_version) {
    // ... read base fields (v1) ...

    if (a_version >= 2) {
        // Read v2 additions
        uint16_t nameLen = 0;
        a_intfc->ReadRecordData(&nameLen, sizeof(nameLen));
        // ... read name string ...
    }
}
```

Increment `kDataVersion` when changing the format. Always handle older versions gracefully.
