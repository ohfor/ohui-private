#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/hud/HUDWidgetManager.h"
#include "ohui/dsl/ComponentHandler.h"
#include "ohui/dsl/WidgetParser.h"

using namespace ohui;
using namespace ohui::hud;
using namespace ohui::dsl;
using namespace ohui::binding;
using namespace ohui::widget;

// Helper: set up full test harness and return manager with bindings wired up
struct TestHarness {
    TokenStore tokens;
    DataBindingEngine bindings;
    ComponentRegistry registry;
    DSLRuntimeEngine dslEngine;
    WidgetRegistry widgetRegistry;
    HUDWidgetManager manager;

    // Binding state
    float healthPct{0.75f};
    float staminaPct{0.6f};
    float magickaPct{0.5f};
    float heading{0.0f};
    float shoutCooldownPct{0.5f};
    int64_t shoutWords{3};
    float detectionLevel{0.5f};
    std::string enemyName{"Dragon"};
    float enemyHealthPct{0.8f};
    int64_t enemyLevel{50};
    float breathPct{0.8f};

    TestHarness()
        : registry(MakeRegistry()),
          dslEngine(tokens, bindings, registry),
          manager(dslEngine, widgetRegistry, bindings)
    {
        RegisterBindings();
    }

    void RegisterBindings() {
        (void)bindings.RegisterBinding({"player.health.pct", BindingType::Float, ""}, [&]() -> BindingValue { return healthPct; });
        (void)bindings.RegisterBinding({"player.stamina.pct", BindingType::Float, ""}, [&]() -> BindingValue { return staminaPct; });
        (void)bindings.RegisterBinding({"player.magicka.pct", BindingType::Float, ""}, [&]() -> BindingValue { return magickaPct; });
        (void)bindings.RegisterBinding({"player.heading", BindingType::Float, ""}, [&]() -> BindingValue { return heading; });
        (void)bindings.RegisterBinding({"player.shout.cooldown.pct", BindingType::Float, ""}, [&]() -> BindingValue { return shoutCooldownPct; });
        (void)bindings.RegisterBinding({"equipped.shout.words", BindingType::Int, ""}, [&]() -> BindingValue { return shoutWords; });
        (void)bindings.RegisterBinding({"player.detection.level", BindingType::Float, ""}, [&]() -> BindingValue { return detectionLevel; });
        (void)bindings.RegisterBinding({"enemy.name", BindingType::String, ""}, [&]() -> BindingValue { return enemyName; });
        (void)bindings.RegisterBinding({"enemy.health.pct", BindingType::Float, ""}, [&]() -> BindingValue { return enemyHealthPct; });
        (void)bindings.RegisterBinding({"enemy.level", BindingType::Int, ""}, [&]() -> BindingValue { return enemyLevel; });
        (void)bindings.RegisterBinding({"player.breath.pct", BindingType::Float, ""}, [&]() -> BindingValue { return breathPct; });

        // Subscribe to trigger polling
        for (const auto& id : bindings.GetAllBindingIds()) {
            (void)bindings.Subscribe("hud-test", id);
        }
        bindings.Update(0.0f);
    }

    void UpdateBindings() {
        bindings.Update(0.0f);
    }

    static ComponentRegistry MakeRegistry() {
        ComponentRegistry reg;
        reg.RegisterBuiltins();
        return reg;
    }
};

static const DrawRect* FindDrawRect(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Rect) {
            if (count == index) return &std::get<DrawRect>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static size_t CountDrawRects(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls)
        if (call.type == DrawCallType::Rect) ++count;
    return count;
}

static size_t CountDrawTexts(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls)
        if (call.type == DrawCallType::Text) ++count;
    return count;
}

static size_t CountDrawLines(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls)
        if (call.type == DrawCallType::Line) ++count;
    return count;
}

// ===========================================================================
// LoadDefaults
// ===========================================================================

TEST_CASE("LoadDefaults registers all 9 manifests in WidgetRegistry", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());
    CHECK(h.widgetRegistry.WidgetCount() == 9);
}

TEST_CASE("LoadDefaults loads all 9 widget defs in DSLRuntimeEngine", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());
    CHECK(h.dslEngine.HasWidget("ohui_hud_health"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_stamina"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_magicka"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_compass"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_shout"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_detection"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_enemy"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_breath"));
    CHECK(h.dslEngine.HasWidget("ohui_hud_notifications"));
}

TEST_CASE("LoadDefaults called twice does not crash or duplicate", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());
    REQUIRE(h.manager.LoadDefaults().has_value());
    CHECK(h.manager.WidgetCount() == 9);
}

// ===========================================================================
// TASK-100: Health / Stamina / Magicka
// ===========================================================================

TEST_CASE("Health bar at 75% produces fill rect at 75% width", "[hud-widget]") {
    TestHarness h;
    h.healthPct = 0.75f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_health", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawRects(*result) >= 2);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // value=0.75, maxValue=1.0, width=200 -> fill width = 150
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(150.0f, 1.0f));
}

TEST_CASE("Stamina bar uses stamina color (green)", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_stamina", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK(fill->fillColor.r == 0x2E);
    CHECK(fill->fillColor.g == 0x7D);
    CHECK(fill->fillColor.b == 0x32);
}

TEST_CASE("Magicka bar uses magicka color (blue)", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_magicka", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK(fill->fillColor.r == 0x15);
    CHECK(fill->fillColor.g == 0x65);
    CHECK(fill->fillColor.b == 0xC0);
}

TEST_CASE("Health bar at 0% produces zero-width fill", "[hud-widget]") {
    TestHarness h;
    h.healthPct = 0.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_health", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("Health bar at 100% produces full-width fill", "[hud-widget]") {
    TestHarness h;
    h.healthPct = 1.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_health", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bg != nullptr);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(bg->width, 0.01f));
}

TEST_CASE("Three bars evaluate independently with different values", "[hud-widget]") {
    TestHarness h;
    h.healthPct = 0.3f;
    h.staminaPct = 0.6f;
    h.magickaPct = 0.9f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto rHealth = h.manager.Evaluate("ohui_hud_health", 1280, 720, 0.0f);
    auto rStamina = h.manager.Evaluate("ohui_hud_stamina", 1280, 720, 0.0f);
    auto rMagicka = h.manager.Evaluate("ohui_hud_magicka", 1280, 720, 0.0f);
    REQUIRE(rHealth.has_value());
    REQUIRE(rStamina.has_value());
    REQUIRE(rMagicka.has_value());

    auto* hFill = FindDrawRect(*rHealth, 1);
    auto* sFill = FindDrawRect(*rStamina, 1);
    auto* mFill = FindDrawRect(*rMagicka, 1);
    REQUIRE(hFill != nullptr);
    REQUIRE(sFill != nullptr);
    REQUIRE(mFill != nullptr);

    // 200 * 0.3 = 60, 200 * 0.6 = 120, 200 * 0.9 = 180
    CHECK_THAT(hFill->width, Catch::Matchers::WithinAbs(60.0f, 1.0f));
    CHECK_THAT(sFill->width, Catch::Matchers::WithinAbs(120.0f, 1.0f));
    CHECK_THAT(mFill->width, Catch::Matchers::WithinAbs(180.0f, 1.0f));
}

// ===========================================================================
// TASK-101: Compass
// ===========================================================================

TEST_CASE("Heading 0 shows N at center", "[hud-widget]") {
    TestHarness h;
    h.heading = 0.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_compass", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    bool foundN = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "N") {
                foundN = true;
                break;
            }
        }
    }
    CHECK(foundN);
}

TEST_CASE("Heading 90 shifts E to center", "[hud-widget]") {
    TestHarness h;
    h.heading = 90.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_compass", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    bool foundE = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "E") {
                foundE = true;
                break;
            }
        }
    }
    CHECK(foundE);
}

TEST_CASE("Heading 350 wraps correctly (N visible)", "[hud-widget]") {
    TestHarness h;
    h.heading = 350.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_compass", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    bool foundN = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "N") {
                foundN = true;
                break;
            }
        }
    }
    CHECK(foundN);
}

// ===========================================================================
// TASK-102: Shout Cooldown
// ===========================================================================

TEST_CASE("50% cooldown shows half fill", "[hud-widget]") {
    TestHarness h;
    h.shoutCooldownPct = 0.5f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_shout", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // 150 * 0.5 = 75
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(75.0f, 1.0f));
}

TEST_CASE("3-word shout shows 2 divider lines", "[hud-widget]") {
    TestHarness h;
    h.shoutWords = 3;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_shout", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawLines(*result) == 2);
}

TEST_CASE("1-word shout shows 0 dividers", "[hud-widget]") {
    TestHarness h;
    h.shoutWords = 1;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_shout", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawLines(*result) == 0);
}

// ===========================================================================
// TASK-103: Detection (StealthEye)
// ===========================================================================

TEST_CASE("Detection 0.5 shows half fill", "[hud-widget]") {
    TestHarness h;
    h.detectionLevel = 0.5f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_detection", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    // StealthEye: icon + fill rect when detectionLevel > 0
    auto* fillRect = FindDrawRect(*result, 0);
    REQUIRE(fillRect != nullptr);
    // 0.5 * 40 = 20
    CHECK_THAT(fillRect->width, Catch::Matchers::WithinAbs(20.0f, 1.0f));
}

TEST_CASE("Detection 0 shows no fill rect", "[hud-widget]") {
    TestHarness h;
    h.detectionLevel = 0.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_detection", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawRects(*result) == 0);
}

TEST_CASE("Detection 1.0 shows full fill", "[hud-widget]") {
    TestHarness h;
    h.detectionLevel = 1.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_detection", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fillRect = FindDrawRect(*result, 0);
    REQUIRE(fillRect != nullptr);
    // 1.0 * 40 = 40
    CHECK_THAT(fillRect->width, Catch::Matchers::WithinAbs(40.0f, 1.0f));
}

// ===========================================================================
// TASK-104: Enemy Health
// ===========================================================================

TEST_CASE("Enemy widget shows enemy name in DrawText", "[hud-widget]") {
    TestHarness h;
    h.enemyName = "Ancient Dragon";
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_enemy", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    bool foundName = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "Ancient Dragon") {
                foundName = true;
                break;
            }
        }
    }
    CHECK(foundName);
}

TEST_CASE("Enemy 80% health shows correct fill width", "[hud-widget]") {
    TestHarness h;
    h.enemyHealthPct = 0.8f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_enemy", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    // Panel (bg) + ValueBar bg + ValueBar fill = at least 3 DrawRects
    // The ValueBar fill is the last DrawRect
    bool foundFill = false;
    for (size_t i = 0; i < result->calls.size(); ++i) {
        if (result->calls[i].type == DrawCallType::Rect) {
            const auto& dr = std::get<DrawRect>(result->calls[i].data);
            // The fill rect should have width = 200 * 0.8 = 160
            if (std::abs(dr.width - 160.0f) < 2.0f) {
                foundFill = true;
                break;
            }
        }
    }
    CHECK(foundFill);
}

TEST_CASE("Enemy full health shows full-width fill", "[hud-widget]") {
    TestHarness h;
    h.enemyHealthPct = 1.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_enemy", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    // Find the ValueBar fill rect (200 * 1.0 = 200)
    bool foundFill = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Rect) {
            const auto& dr = std::get<DrawRect>(call.data);
            if (std::abs(dr.width - 200.0f) < 2.0f &&
                dr.fillColor.r == 0x8B && dr.fillColor.g == 0x00 && dr.fillColor.b == 0x00) {
                foundFill = true;
                break;
            }
        }
    }
    CHECK(foundFill);
}

// ===========================================================================
// TASK-106: Notification Toast
// ===========================================================================

TEST_CASE("Notification produces DrawRect + DrawText", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_notifications", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 1);
}

TEST_CASE("Notification default opacity is correct", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_notifications", 1280, 720, 0.0f);
    REQUIRE(result.has_value());

    // elapsed=0.0, fadeIn=0.3 -> opacity = 0.0/0.3 = 0.0
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(0.0f, 0.05f));
}

// ===========================================================================
// TASK-107: Breath Meter
// ===========================================================================

TEST_CASE("80% breath shows correct fill width", "[hud-widget]") {
    TestHarness h;
    h.breathPct = 0.8f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_breath", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    // 150 * 0.8 = 120
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(120.0f, 1.0f));
}

TEST_CASE("0% breath shows zero fill", "[hud-widget]") {
    TestHarness h;
    h.breathPct = 0.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_breath", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("100% breath shows full fill", "[hud-widget]") {
    TestHarness h;
    h.breathPct = 1.0f;
    h.UpdateBindings();
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto result = h.manager.Evaluate("ohui_hud_breath", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bg != nullptr);
    REQUIRE(fill != nullptr);
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(bg->width, 0.01f));
}

// ===========================================================================
// Integration
// ===========================================================================

TEST_CASE("GetWidgetIds returns all 9 IDs", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());

    auto ids = h.manager.GetWidgetIds();
    CHECK(ids.size() == 9);
}

TEST_CASE("WidgetCount returns 9", "[hud-widget]") {
    TestHarness h;
    REQUIRE(h.manager.LoadDefaults().has_value());
    CHECK(h.manager.WidgetCount() == 9);
}
