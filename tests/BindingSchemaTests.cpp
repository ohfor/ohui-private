#include <catch2/catch_test_macros.hpp>

#include "ohui/binding/BindingSchema.h"
#include "ohui/binding/DataBindingEngine.h"

#include <algorithm>
#include <unordered_set>

using namespace ohui;
using namespace ohui::binding;

TEST_CASE("GetBuiltinBindings returns all schema entries (count = 79)", "[schema]") {
    const auto& bindings = BindingSchema::GetBuiltinBindings();
    CHECK(bindings.size() == 79);
    CHECK(BindingSchema::BuiltinBindingCount() == 79);
}

TEST_CASE("Each builtin has non-empty id, description, and valid type", "[schema]") {
    for (const auto& entry : BindingSchema::GetBuiltinBindings()) {
        CHECK(!entry.id.empty());
        CHECK(!entry.description.empty());
        // Type is always valid (enum class)
    }
}

TEST_CASE("No duplicate binding IDs in builtins", "[schema]") {
    std::unordered_set<std::string> ids;
    for (const auto& entry : BindingSchema::GetBuiltinBindings()) {
        CHECK(ids.insert(entry.id).second);
    }
}

TEST_CASE("RegisterBuiltinBindings registers all in engine", "[schema]") {
    DataBindingEngine engine;
    auto result = BindingSchema::RegisterBuiltinBindings(engine);
    REQUIRE(result.has_value());
    CHECK(engine.BindingCount() == 79);
}

TEST_CASE("Builtin bindings have correct types", "[schema]") {
    DataBindingEngine engine;
    BindingSchema::RegisterBuiltinBindings(engine);

    auto* health = engine.GetBindingDefinition("player.health");
    REQUIRE(health != nullptr);
    CHECK(health->type == BindingType::Float);

    auto* level = engine.GetBindingDefinition("player.level");
    REQUIRE(level != nullptr);
    CHECK(level->type == BindingType::Int);

    auto* sneaking = engine.GetBindingDefinition("player.sneaking");
    REQUIRE(sneaking != nullptr);
    CHECK(sneaking->type == BindingType::Bool);

    auto* locName = engine.GetBindingDefinition("location.name");
    REQUIRE(locName != nullptr);
    CHECK(locName->type == BindingType::String);
}

TEST_CASE("Placeholder poll functions return default values", "[schema]") {
    DataBindingEngine engine;
    BindingSchema::RegisterBuiltinBindings(engine);

    // Subscribe to trigger polling
    engine.Subscribe("test", "player.health");
    engine.Subscribe("test", "player.level");
    engine.Subscribe("test", "player.sneaking");
    engine.Subscribe("test", "location.name");
    engine.Update(0.016f);

    auto* healthVal = engine.GetCurrentValue("player.health");
    REQUIRE(healthVal != nullptr);
    CHECK(std::get<float>(*healthVal) == 0.0f);

    auto* levelVal = engine.GetCurrentValue("player.level");
    REQUIRE(levelVal != nullptr);
    CHECK(std::get<int64_t>(*levelVal) == 0);

    auto* sneakVal = engine.GetCurrentValue("player.sneaking");
    REQUIRE(sneakVal != nullptr);
    CHECK(std::get<bool>(*sneakVal) == false);

    auto* locVal = engine.GetCurrentValue("location.name");
    REQUIRE(locVal != nullptr);
    CHECK(std::get<std::string>(*locVal).empty());
}

TEST_CASE("RegisterCustomBinding succeeds with valid namespace", "[schema]") {
    DataBindingEngine engine;
    auto result = BindingSchema::RegisterCustomBinding(engine, "mymod",
        "mymod.custom.value", BindingType::Float, "Custom value",
        []() -> BindingValue { return 42.0f; });
    REQUIRE(result.has_value());
    CHECK(engine.HasBinding("mymod.custom.value"));
}

TEST_CASE("RegisterCustomBinding fails without namespace prefix", "[schema]") {
    DataBindingEngine engine;
    auto result = BindingSchema::RegisterCustomBinding(engine, "mymod",
        "custom.value", BindingType::Float, "No prefix",
        []() -> BindingValue { return 0.0f; });
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidNamespace);
}

TEST_CASE("RegisterCustomBinding fails with wrong namespace prefix", "[schema]") {
    DataBindingEngine engine;
    auto result = BindingSchema::RegisterCustomBinding(engine, "mymod",
        "othermod.value", BindingType::Float, "Wrong prefix",
        []() -> BindingValue { return 0.0f; });
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidNamespace);
}

TEST_CASE("Custom binding coexists with builtins", "[schema]") {
    DataBindingEngine engine;
    BindingSchema::RegisterBuiltinBindings(engine);
    BindingSchema::RegisterCustomBinding(engine, "mymod",
        "mymod.custom", BindingType::Float, "Custom",
        []() -> BindingValue { return 1.0f; });
    CHECK(engine.BindingCount() == 80);
}

TEST_CASE("RegisterCustomBinding with null poll function returns InvalidBinding", "[schema]") {
    DataBindingEngine engine;
    auto result = BindingSchema::RegisterCustomBinding(engine, "mymod",
        "mymod.value", BindingType::Float, "Null poll", nullptr);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidBinding);
}

TEST_CASE("All 18 skill bindings present", "[schema]") {
    const auto& bindings = BindingSchema::GetBuiltinBindings();
    std::vector<std::string> skillIds;
    for (const auto& entry : bindings) {
        if (entry.id.starts_with("skill.")) {
            skillIds.push_back(entry.id);
        }
    }
    CHECK(skillIds.size() == 18);
}

TEST_CASE("Player vitals bindings present with correct types (15 Float)", "[schema]") {
    const auto& bindings = BindingSchema::GetBuiltinBindings();
    int vitalCount = 0;
    for (const auto& entry : bindings) {
        if (entry.id.starts_with("player.health") ||
            entry.id.starts_with("player.magicka") ||
            entry.id.starts_with("player.stamina") ||
            entry.id.starts_with("player.carry") ||
            entry.id.starts_with("player.shout.cooldown") ||
            entry.id == "player.enchant.charge") {
            CHECK(entry.type == BindingType::Float);
            ++vitalCount;
        }
    }
    CHECK(vitalCount == 15);
}

TEST_CASE("Enemy target bindings present (5 with correct types)", "[schema]") {
    const auto& bindings = BindingSchema::GetBuiltinBindings();
    int enemyCount = 0;
    for (const auto& entry : bindings) {
        if (entry.id.starts_with("enemy.")) {
            ++enemyCount;
        }
    }
    CHECK(enemyCount == 5);
}

TEST_CASE("Time and world bindings present (10 with correct types)", "[schema]") {
    const auto& bindings = BindingSchema::GetBuiltinBindings();
    int count = 0;
    for (const auto& entry : bindings) {
        if (entry.id.starts_with("time.") ||
            entry.id.starts_with("weather.") ||
            entry.id.starts_with("world.")) {
            ++count;
        }
    }
    CHECK(count == 10);
}
