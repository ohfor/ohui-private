#include <catch2/catch_test_macros.hpp>

#include "ohui/widget/WidgetRegistry.h"

using namespace ohui;
using namespace ohui::widget;

static WidgetManifest MakeManifest(const std::string& id, Vec2 defPos = {10.0f, 20.0f},
                                   Vec2 defSize = {100.0f, 50.0f},
                                   Vec2 minSize = {0.0f, 0.0f},
                                   Vec2 maxSize = {0.0f, 0.0f},
                                   bool visible = true) {
    return WidgetManifest{id, id + "_display", defPos, defSize, minSize, maxSize, visible};
}

TEST_CASE("Register stores manifest and initializes state", "[widget]") {
    WidgetRegistry reg;
    auto manifest = MakeManifest("health", {5.0f, 10.0f}, {200.0f, 30.0f});
    manifest.defaultVisible = true;

    auto result = reg.Register(manifest);
    REQUIRE(result.has_value());

    auto* state = reg.GetWidgetState("health");
    REQUIRE(state != nullptr);
    CHECK(state->id == "health");
    CHECK(state->position == Vec2{5.0f, 10.0f});
    CHECK(state->size == Vec2{200.0f, 30.0f});
    CHECK(state->visible == true);
    CHECK(state->active == false);
}

TEST_CASE("Duplicate registration returns DuplicateRegistration", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));

    auto result = reg.Register(MakeManifest("health"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("Activate and Deactivate set active flag", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));

    reg.Activate("health");
    CHECK(reg.GetWidgetState("health")->active == true);

    reg.Deactivate("health");
    CHECK(reg.GetWidgetState("health")->active == false);
}

TEST_CASE("Activate unknown widget returns WidgetNotFound", "[widget]") {
    WidgetRegistry reg;
    auto result = reg.Activate("nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::WidgetNotFound);
}

TEST_CASE("Move updates position", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));

    reg.Move("health", {50.0f, 75.0f});
    CHECK(reg.GetWidgetState("health")->position == Vec2{50.0f, 75.0f});
}

TEST_CASE("Move unknown widget returns WidgetNotFound", "[widget]") {
    WidgetRegistry reg;
    auto result = reg.Move("nonexistent", {0.0f, 0.0f});
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::WidgetNotFound);
}

TEST_CASE("Resize updates size", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health", {}, {100.0f, 50.0f}));

    reg.Resize("health", {150.0f, 80.0f});
    CHECK(reg.GetWidgetState("health")->size == Vec2{150.0f, 80.0f});
}

TEST_CASE("Resize clamps to minimum size", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health", {}, {100.0f, 50.0f}, {40.0f, 20.0f}));

    reg.Resize("health", {10.0f, 5.0f});
    CHECK(reg.GetWidgetState("health")->size == Vec2{40.0f, 20.0f});
}

TEST_CASE("Resize clamps to maximum size", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health", {}, {100.0f, 50.0f}, {0.0f, 0.0f}, {200.0f, 100.0f}));

    reg.Resize("health", {300.0f, 200.0f});
    CHECK(reg.GetWidgetState("health")->size == Vec2{200.0f, 100.0f});
}

TEST_CASE("Resize with unconstrained max allows any size", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health", {}, {100.0f, 50.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}));

    reg.Resize("health", {5000.0f, 3000.0f});
    CHECK(reg.GetWidgetState("health")->size == Vec2{5000.0f, 3000.0f});
}

TEST_CASE("SetVisible updates visibility", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));

    reg.SetVisible("health", false);
    CHECK(reg.GetWidgetState("health")->visible == false);

    reg.SetVisible("health", true);
    CHECK(reg.GetWidgetState("health")->visible == true);
}

TEST_CASE("GetWidgetState returns null for unknown ID", "[widget]") {
    WidgetRegistry reg;
    CHECK(reg.GetWidgetState("nonexistent") == nullptr);
}

TEST_CASE("GetAllWidgets returns all registered widgets", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));
    reg.Register(MakeManifest("magicka"));
    reg.Register(MakeManifest("stamina"));

    auto all = reg.GetAllWidgets();
    CHECK(all.size() == 3);
}

TEST_CASE("GetActiveWidgets returns only active widgets", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));
    reg.Register(MakeManifest("magicka"));
    reg.Register(MakeManifest("stamina"));

    reg.Activate("health");
    reg.Activate("stamina");

    auto active = reg.GetActiveWidgets();
    CHECK(active.size() == 2);
}

TEST_CASE("HasWidget true for registered, false for unknown", "[widget]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("health"));

    CHECK(reg.HasWidget("health") == true);
    CHECK(reg.HasWidget("nonexistent") == false);
}

TEST_CASE("GetManifest returns stored manifest", "[widget]") {
    WidgetRegistry reg;
    auto manifest = MakeManifest("health", {5.0f, 10.0f}, {200.0f, 30.0f}, {20.0f, 10.0f}, {400.0f, 100.0f});
    reg.Register(manifest);

    auto* stored = reg.GetManifest("health");
    REQUIRE(stored != nullptr);
    CHECK(stored->id == "health");
    CHECK(stored->displayName == "health_display");
    CHECK(stored->defaultPosition == Vec2{5.0f, 10.0f});
    CHECK(stored->defaultSize == Vec2{200.0f, 30.0f});
    CHECK(stored->minSize == Vec2{20.0f, 10.0f});
    CHECK(stored->maxSize == Vec2{400.0f, 100.0f});
}

TEST_CASE("WidgetCount reflects registrations", "[widget]") {
    WidgetRegistry reg;
    CHECK(reg.WidgetCount() == 0);

    reg.Register(MakeManifest("health"));
    CHECK(reg.WidgetCount() == 1);

    reg.Register(MakeManifest("magicka"));
    CHECK(reg.WidgetCount() == 2);
}
