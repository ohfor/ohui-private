#include <catch2/catch_test_macros.hpp>

#include "ohui/dsl/TokenStore.h"

using namespace ohui::dsl;

TEST_CASE("Load base tokens from USS", "[tokens]") {
    TokenStore store;
    REQUIRE(store.LoadBaseTokens(":root { --color-primary: #ff0000; }").has_value());
    CHECK(store.HasToken("--color-primary"));
    CHECK(store.TokenCount() == 1);
}

TEST_CASE("Resolve returns value for known token, nullopt for unknown", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --color-primary: #ff0000; }");
    auto val = store.Resolve("--color-primary");
    REQUIRE(val.has_value());
    CHECK(*val == "#ff0000");
    CHECK(!store.Resolve("--unknown").has_value());
}

TEST_CASE("Skin tokens override base, leave unrelated keys", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --color-primary: #ff0000; --color-text: #ffffff; }");
    (void)store.LoadSkinTokens(":root { --color-primary: #0000ff; }");

    CHECK(store.Resolve("--color-primary").value() == "#0000ff");
    CHECK(store.Resolve("--color-text").value() == "#ffffff");
}

TEST_CASE("ClearSkinTokens reverts to base", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --color-primary: #ff0000; }");
    (void)store.LoadSkinTokens(":root { --color-primary: #0000ff; }");
    CHECK(store.Resolve("--color-primary").value() == "#0000ff");

    store.ClearSkinTokens();
    CHECK(store.Resolve("--color-primary").value() == "#ff0000");
}

TEST_CASE("Skin can add new tokens not in base", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --color-primary: #ff0000; }");
    (void)store.LoadSkinTokens(":root { --color-accent: #ff00ff; }");

    CHECK(store.HasToken("--color-accent"));
    CHECK(store.Resolve("--color-accent").value() == "#ff00ff");
    CHECK(store.TokenCount() == 2);
}

TEST_CASE("ResolveVar with token found", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --primary: red; }");
    CHECK(store.ResolveVar("var(--primary)") == "red");
}

TEST_CASE("ResolveVar with fallback, token exists (token wins)", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --primary: red; }");
    CHECK(store.ResolveVar("var(--primary, blue)") == "red");
}

TEST_CASE("ResolveVar with fallback, token missing (fallback used)", "[tokens]") {
    TokenStore store;
    CHECK(store.ResolveVar("var(--missing, blue)") == "blue");
}

TEST_CASE("ResolveVar undefined without fallback returns empty", "[tokens]") {
    TokenStore store;
    CHECK(store.ResolveVar("var(--missing)") == "");
}

TEST_CASE("ResolveAllVars replaces multiple var() calls", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --fg: white; --bg: black; }");
    auto result = store.ResolveAllVars("color: var(--fg); background: var(--bg);");
    CHECK(result == "color: white; background: black;");
}

TEST_CASE("HasToken and TokenCount", "[tokens]") {
    TokenStore store;
    CHECK(store.TokenCount() == 0);
    CHECK(!store.HasToken("--x"));

    (void)store.LoadBaseTokens(":root { --a: 1; --b: 2; }");
    CHECK(store.TokenCount() == 2);
    CHECK(store.HasToken("--a"));
    CHECK(store.HasToken("--b"));
}

TEST_CASE("Multiple LoadBaseTokens replaces previous", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --old: value; }");
    CHECK(store.HasToken("--old"));

    (void)store.LoadBaseTokens(":root { --new: value; }");
    CHECK(!store.HasToken("--old"));
    CHECK(store.HasToken("--new"));
}

TEST_CASE("Multiple LoadSkinTokens replaces (not merges) skin layer", "[tokens]") {
    TokenStore store;
    (void)store.LoadBaseTokens(":root { --base: x; }");
    (void)store.LoadSkinTokens(":root { --skin1: a; }");
    CHECK(store.HasToken("--skin1"));

    (void)store.LoadSkinTokens(":root { --skin2: b; }");
    CHECK(!store.HasToken("--skin1"));
    CHECK(store.HasToken("--skin2"));
}
