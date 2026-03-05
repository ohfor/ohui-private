#include <catch2/catch_test_macros.hpp>

#include "ohui/input/ActionRegistry.h"
#include "ohui/input/RebindingStore.h"

#include <string>

using namespace ohui;
using namespace ohui::input;

// -- ActionRegistry tests --

TEST_CASE("RegisterAction stores definition", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "jump";
    def.displayName = "Jump";
    def.defaultKey = KeyCode::Space;
    auto result = reg.RegisterAction(def);
    REQUIRE(result.has_value());
    CHECK(reg.ActionCount() == 1);
}

TEST_CASE("RegisterAction duplicate returns DuplicateRegistration", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "jump";
    reg.RegisterAction(def);
    auto result = reg.RegisterAction(def);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("HasAction true for registered", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "attack";
    reg.RegisterAction(def);
    CHECK(reg.HasAction("gameplay", "attack"));
    CHECK(!reg.HasAction("gameplay", "unknown"));
}

TEST_CASE("GetAction returns definition", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "sprint";
    def.displayName = "Sprint";
    def.defaultKey = KeyCode::LeftShift;
    reg.RegisterAction(def);

    auto* action = reg.GetAction("gameplay", "sprint");
    REQUIRE(action != nullptr);
    CHECK(action->displayName == "Sprint");
    CHECK(action->defaultKey == KeyCode::LeftShift);
}

TEST_CASE("GetActionsForContext returns actions for specific context", "[action]") {
    ActionRegistry reg;
    ActionDefinition d1{.contextId = "gameplay", .actionId = "jump"};
    ActionDefinition d2{.contextId = "gameplay", .actionId = "sprint"};
    ActionDefinition d3{.contextId = "menu", .actionId = "select"};
    reg.RegisterAction(d1);
    reg.RegisterAction(d2);
    reg.RegisterAction(d3);

    auto actions = reg.GetActionsForContext("gameplay");
    CHECK(actions.size() == 2);
}

TEST_CASE("GetActionsForContext returns empty for unknown context", "[action]") {
    ActionRegistry reg;
    auto actions = reg.GetActionsForContext("nonexistent");
    CHECK(actions.empty());
}

TEST_CASE("PopulateContext adds actions to InputContext", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "jump";
    def.displayName = "Jump";
    def.defaultKey = KeyCode::Space;
    reg.RegisterAction(def);

    InputContext ctx("gameplay");
    reg.PopulateContext(ctx, "gameplay");
    CHECK(ctx.ActionCount() == 1);
    CHECK(ctx.HasAction("jump"));
}

TEST_CASE("PopulateContext applies default bindings", "[action]") {
    ActionRegistry reg;
    ActionDefinition def;
    def.contextId = "gameplay";
    def.actionId = "use";
    def.defaultKey = KeyCode::E;
    def.defaultButton = GamepadButton::A;
    reg.RegisterAction(def);

    InputContext ctx("gameplay");
    reg.PopulateContext(ctx, "gameplay");
    CHECK(ctx.GetKeyBinding("use") == KeyCode::E);
    CHECK(ctx.GetButtonBinding("use") == GamepadButton::A);
}

// -- RebindingStore tests --

TEST_CASE("SetKeyBinding stores rebinding", "[action]") {
    RebindingStore store;
    auto result = store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    REQUIRE(result.has_value());
    CHECK(store.HasRebinding("gameplay", "jump"));
    auto* entry = store.GetRebinding("gameplay", "jump");
    REQUIRE(entry != nullptr);
    CHECK(entry->key == KeyCode::F);
}

TEST_CASE("SetButtonBinding stores rebinding", "[action]") {
    RebindingStore store;
    auto result = store.SetButtonBinding("gameplay", "accept", GamepadButton::B);
    REQUIRE(result.has_value());
    CHECK(store.HasRebinding("gameplay", "accept"));
    auto* entry = store.GetRebinding("gameplay", "accept");
    REQUIRE(entry != nullptr);
    CHECK(entry->button == GamepadButton::B);
}

TEST_CASE("HasKeyConflict detects within same context", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    CHECK(store.HasKeyConflict("gameplay", "sprint", KeyCode::F));
}

TEST_CASE("HasKeyConflict no conflict across different contexts", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    CHECK(!store.HasKeyConflict("menu", "select", KeyCode::F));
}

TEST_CASE("HasButtonConflict detects within same context", "[action]") {
    RebindingStore store;
    store.SetButtonBinding("gameplay", "jump", GamepadButton::A);
    CHECK(store.HasButtonConflict("gameplay", "sprint", GamepadButton::A));
}

TEST_CASE("GetConflictingAction returns action ID of conflict", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    CHECK(store.GetConflictingAction("gameplay", KeyCode::F) == "jump");
}

TEST_CASE("GetConflictingAction returns empty when no conflict", "[action]") {
    RebindingStore store;
    CHECK(store.GetConflictingAction("gameplay", KeyCode::F).empty());
}

TEST_CASE("ApplyToContext overrides default bindings", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("test", "use", KeyCode::F);

    InputContext ctx("test");
    ctx.RegisterAction(ActionBinding{"use", "Use", KeyCode::E, GamepadButton::None, nullptr});
    store.ApplyToContext(ctx);

    CHECK(ctx.GetKeyBinding("use") == KeyCode::F);
}

TEST_CASE("ResetContext removes rebindings for specific context", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    store.SetKeyBinding("menu", "select", KeyCode::Enter);
    store.ResetContext("gameplay");

    CHECK(!store.HasRebinding("gameplay", "jump"));
    CHECK(store.HasRebinding("menu", "select"));
}

TEST_CASE("ResetAll clears all rebindings", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    store.SetKeyBinding("menu", "select", KeyCode::Enter);
    store.ResetAll();

    CHECK(store.RebindingCount() == 0);
}

TEST_CASE("Serialize then Deserialize round-trips rebindings", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::Space);
    store.SetButtonBinding("gameplay", "jump", GamepadButton::A);
    store.SetKeyBinding("menu", "back", KeyCode::Escape);

    auto data = store.Serialize();
    REQUIRE(!data.empty());

    RebindingStore loaded;
    auto result = loaded.Deserialize(data);
    REQUIRE(result.has_value());
    CHECK(loaded.RebindingCount() == store.RebindingCount());

    auto* entry = loaded.GetRebinding("gameplay", "jump");
    REQUIRE(entry != nullptr);
    CHECK(entry->key == KeyCode::Space);
    CHECK(entry->button == GamepadButton::A);

    auto* menuEntry = loaded.GetRebinding("menu", "back");
    REQUIRE(menuEntry != nullptr);
    CHECK(menuEntry->key == KeyCode::Escape);
}

TEST_CASE("Deserialize empty data produces empty state", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    auto result = store.Deserialize({});
    REQUIRE(result.has_value());
    CHECK(store.RebindingCount() == 0);
}

TEST_CASE("HasRebinding true after set, false after reset", "[action]") {
    RebindingStore store;
    store.SetKeyBinding("gameplay", "jump", KeyCode::F);
    CHECK(store.HasRebinding("gameplay", "jump"));
    store.ResetAll();
    CHECK(!store.HasRebinding("gameplay", "jump"));
}
