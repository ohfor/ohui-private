#include <catch2/catch_test_macros.hpp>

#include "ohui/dsl/IconAtlas.h"

using namespace ohui::dsl;

static const IconEntry kTestEntries[] = {
    {"weapon-sword", "atlas/weapons.dds", {0.0f, 0.0f, 0.25f, 0.25f}},
    {"weapon-bow",   "atlas/weapons.dds", {0.25f, 0.0f, 0.5f, 0.25f}},
    {"status-stolen", "atlas/status.dds", {0.0f, 0.0f, 0.5f, 0.5f}},
};

TEST_CASE("LoadAtlas stores entries retrievable by ID", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());
    CHECK(atlas.IconCount() == 3);
}

TEST_CASE("Resolve returns correct IconEntry", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());

    const auto* entry = atlas.Resolve("weapon-sword");
    REQUIRE(entry != nullptr);
    CHECK(entry->atlasPath == "atlas/weapons.dds");
    CHECK(entry->uv.u1 == 0.25f);
}

TEST_CASE("Resolve returns nullptr for unknown icon", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());
    CHECK(atlas.Resolve("nonexistent") == nullptr);
}

TEST_CASE("HasIcon true for loaded, false for unknown", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());
    CHECK(atlas.HasIcon("weapon-sword"));
    CHECK(atlas.HasIcon("status-stolen"));
    CHECK_FALSE(atlas.HasIcon("nope"));
}

TEST_CASE("IconCount reflects loaded entries", "[icon-atlas]") {
    IconAtlas atlas;
    CHECK(atlas.IconCount() == 0);
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());
    CHECK(atlas.IconCount() == 3);
}

TEST_CASE("LoadSkinAtlas overrides base atlas entries", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());

    IconEntry skinEntry{"weapon-sword", "atlas/custom_weapons.dds", {0, 0, 1, 1}};
    REQUIRE(atlas.LoadSkinAtlas("custom", std::span<const IconEntry>(&skinEntry, 1)).has_value());

    const auto* entry = atlas.Resolve("weapon-sword");
    REQUIRE(entry != nullptr);
    CHECK(entry->atlasPath == "atlas/custom_weapons.dds");
}

TEST_CASE("ClearSkinAtlas restores base entries", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());

    IconEntry skinEntry{"weapon-sword", "atlas/custom.dds", {0, 0, 1, 1}};
    REQUIRE(atlas.LoadSkinAtlas("custom", std::span<const IconEntry>(&skinEntry, 1)).has_value());

    atlas.ClearSkinAtlas("custom");

    const auto* entry = atlas.Resolve("weapon-sword");
    REQUIRE(entry != nullptr);
    CHECK(entry->atlasPath == "atlas/weapons.dds");
}

TEST_CASE("Multiple atlases coexist", "[icon-atlas]") {
    IconAtlas atlas;

    IconEntry armorEntries[] = {
        {"armor-iron", "atlas/armor.dds", {0, 0, 0.5f, 0.5f}},
    };
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());
    REQUIRE(atlas.LoadAtlas("armor", armorEntries).has_value());

    CHECK(atlas.HasIcon("weapon-sword"));
    CHECK(atlas.HasIcon("armor-iron"));
    CHECK(atlas.IconCount() == 4);
}

TEST_CASE("Skin atlas only overrides matching IDs", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());

    IconEntry skinEntry{"weapon-bow", "atlas/skin_bow.dds", {0, 0, 1, 1}};
    REQUIRE(atlas.LoadSkinAtlas("skin1", std::span<const IconEntry>(&skinEntry, 1)).has_value());

    // weapon-sword still resolves to base
    const auto* sword = atlas.Resolve("weapon-sword");
    REQUIRE(sword != nullptr);
    CHECK(sword->atlasPath == "atlas/weapons.dds");

    // weapon-bow resolves to skin
    const auto* bow = atlas.Resolve("weapon-bow");
    REQUIRE(bow != nullptr);
    CHECK(bow->atlasPath == "atlas/skin_bow.dds");
}

TEST_CASE("ClearAllSkinAtlases restores all", "[icon-atlas]") {
    IconAtlas atlas;
    REQUIRE(atlas.LoadAtlas("weapons", kTestEntries).has_value());

    IconEntry skin1{"weapon-sword", "atlas/s1.dds", {0, 0, 1, 1}};
    IconEntry skin2{"weapon-bow", "atlas/s2.dds", {0, 0, 1, 1}};
    REQUIRE(atlas.LoadSkinAtlas("s1", std::span<const IconEntry>(&skin1, 1)).has_value());
    REQUIRE(atlas.LoadSkinAtlas("s2", std::span<const IconEntry>(&skin2, 1)).has_value());

    atlas.ClearAllSkinAtlases();

    CHECK(atlas.Resolve("weapon-sword")->atlasPath == "atlas/weapons.dds");
    CHECK(atlas.Resolve("weapon-bow")->atlasPath == "atlas/weapons.dds");
}
