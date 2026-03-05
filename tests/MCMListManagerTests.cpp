#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCMListManager.h"
#include "ohui/mcm/MCMRegistrationEngine.h"

using namespace ohui;
using namespace ohui::mcm;

// Helper: register 3 mods into an engine
static void RegisterThreeMods(MCMRegistrationEngine& reg) {
    reg.RegisterMod("ModC");
    reg.RegisterMod("ModA");
    reg.RegisterMod("ModB");
    reg.AddPage("ModA", "General");
    reg.AddPage("ModB", "General");
    reg.AddPage("ModC", "General");
}

// 1. SyncWithRegistry creates entries for all registered mods
TEST_CASE("SyncWithRegistry creates entries for all registered mods", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();
    CHECK(list.EntryCount() == 3);
    CHECK(list.HasEntry("ModA"));
    CHECK(list.HasEntry("ModB"));
    CHECK(list.HasEntry("ModC"));
}

// 2. SyncWithRegistry removes entries for unregistered mods
TEST_CASE("SyncWithRegistry removes entries for unregistered mods", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    reg.UnregisterMod("ModB");
    list.SyncWithRegistry();
    CHECK(list.EntryCount() == 2);
    CHECK(!list.HasEntry("ModB"));
}

// 3. SyncWithRegistry preserves existing entries for still-registered mods
TEST_CASE("SyncWithRegistry preserves existing entries", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetDisplayName("ModA", "Renamed A");
    list.SetHidden("ModC", true);

    reg.RegisterMod("ModD");
    reg.AddPage("ModD", "General");
    list.SyncWithRegistry();

    CHECK(list.GetDisplayName("ModA") == "Renamed A");
    CHECK(list.IsHidden("ModC"));
    CHECK(list.HasEntry("ModD"));
}

// 4. SetHidden hides a mod
TEST_CASE("SetHidden hides a mod", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto result = list.SetHidden("ModA", true);
    REQUIRE(result.has_value());
    CHECK(list.IsHidden("ModA"));
}

// 5. SetHidden on unknown mod returns NotRegistered
TEST_CASE("SetHidden on unknown mod returns NotRegistered", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto result = list.SetHidden("Unknown", true);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

// 6. IsHidden returns correct state
TEST_CASE("IsHidden returns correct state", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    CHECK(!list.IsHidden("ModA"));
    list.SetHidden("ModA", true);
    CHECK(list.IsHidden("ModA"));
    list.SetHidden("ModA", false);
    CHECK(!list.IsHidden("ModA"));
}

// 7. GetSortedList excludes hidden by default
TEST_CASE("GetSortedList excludes hidden by default", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetHidden("ModB", true);
    auto sorted = list.GetSortedList();
    CHECK(sorted.size() == 2);
    for (const auto& entry : sorted) {
        CHECK(entry.modName != "ModB");
    }
}

// 8. GetSortedList includes hidden when includeHidden=true
TEST_CASE("GetSortedList includes hidden when includeHidden=true", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetHidden("ModB", true);
    auto sorted = list.GetSortedList(true);
    CHECK(sorted.size() == 3);
}

// 9. SetDisplayName changes display name
TEST_CASE("SetDisplayName changes display name", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto result = list.SetDisplayName("ModA", "My Cool Mod");
    REQUIRE(result.has_value());
    CHECK(list.GetDisplayName("ModA") == "My Cool Mod");
}

// 10. SetDisplayName on unknown mod returns NotRegistered
TEST_CASE("SetDisplayName on unknown mod returns NotRegistered", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto result = list.SetDisplayName("Unknown", "Name");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

// 11. ClearDisplayName reverts to mod name
TEST_CASE("ClearDisplayName reverts to mod name", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetDisplayName("ModA", "Renamed");
    list.ClearDisplayName("ModA");
    CHECK(list.GetDisplayName("ModA") == "ModA");
}

// 12. GetDisplayName returns override when set
TEST_CASE("GetDisplayName returns override when set", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetDisplayName("ModA", "Override");
    CHECK(list.GetDisplayName("ModA") == "Override");
}

// 13. GetDisplayName returns mod name when no override
TEST_CASE("GetDisplayName returns mod name when no override", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    CHECK(list.GetDisplayName("ModA") == "ModA");
}

// 14. SetManualOrder sets position
TEST_CASE("SetManualOrder sets position", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetManualOrder("ModA", 10);
    auto* entry = list.GetEntry("ModA");
    REQUIRE(entry != nullptr);
    CHECK(entry->manualOrder == 10);
}

// 15. SwapOrder swaps two mods' positions
TEST_CASE("SwapOrder swaps two mods positions", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetManualOrder("ModA", 1);
    list.SetManualOrder("ModB", 2);

    auto result = list.SwapOrder("ModA", "ModB");
    REQUIRE(result.has_value());

    auto* a = list.GetEntry("ModA");
    auto* b = list.GetEntry("ModB");
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a->manualOrder == 2);
    CHECK(b->manualOrder == 1);
}

// 16. MoveToPosition adjusts surrounding entries
TEST_CASE("MoveToPosition adjusts surrounding entries", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetManualOrder("ModA", 0);
    list.SetManualOrder("ModB", 1);
    list.SetManualOrder("ModC", 2);

    // Move ModA from position 0 to position 2
    auto result = list.MoveToPosition("ModA", 2);
    REQUIRE(result.has_value());

    auto* a = list.GetEntry("ModA");
    auto* b = list.GetEntry("ModB");
    auto* c = list.GetEntry("ModC");
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    CHECK(a->manualOrder == 2);
    CHECK(b->manualOrder == 0);
    CHECK(c->manualOrder == 1);
}

// 17. Alphabetical sort -- mods sorted by display name
TEST_CASE("Alphabetical sort -- mods sorted by display name", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetSortMode(MCMSortMode::Alphabetical);
    auto sorted = list.GetSortedList();
    REQUIRE(sorted.size() == 3);
    CHECK(sorted[0].displayName == "ModA");
    CHECK(sorted[1].displayName == "ModB");
    CHECK(sorted[2].displayName == "ModC");
}

// 18. Manual sort -- mods sorted by manual order
TEST_CASE("Manual sort -- mods sorted by manual order", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetManualOrder("ModC", 0);
    list.SetManualOrder("ModA", 1);
    list.SetManualOrder("ModB", 2);
    list.SetSortMode(MCMSortMode::Manual);

    auto sorted = list.GetSortedList();
    REQUIRE(sorted.size() == 3);
    CHECK(sorted[0].modName == "ModC");
    CHECK(sorted[1].modName == "ModA");
    CHECK(sorted[2].modName == "ModB");
}

// 19. MRU sort -- mods sorted by lastOpenedTimestamp descending
TEST_CASE("MRU sort -- mods sorted by lastOpenedTimestamp descending", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.RecordOpen("ModB");
    list.RecordOpen("ModC");
    list.RecordOpen("ModA");
    list.SetSortMode(MCMSortMode::MostRecentlyUsed);

    auto sorted = list.GetSortedList();
    REQUIRE(sorted.size() == 3);
    CHECK(sorted[0].modName == "ModA");
    CHECK(sorted[1].modName == "ModC");
    CHECK(sorted[2].modName == "ModB");
}

// 20. RecordOpen updates timestamp
TEST_CASE("RecordOpen updates timestamp", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto* before = list.GetEntry("ModA");
    REQUIRE(before != nullptr);
    CHECK(before->lastOpenedTimestamp == 0);

    list.RecordOpen("ModA");
    auto* after = list.GetEntry("ModA");
    REQUIRE(after != nullptr);
    CHECK(after->lastOpenedTimestamp > 0);
}

// 21. SetSortMode changes mode, GetSortedList reflects it
TEST_CASE("SetSortMode changes mode, GetSortedList reflects it", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetSortMode(MCMSortMode::Manual);
    CHECK(list.GetSortMode() == MCMSortMode::Manual);

    list.SetSortMode(MCMSortMode::Alphabetical);
    CHECK(list.GetSortMode() == MCMSortMode::Alphabetical);
}

// 22. Serialize produces non-empty buffer
TEST_CASE("Serialize produces non-empty buffer", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto data = list.Serialize();
    CHECK(!data.empty());
}

// 23. Deserialize restores exact state (round-trip)
TEST_CASE("Deserialize restores exact state (round-trip)", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetDisplayName("ModA", "Custom A");
    list.SetHidden("ModB", true);
    list.SetManualOrder("ModC", 5);
    list.RecordOpen("ModA");
    list.SetSortMode(MCMSortMode::Manual);

    auto data = list.Serialize();

    MCMListManager loaded(reg);
    auto result = loaded.Deserialize(data);
    REQUIRE(result.has_value());

    CHECK(loaded.EntryCount() == 3);
    CHECK(loaded.GetDisplayName("ModA") == "Custom A");
    CHECK(loaded.IsHidden("ModB"));
    auto* entryC = loaded.GetEntry("ModC");
    REQUIRE(entryC != nullptr);
    CHECK(entryC->manualOrder == 5);
    auto* entryA = loaded.GetEntry("ModA");
    REQUIRE(entryA != nullptr);
    CHECK(entryA->lastOpenedTimestamp > 0);
    CHECK(loaded.GetSortMode() == MCMSortMode::Manual);
}

// 24. Deserialize empty data produces empty state
TEST_CASE("Deserialize empty data produces empty state", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    auto result = list.Deserialize({});
    REQUIRE(result.has_value());
    CHECK(list.EntryCount() == 0);
}

// 25. Deserialize corrupt data returns InvalidFormat
TEST_CASE("Deserialize corrupt data returns InvalidFormat", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    std::vector<uint8_t> corrupt = {0xFF};
    auto result = list.Deserialize(corrupt);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidFormat);
}

// 26. Sort mode persisted through serialize/deserialize round-trip
TEST_CASE("Sort mode persisted through serialize/deserialize round-trip", "[mcm-list]") {
    MCMRegistrationEngine reg;
    RegisterThreeMods(reg);
    MCMListManager list(reg);
    list.SyncWithRegistry();

    list.SetSortMode(MCMSortMode::MostRecentlyUsed);
    auto data = list.Serialize();

    MCMListManager loaded(reg);
    auto result = loaded.Deserialize(data);
    REQUIRE(result.has_value());
    CHECK(loaded.GetSortMode() == MCMSortMode::MostRecentlyUsed);
}
