#include <catch2/catch_test_macros.hpp>

#include "ohui/widget/ViewportManager.h"

using namespace ohui;
using namespace ohui::widget;

// --- Mock viewport widget ---

class MockViewportWidget : public IViewportWidget {
public:
    MockViewportWidget(WidgetManifest manifest)
        : m_manifest(std::move(manifest)) {}

    void Render(const ViewportRect& viewport) override {
        renderCalls.push_back(viewport);
    }

    void OnViewportEvent(const ViewportEvent& event) override {
        events.push_back(event);
    }

    const WidgetManifest& GetManifest() const override {
        return m_manifest;
    }

    std::vector<ViewportRect> renderCalls;
    std::vector<ViewportEvent> events;

private:
    WidgetManifest m_manifest;
};

static WidgetManifest MakeManifest(const std::string& id, Vec2 pos = {100, 200},
                                    Vec2 size = {300, 50}) {
    WidgetManifest m;
    m.id = id;
    m.displayName = id;
    m.defaultPosition = pos;
    m.defaultSize = size;
    m.defaultVisible = true;
    return m;
}

TEST_CASE("Register viewport widget succeeds, appears in WidgetRegistry", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-test"));

    auto result = mgr.Register(&widget);
    REQUIRE(result.has_value());
    CHECK(mgr.HasViewport("vp-test"));
    CHECK(registry.HasWidget("vp-test"));
}

TEST_CASE("Duplicate registration returns DuplicateRegistration", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget w1(MakeManifest("vp-dup"));
    MockViewportWidget w2(MakeManifest("vp-dup"));

    REQUIRE(mgr.Register(&w1).has_value());
    auto result = mgr.Register(&w2);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("Unregister removes from ViewportManager", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-unreg"));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(mgr.HasViewport("vp-unreg"));

    auto result = mgr.Unregister("vp-unreg");
    REQUIRE(result.has_value());
    CHECK_FALSE(mgr.HasViewport("vp-unreg"));
}

TEST_CASE("Unregister unknown returns WidgetNotFound", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);

    auto result = mgr.Unregister("nonexistent");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == ErrorCode::WidgetNotFound);
}

TEST_CASE("UpdateAll renders active+visible widgets with correct viewport rect", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-render", {10, 20}, {100, 50}));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.Activate("vp-render").has_value());

    mgr.UpdateAll();

    REQUIRE(widget.renderCalls.size() == 1);
    CHECK(widget.renderCalls[0].left == 10.0f);
    CHECK(widget.renderCalls[0].top == 20.0f);
    CHECK(widget.renderCalls[0].width == 100.0f);
    CHECK(widget.renderCalls[0].height == 50.0f);
}

TEST_CASE("UpdateAll skips inactive widgets", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-inactive"));

    REQUIRE(mgr.Register(&widget).has_value());
    // Widget is not activated — active=false by default

    mgr.UpdateAll();

    CHECK(widget.renderCalls.empty());
}

TEST_CASE("UpdateAll skips hidden widgets", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-hidden"));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.Activate("vp-hidden").has_value());
    REQUIRE(registry.SetVisible("vp-hidden", false).has_value());

    mgr.UpdateAll();

    CHECK(widget.renderCalls.empty());
}

TEST_CASE("SyncWidgetState after Move fires Moved event with updated viewport", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-move", {10, 20}, {100, 50}));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.Move("vp-move", {50, 60}).has_value());

    mgr.SyncWidgetState("vp-move");

    REQUIRE(widget.events.size() == 1);
    CHECK(widget.events[0].type == ViewportNotification::Moved);
    CHECK(widget.events[0].viewport.left == 50.0f);
    CHECK(widget.events[0].viewport.top == 60.0f);
}

TEST_CASE("SyncWidgetState after Resize fires Resized event", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-resize", {10, 20}, {100, 50}));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.Resize("vp-resize", {200, 100}).has_value());

    mgr.SyncWidgetState("vp-resize");

    REQUIRE(widget.events.size() == 1);
    CHECK(widget.events[0].type == ViewportNotification::Resized);
    CHECK(widget.events[0].viewport.width == 200.0f);
    CHECK(widget.events[0].viewport.height == 100.0f);
}

TEST_CASE("SyncWidgetState after SetVisible fires VisibilityChanged event", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-vis"));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.SetVisible("vp-vis", false).has_value());

    mgr.SyncWidgetState("vp-vis");

    REQUIRE(widget.events.size() == 1);
    CHECK(widget.events[0].type == ViewportNotification::VisibilityChanged);
    CHECK_FALSE(widget.events[0].visible);
}

TEST_CASE("NotifyEditModeChanged(true) fires EditModeEntered", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-edit"));

    REQUIRE(mgr.Register(&widget).has_value());

    mgr.NotifyEditModeChanged(true);

    REQUIRE(widget.events.size() == 1);
    CHECK(widget.events[0].type == ViewportNotification::EditModeEntered);
}

TEST_CASE("NotifyEditModeChanged(false) fires EditModeExited", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-exit-edit"));

    REQUIRE(mgr.Register(&widget).has_value());

    mgr.NotifyEditModeChanged(false);

    REQUIRE(widget.events.size() == 1);
    CHECK(widget.events[0].type == ViewportNotification::EditModeExited);
}

TEST_CASE("Viewport dimensions in Render match WidgetState position/size", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    MockViewportWidget widget(MakeManifest("vp-dims", {15, 25}, {200, 100}));

    REQUIRE(mgr.Register(&widget).has_value());
    REQUIRE(registry.Activate("vp-dims").has_value());
    REQUIRE(registry.Move("vp-dims", {30, 40}).has_value());
    REQUIRE(registry.Resize("vp-dims", {250, 120}).has_value());

    mgr.UpdateAll();

    REQUIRE(widget.renderCalls.size() == 1);
    CHECK(widget.renderCalls[0].left == 30.0f);
    CHECK(widget.renderCalls[0].top == 40.0f);
    CHECK(widget.renderCalls[0].width == 250.0f);
    CHECK(widget.renderCalls[0].height == 120.0f);
}

TEST_CASE("GetManifest from IViewportWidget is what gets registered", "[viewport]") {
    WidgetRegistry registry;
    ViewportManager mgr(registry);
    auto manifest = MakeManifest("vp-manifest", {5, 10}, {400, 200});
    MockViewportWidget widget(manifest);

    REQUIRE(mgr.Register(&widget).has_value());

    const auto* regManifest = registry.GetManifest("vp-manifest");
    REQUIRE(regManifest != nullptr);
    CHECK(regManifest->id == "vp-manifest");
    CHECK(regManifest->defaultPosition.x == 5.0f);
    CHECK(regManifest->defaultSize.x == 400.0f);
}
