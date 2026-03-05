#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCM2ConditionEngine.h"

using namespace ohui;
using namespace ohui::mcm;

// --- Mock state provider ---

class MockStateProvider : public IConditionStateProvider {
public:
    std::string prefix;
    std::unordered_map<std::string, ConditionValue> values;

    explicit MockStateProvider(std::string p) : prefix(std::move(p)) {}

    std::string ProviderPrefix() const override { return prefix; }

    std::optional<ConditionValue> Resolve(std::string_view key) const override {
        auto it = values.find(std::string(key));
        if (it != values.end()) return it->second;
        return std::nullopt;
    }
};

// --- Helper to build a minimal definition with conditions ---

static MCM2Definition MakeDef(
    std::vector<std::pair<std::string, std::string>> controlConditions) {
    MCM2Definition def;
    def.id = "test";
    MCM2PageDef page;
    page.id = "p";
    MCM2SectionDef section;
    section.id = "s";
    for (auto& [id, cond] : controlConditions) {
        MCM2ControlDef ctrl;
        ctrl.type = MCM2ControlType::Toggle;
        ctrl.id = id;
        ctrl.conditionExpr = cond;
        ctrl.properties = MCM2ToggleProps{};
        section.controls.push_back(std::move(ctrl));
    }
    page.sections.push_back(std::move(section));
    def.pages.push_back(std::move(page));
    return def;
}

// 1. enableFeature == true evaluates true
TEST_CASE("Condition: control == true when true", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("enableFeature", ConditionValue{true});
    auto result = engine.EvaluateExpression("enableFeature == true");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 2. enableFeature == true evaluates false when false
TEST_CASE("Condition: control == true when false", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("enableFeature", ConditionValue{false});
    auto result = engine.EvaluateExpression("enableFeature == true");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == false);
}

// 3. featureMode != "Disabled"
TEST_CASE("Condition: string inequality", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("featureMode", ConditionValue{std::string("Normal")});
    auto result = engine.EvaluateExpression("featureMode != \"Disabled\"");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 4. featureStrength >= 50 numeric comparison
TEST_CASE("Condition: numeric >=", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("featureStrength", ConditionValue{int64_t(75)});
    auto result = engine.EvaluateExpression("featureStrength >= 50");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 5. featureStrength < 10
TEST_CASE("Condition: numeric <", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("featureStrength", ConditionValue{int64_t(5)});
    auto result = engine.EvaluateExpression("featureStrength < 10");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 6. a == true && b > 0
TEST_CASE("Condition: logical AND", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("a", ConditionValue{true});
    engine.SetControlValue("b", ConditionValue{int64_t(5)});
    auto result = engine.EvaluateExpression("a == true && b > 0");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 7. a == true || b == true
TEST_CASE("Condition: logical OR", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("a", ConditionValue{false});
    engine.SetControlValue("b", ConditionValue{true});
    auto result = engine.EvaluateExpression("a == true || b == true");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 8. (a || b) && c
TEST_CASE("Condition: parenthesized grouping", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("a", ConditionValue{false});
    engine.SetControlValue("b", ConditionValue{true});
    engine.SetControlValue("c", ConditionValue{true});
    auto result = engine.EvaluateExpression("(a || b) && c");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 9. !disabled
TEST_CASE("Condition: NOT operator", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("disabled", ConditionValue{false});
    auto result = engine.EvaluateExpression("!disabled");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 10. player.skill.smithing >= 50 provider call
TEST_CASE("Condition: provider call dotted", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto provider = std::make_shared<MockStateProvider>("player");
    provider->values["skill.smithing"] = ConditionValue{int64_t(75)};
    engine.RegisterStateProvider(provider);

    auto result = engine.EvaluateExpression("player.skill.smithing >= 50");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 11. mod.isLoaded("Ordinator") with string argument
TEST_CASE("Condition: provider with string argument", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto provider = std::make_shared<MockStateProvider>("mod");
    provider->values["isLoaded(Ordinator)"] = ConditionValue{true};
    engine.RegisterStateProvider(provider);

    auto result = engine.EvaluateExpression("mod.isLoaded(\"Ordinator\")");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 12. ohui.version >= "1.2.0"
TEST_CASE("Condition: provider string comparison", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto provider = std::make_shared<MockStateProvider>("ohui");
    provider->values["version"] = ConditionValue{std::string("1.3.0")};
    engine.RegisterStateProvider(provider);

    auto result = engine.EvaluateExpression("ohui.version >= \"1.2.0\"");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 13. Missing control returns false
TEST_CASE("Condition: missing control defaults to false", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto result = engine.EvaluateExpression("missingControl == true");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == false);
}

// 14. Missing provider returns false
TEST_CASE("Condition: missing provider defaults to false", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto result = engine.EvaluateExpression("unknown.value == true");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == false);
}

// 15. SetControlValue + re-evaluate visibility
TEST_CASE("Condition: toggle visibility via SetControlValue", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({{"target", "gate == true"}});
    REQUIRE(engine.CompileConditions(def).has_value());

    engine.SetControlValue("gate", ConditionValue{false});
    CHECK(engine.EvaluateVisibility("target") == ControlVisibility::Hidden);

    engine.SetControlValue("gate", ConditionValue{true});
    CHECK(engine.EvaluateVisibility("target") == ControlVisibility::Visible);
}

// 16. GetAffectedControls returns correct set
TEST_CASE("Condition: GetAffectedControls direct", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"b", "a == true"},
        {"c", "a == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    auto affected = engine.GetAffectedControls("a");
    CHECK(affected.size() == 2);
}

// 17. GetAffectedControls returns transitive dependents
TEST_CASE("Condition: GetAffectedControls transitive", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"b", "a == true"},
        {"c", "b == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    auto affected = engine.GetAffectedControls("a");
    // a -> b -> c (transitive)
    CHECK(affected.size() == 2);
    bool foundB = false, foundC = false;
    for (const auto& id : affected) {
        if (id == "b") foundB = true;
        if (id == "c") foundC = true;
    }
    CHECK(foundB);
    CHECK(foundC);
}

// 18. Circular reference: A depends on B, B depends on A
TEST_CASE("Condition: two-node cycle detected", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"a", "b == true"},
        {"b", "a == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    CHECK(engine.HasCycle());
}

// 19. Three-node cycle
TEST_CASE("Condition: three-node cycle detected", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"a", "b == true"},
        {"b", "c == true"},
        {"c", "a == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    CHECK(engine.HasCycle());
}

// 20. Cyclic control marked as Hidden
TEST_CASE("Condition: cyclic control is Hidden", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"a", "b == true"},
        {"b", "a == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    CHECK(engine.EvaluateVisibility("a") == ControlVisibility::Hidden);
    CHECK(engine.EvaluateVisibility("b") == ControlVisibility::Hidden);
}

// 21. GetCyclicControls returns correct IDs
TEST_CASE("Condition: GetCyclicControls lists cyclic nodes", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"a", "b == true"},
        {"b", "a == true"},
        {"c", "x == true"}  // not cyclic
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    auto cyclic = engine.GetCyclicControls();
    CHECK(cyclic.size() == 2);
    bool foundA = false, foundB = false;
    for (const auto& id : cyclic) {
        if (id == "a") foundA = true;
        if (id == "b") foundB = true;
    }
    CHECK(foundA);
    CHECK(foundB);
}

// 22. CompileConditions builds graph from MCM2Definition
TEST_CASE("Condition: CompileConditions from definition", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"slider", "toggle == true"},
        {"toggle", ""}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    CHECK(engine.HasCondition("slider"));
    CHECK(!engine.HasCondition("toggle"));
}

// 23. ReEvaluateAll returns changed control IDs
TEST_CASE("Condition: ReEvaluateAll returns changes", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({{"target", "gate == true"}});
    REQUIRE(engine.CompileConditions(def).has_value());

    engine.SetControlValue("gate", ConditionValue{false});
    auto changes1 = engine.ReEvaluateAll();
    // First evaluation: cache was empty (Visible) -> now Hidden = changed
    CHECK(changes1.size() == 1);

    // Re-evaluate with no change
    auto changes2 = engine.ReEvaluateAll();
    CHECK(changes2.empty());

    // Now change gate
    engine.SetControlValue("gate", ConditionValue{true});
    auto changes3 = engine.ReEvaluateAll();
    CHECK(changes3.size() == 1);
}

// 24. Empty condition string: control always visible
TEST_CASE("Condition: no condition means always visible", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    CHECK(engine.EvaluateVisibility("noConditionControl") == ControlVisibility::Visible);
}

// 25. Condition parse error returns ParseError
TEST_CASE("Condition: parse error", "[mcm2-conditions]") {
    auto result = MCM2ConditionEngine::ParseCondition("== &&");
    CHECK(!result.has_value());
    CHECK(result.error().code == ErrorCode::ParseError);
}

// 26. Bare identifier as shorthand for `enabled == true`
TEST_CASE("Condition: bare identifier as bool shorthand", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({{"target", "enabled"}});
    REQUIRE(engine.CompileConditions(def).has_value());

    engine.SetControlValue("enabled", ConditionValue{true});
    CHECK(engine.EvaluateVisibility("target") == ControlVisibility::Visible);

    engine.SetControlValue("enabled", ConditionValue{false});
    CHECK(engine.EvaluateVisibility("target") == ControlVisibility::Hidden);
}

// 27. String comparison
TEST_CASE("Condition: string equality", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("mode", ConditionValue{std::string("Normal")});
    auto result = engine.EvaluateExpression("mode == \"Normal\"");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 28. Nested NOT
TEST_CASE("Condition: double negation", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    engine.SetControlValue("enabled", ConditionValue{true});
    auto result = engine.EvaluateExpression("!!enabled");
    REQUIRE(result.has_value());
    CHECK(std::get<bool>(*result) == true);
}

// 29. Multiple conditions on separate controls don't interfere
TEST_CASE("Condition: multiple independent conditions", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({
        {"a", "x == true"},
        {"b", "y == true"}
    });
    REQUIRE(engine.CompileConditions(def).has_value());

    engine.SetControlValue("x", ConditionValue{true});
    engine.SetControlValue("y", ConditionValue{false});

    CHECK(engine.EvaluateVisibility("a") == ControlVisibility::Visible);
    CHECK(engine.EvaluateVisibility("b") == ControlVisibility::Hidden);
}

// 30. Provider registration and unregistration
TEST_CASE("Condition: provider register and unregister", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;
    auto provider = std::make_shared<MockStateProvider>("test");
    provider->values["val"] = ConditionValue{true};

    engine.RegisterStateProvider(provider);
    auto result1 = engine.EvaluateExpression("test.val == true");
    REQUIRE(result1.has_value());
    CHECK(std::get<bool>(*result1) == true);

    engine.UnregisterStateProvider("test");
    auto result2 = engine.EvaluateExpression("test.val == true");
    REQUIRE(result2.has_value());
    // Missing provider -> false
    CHECK(std::get<bool>(*result2) == false);
}

// 31. Cache invalidation on SetControlValue
TEST_CASE("Condition: cache invalidation", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto def = MakeDef({{"target", "gate == true"}});
    REQUIRE(engine.CompileConditions(def).has_value());

    engine.SetControlValue("gate", ConditionValue{true});
    engine.ReEvaluateAll(); // populate cache

    engine.SetControlValue("gate", ConditionValue{false});
    // After SetControlValue, cache should be invalidated
    CHECK(engine.EvaluateVisibility("target") == ControlVisibility::Hidden);
}

// 32. Condition with only provider references (no control refs)
TEST_CASE("Condition: provider-only condition", "[mcm2-conditions]") {
    MCM2ConditionEngine engine;

    auto provider = std::make_shared<MockStateProvider>("game");
    provider->values["difficulty"] = ConditionValue{int64_t(3)};
    engine.RegisterStateProvider(provider);

    auto def = MakeDef({{"hardModeOption", "game.difficulty >= 3"}});
    REQUIRE(engine.CompileConditions(def).has_value());

    CHECK(engine.EvaluateVisibility("hardModeOption") == ControlVisibility::Visible);
    CHECK(!engine.HasCycle());
}
