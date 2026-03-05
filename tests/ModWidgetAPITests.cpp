#include <catch2/catch_test_macros.hpp>

#include "ohui/widget/ModWidgetAPI.h"

using namespace ohui;
using namespace ohui::widget;

static WidgetManifest MakeWidgetManifest(const std::string& id) {
    return WidgetManifest{id, id + "_display", {0, 0}, {100, 50}, {0, 0}, {0, 0}, true};
}

static ModWidgetManifest MakeModManifest(const std::string& modId, const std::string& widgetId) {
    ModWidgetManifest m;
    m.modId = modId;
    m.widget = MakeWidgetManifest(widgetId);
    m.renderFn = [](const WidgetState&, dsl::DrawCallList&) {};
    return m;
}

TEST_CASE("Registration window starts Closed", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Closed);
}

TEST_CASE("OpenRegistrationWindow sets Open", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Open);
}

TEST_CASE("LockRegistrationWindow sets Locked", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();
    api.LockRegistrationWindow();
    CHECK(api.GetRegistrationWindowState() == RegistrationWindow::Locked);
}

TEST_CASE("RegisterWidget succeeds when window Open", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    auto result = api.RegisterWidget(MakeModManifest("com.mymod", "custom_widget"));
    CHECK(result.has_value());
}

TEST_CASE("RegisterWidget fails when window Closed", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);

    auto result = api.RegisterWidget(MakeModManifest("com.mymod", "custom_widget"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::RegistrationWindowClosed);
}

TEST_CASE("RegisterWidget fails when window Locked", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();
    api.LockRegistrationWindow();

    auto result = api.RegisterWidget(MakeModManifest("com.mymod", "custom_widget"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::RegistrationWindowClosed);
}

TEST_CASE("RegisterWidget fails with empty modId", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    auto result = api.RegisterWidget(MakeModManifest("", "custom_widget"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidModId);
}

TEST_CASE("RegisterWidget succeeds with valid manifest", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    auto result = api.RegisterWidget(MakeModManifest("com.mymod", "my_widget"));
    REQUIRE(result.has_value());
    CHECK(api.ModWidgetCount() == 1);
    CHECK(reg.HasWidget("my_widget"));
}

TEST_CASE("Duplicate widget ID supersedes (replaces, not rejects)", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    auto first = MakeModManifest("com.mod_a", "shared_id");
    auto second = MakeModManifest("com.mod_b", "shared_id");

    (void)api.RegisterWidget(first);
    auto result = api.RegisterWidget(second);

    REQUIRE(result.has_value());
    CHECK(api.ModWidgetCount() == 1);
    CHECK(api.GetOwnerMod("shared_id") == "com.mod_b");
}

TEST_CASE("Superseded widget retains new render callback", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    bool firstCalled = false;
    bool secondCalled = false;

    ModWidgetManifest first = MakeModManifest("com.mod_a", "w");
    first.renderFn = [&](const WidgetState&, dsl::DrawCallList&) { firstCalled = true; };
    (void)api.RegisterWidget(first);

    ModWidgetManifest second = MakeModManifest("com.mod_b", "w");
    second.renderFn = [&](const WidgetState&, dsl::DrawCallList&) { secondCalled = true; };
    (void)api.RegisterWidget(second);

    auto* cb = api.GetRenderCallback("w");
    REQUIRE(cb != nullptr);
    WidgetState dummyState;
    dsl::DrawCallList dummyOutput;
    (*cb)(dummyState, dummyOutput);

    CHECK(!firstCalled);
    CHECK(secondCalled);
}

TEST_CASE("Widget registered via ModWidgetAPI appears in WidgetRegistry", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    (void)api.RegisterWidget(MakeModManifest("com.mymod", "mod_w"));
    CHECK(reg.HasWidget("mod_w"));
    CHECK(reg.WidgetCount() == 1);
}

TEST_CASE("UnregisterWidget removes from both registries", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    (void)api.RegisterWidget(MakeModManifest("com.mymod", "to_remove"));
    REQUIRE(reg.HasWidget("to_remove"));
    REQUIRE(api.IsModWidget("to_remove"));

    auto result = api.UnregisterWidget("to_remove");
    CHECK(result.has_value());
    CHECK(!reg.HasWidget("to_remove"));
    CHECK(!api.IsModWidget("to_remove"));
}

TEST_CASE("UnregisterWidget for unknown ID returns WidgetNotFound", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);

    auto result = api.UnregisterWidget("nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::WidgetNotFound);
}

TEST_CASE("IsModWidget true for mod widgets, false for internal", "[mod-widget]") {
    WidgetRegistry reg;
    // Register an internal widget directly
    (void)reg.Register(MakeWidgetManifest("internal_w"));

    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();
    (void)api.RegisterWidget(MakeModManifest("com.mymod", "mod_w"));

    CHECK(api.IsModWidget("mod_w"));
    CHECK(!api.IsModWidget("internal_w"));
}

TEST_CASE("GetOwnerMod returns correct mod ID", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();
    (void)api.RegisterWidget(MakeModManifest("com.mymod", "w1"));

    CHECK(api.GetOwnerMod("w1") == "com.mymod");
    CHECK(api.GetOwnerMod("unknown").empty());
}

TEST_CASE("GetWidgetsByMod returns correct list", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    (void)api.RegisterWidget(MakeModManifest("com.mod_a", "w1"));
    (void)api.RegisterWidget(MakeModManifest("com.mod_a", "w2"));
    (void)api.RegisterWidget(MakeModManifest("com.mod_b", "w3"));

    auto modAWidgets = api.GetWidgetsByMod("com.mod_a");
    CHECK(modAWidgets.size() == 2);

    auto modBWidgets = api.GetWidgetsByMod("com.mod_b");
    CHECK(modBWidgets.size() == 1);

    auto modCWidgets = api.GetWidgetsByMod("com.mod_c");
    CHECK(modCWidgets.empty());
}

TEST_CASE("ModWidgetCount reflects registrations", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    CHECK(api.ModWidgetCount() == 0);
    (void)api.RegisterWidget(MakeModManifest("com.mymod", "w1"));
    CHECK(api.ModWidgetCount() == 1);
    (void)api.RegisterWidget(MakeModManifest("com.mymod", "w2"));
    CHECK(api.ModWidgetCount() == 2);
}

TEST_CASE("GetRenderCallback returns stored callback", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    api.OpenRegistrationWindow();

    (void)api.RegisterWidget(MakeModManifest("com.mymod", "w"));
    auto* cb = api.GetRenderCallback("w");
    CHECK(cb != nullptr);
}

TEST_CASE("GetRenderCallback returns nullptr for unknown widget", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);
    CHECK(api.GetRenderCallback("unknown") == nullptr);
}

TEST_CASE("No mod widgets registered: all queries return empty/zero", "[mod-widget]") {
    WidgetRegistry reg;
    ModWidgetAPI api(reg);

    CHECK(api.ModWidgetCount() == 0);
    CHECK(!api.IsModWidget("anything"));
    CHECK(api.GetOwnerMod("anything").empty());
    CHECK(api.GetWidgetsByMod("any").empty());
    CHECK(api.GetRenderCallback("anything") == nullptr);
}
