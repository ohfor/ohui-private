#include <catch2/catch_test_macros.hpp>

#include "ohui/dsl/DefaultTokens.h"
#include "ohui/dsl/TokenStore.h"

using namespace ohui::dsl;

TEST_CASE("Default tokens load without error", "[tokens]") {
    TokenStore store;
    auto result = store.LoadBaseTokens(kDefaultTokensUSS);
    REQUIRE(result.has_value());
}

TEST_CASE("All 27 colour tokens present", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());

    // Text colours (8)
    CHECK(store.HasToken("--color-text-primary"));
    CHECK(store.HasToken("--color-text-secondary"));
    CHECK(store.HasToken("--color-text-disabled"));
    CHECK(store.HasToken("--color-text-positive"));
    CHECK(store.HasToken("--color-text-negative"));
    CHECK(store.HasToken("--color-text-warning"));
    CHECK(store.HasToken("--color-text-critical"));
    CHECK(store.HasToken("--color-text-lore"));

    // Surface (4)
    CHECK(store.HasToken("--color-surface-base"));
    CHECK(store.HasToken("--color-surface-raised"));
    CHECK(store.HasToken("--color-surface-overlay"));
    CHECK(store.HasToken("--color-surface-input"));

    // Border (3)
    CHECK(store.HasToken("--color-border-default"));
    CHECK(store.HasToken("--color-border-focus"));
    CHECK(store.HasToken("--color-border-subtle"));

    // Accent (2)
    CHECK(store.HasToken("--color-accent-primary"));
    CHECK(store.HasToken("--color-accent-secondary"));

    // Resource (4)
    CHECK(store.HasToken("--color-resource-health"));
    CHECK(store.HasToken("--color-resource-stamina"));
    CHECK(store.HasToken("--color-resource-magicka"));
    CHECK(store.HasToken("--color-resource-xp"));

    // Status (5) -> minus museum = still 5
    CHECK(store.HasToken("--color-status-stolen"));
    CHECK(store.HasToken("--color-status-quest"));
    CHECK(store.HasToken("--color-status-enchanted"));
    CHECK(store.HasToken("--color-status-museum-needed"));
    CHECK(store.HasToken("--color-status-museum-donated"));
}

TEST_CASE("All 6 spacing tokens present", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());

    CHECK(store.HasToken("--spacing-xs"));
    CHECK(store.HasToken("--spacing-sm"));
    CHECK(store.HasToken("--spacing-md"));
    CHECK(store.HasToken("--spacing-lg"));
    CHECK(store.HasToken("--spacing-xl"));
    CHECK(store.HasToken("--spacing-xxl"));

    // Verify values
    CHECK(store.Resolve("--spacing-xs").value() == "2");
    CHECK(store.Resolve("--spacing-md").value() == "8");
    CHECK(store.Resolve("--spacing-xxl").value() == "32");
}

TEST_CASE("All 9 typography tokens present", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());

    // Size (6)
    CHECK(store.HasToken("--font-size-xs"));
    CHECK(store.HasToken("--font-size-sm"));
    CHECK(store.HasToken("--font-size-md"));
    CHECK(store.HasToken("--font-size-lg"));
    CHECK(store.HasToken("--font-size-xl"));
    CHECK(store.HasToken("--font-size-display"));

    // Weight (3)
    CHECK(store.HasToken("--font-weight-regular"));
    CHECK(store.HasToken("--font-weight-medium"));
    CHECK(store.HasToken("--font-weight-bold"));

    // Family (2)
    CHECK(store.HasToken("--font-family-ui"));
    CHECK(store.HasToken("--font-family-lore"));

    CHECK(store.Resolve("--font-size-md").value() == "14");
    CHECK(store.Resolve("--font-family-lore").value() == "Garamond");
}

TEST_CASE("All 4 shape tokens present", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());

    CHECK(store.HasToken("--radius-sm"));
    CHECK(store.HasToken("--radius-md"));
    CHECK(store.HasToken("--radius-lg"));
    CHECK(store.HasToken("--radius-pill"));

    CHECK(store.Resolve("--radius-sm").value() == "2");
    CHECK(store.Resolve("--radius-pill").value() == "9999");
}

TEST_CASE("Skin tokens override default colour", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());

    auto original = store.Resolve("--color-text-primary");
    REQUIRE(original.has_value());
    CHECK(*original == "#E8E8E8");

    REQUIRE(store.LoadSkinTokens(":root { --color-text-primary: #FFFFFF; }").has_value());
    auto skinned = store.Resolve("--color-text-primary");
    REQUIRE(skinned.has_value());
    CHECK(*skinned == "#FFFFFF");
}

TEST_CASE("ClearSkinTokens restores defaults", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());
    REQUIRE(store.LoadSkinTokens(":root { --color-text-primary: #FFFFFF; }").has_value());

    store.ClearSkinTokens();

    auto val = store.Resolve("--color-text-primary");
    REQUIRE(val.has_value());
    CHECK(*val == "#E8E8E8");
}

TEST_CASE("Token count is 47", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(kDefaultTokensUSS).has_value());
    // 8 text + 4 surface + 3 border + 2 accent + 4 resource + 5 status = 26 color
    // 6 spacing + 6 font-size + 3 font-weight + 2 font-family + 4 radius = 21
    // Total: 47
    CHECK(store.TokenCount() == 47);
}
