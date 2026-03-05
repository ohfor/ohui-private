#include <catch2/catch_test_macros.hpp>

#include "ohui/binding/ModBindingAPI.h"

using namespace ohui;
using namespace ohui::binding;

static PollFunction DummyPoll() {
    return []() -> BindingValue { return 42.0f; };
}

static ModBindingDefinition MakeDef(const std::string& modId, const std::string& id) {
    return ModBindingDefinition{modId, id, BindingType::Float, "test binding", DummyPoll()};
}

TEST_CASE("Registration window starts Closed", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Closed);
}

TEST_CASE("OpenRegistrationWindow sets Open", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Open);
}

TEST_CASE("LockRegistrationWindow sets Locked", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();
    api.LockRegistrationWindow();
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Locked);
}

TEST_CASE("RegisterBinding succeeds when window Open", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    auto result = api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));
    CHECK(result.has_value());
}

TEST_CASE("RegisterBinding fails when window Closed", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);

    auto result = api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::RegistrationWindowClosed);
}

TEST_CASE("RegisterBinding fails when window Locked", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();
    api.LockRegistrationWindow();

    auto result = api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::RegistrationWindowClosed);
}

TEST_CASE("RegisterBinding fails with empty modId", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    auto result = api.RegisterBinding(MakeDef("", ".health"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidModId);
}

TEST_CASE("RegisterBinding fails when ID doesn't match namespace", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    auto result = api.RegisterBinding(MakeDef("com.mymod", "wrong.namespace.health"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidNamespace);
}

TEST_CASE("RegisterBinding succeeds and binding appears in engine", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    (void)api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));
    CHECK(engine.HasBinding("com.mymod.health"));
}

TEST_CASE("RegisterBinding with null poll function fails", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    ModBindingDefinition def;
    def.modId = "com.mymod";
    def.id = "com.mymod.test";
    def.type = BindingType::Float;
    def.description = "test";
    def.pollFn = nullptr;

    auto result = api.RegisterBinding(def);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidBinding);
}

TEST_CASE("Duplicate binding ID supersedes", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    (void)api.RegisterBinding(MakeDef("com.mod_a", "com.mod_a.shared"));

    // Re-register with same mod — supersede
    auto result = api.RegisterBinding(MakeDef("com.mod_a", "com.mod_a.shared"));
    CHECK(result.has_value());
    CHECK(api.ModBindingCount() == 1);
}

TEST_CASE("UnregisterBinding removes binding", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    (void)api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));
    auto result = api.UnregisterBinding("com.mymod.health");
    CHECK(result.has_value());
    CHECK(!engine.HasBinding("com.mymod.health"));
    CHECK(!api.IsModBinding("com.mymod.health"));
}

TEST_CASE("UnregisterBinding for unknown ID returns BindingNotFound", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);

    auto result = api.UnregisterBinding("nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::BindingNotFound);
}

TEST_CASE("RegisterBatch registers all on success (returns count)", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    std::vector<ModBindingDefinition> defs = {
        MakeDef("com.mymod", "com.mymod.a"),
        MakeDef("com.mymod", "com.mymod.b"),
        MakeDef("com.mymod", "com.mymod.c"),
    };

    auto result = api.RegisterBatch(defs);
    REQUIRE(result.has_value());
    CHECK(*result == 3);
    CHECK(api.ModBindingCount() == 3);
}

TEST_CASE("RegisterBatch stops on first error (partial success)", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    std::vector<ModBindingDefinition> defs = {
        MakeDef("com.mymod", "com.mymod.a"),
        MakeDef("com.mymod", "wrong.namespace.b"),  // fails namespace check
        MakeDef("com.mymod", "com.mymod.c"),
    };

    auto result = api.RegisterBatch(defs);
    REQUIRE(result.has_value());
    CHECK(*result == 1);  // only first succeeded before error
}

TEST_CASE("IsModBinding/GetOwnerMod correct", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    (void)api.RegisterBinding(MakeDef("com.mymod", "com.mymod.health"));

    CHECK(api.IsModBinding("com.mymod.health"));
    CHECK(!api.IsModBinding("nonexistent"));
    CHECK(api.GetOwnerMod("com.mymod.health") == "com.mymod");
    CHECK(api.GetOwnerMod("nonexistent").empty());
}

TEST_CASE("GetBindingsByMod returns correct list", "[mod-binding]") {
    DataBindingEngine engine;
    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();

    (void)api.RegisterBinding(MakeDef("com.mod_a", "com.mod_a.x"));
    (void)api.RegisterBinding(MakeDef("com.mod_a", "com.mod_a.y"));
    (void)api.RegisterBinding(MakeDef("com.mod_b", "com.mod_b.z"));

    auto modABindings = api.GetBindingsByMod("com.mod_a");
    CHECK(modABindings.size() == 2);

    auto modBBindings = api.GetBindingsByMod("com.mod_b");
    CHECK(modBBindings.size() == 1);

    auto modCBindings = api.GetBindingsByMod("com.mod_c");
    CHECK(modCBindings.empty());
}

TEST_CASE("Custom binding coexists with builtin bindings", "[mod-binding]") {
    DataBindingEngine engine;

    // Register a "builtin" binding directly
    BindingDefinition builtinDef;
    builtinDef.id = "ohui.player.health";
    builtinDef.type = BindingType::Float;
    builtinDef.description = "Player health";
    (void)engine.RegisterBinding(builtinDef, DummyPoll());

    ModBindingAPI api(engine);
    api.OpenRegistrationWindow();
    (void)api.RegisterBinding(MakeDef("com.mymod", "com.mymod.custom"));

    CHECK(engine.HasBinding("ohui.player.health"));
    CHECK(engine.HasBinding("com.mymod.custom"));
    CHECK(!api.IsModBinding("ohui.player.health"));
    CHECK(api.IsModBinding("com.mymod.custom"));
}
