#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/dsl/ComponentHandler.h"
#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/dsl/WidgetParser.h"

using namespace ohui;
using namespace ohui::dsl;
using namespace ohui::binding;
using namespace ohui::widget;

static ComponentRegistry MakeRegistry() {
    ComponentRegistry reg;
    reg.RegisterBuiltins();
    return reg;
}

static ParseResult ParseInput(const std::string& input) {
    WidgetParser parser;
    auto result = parser.Parse(input);
    REQUIRE(result.has_value());
    return std::move(*result);
}

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
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Rect) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Portrait
// ---------------------------------------------------------------------------

TEST_CASE("Portrait emits DrawRect with opacity 0 (viewport placeholder)", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Portrait {
                viewportId: "player-portrait";
                width: 100;
                height: 100;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->opacity, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("Portrait has fixed aspect ratio (square: width == height)", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Portrait {
                viewportId: "npc-portrait";
                width: 120;
                height: 80;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    // Portrait enforces square: uses min(width, height)
    CHECK_THAT(rect->width, Catch::Matchers::WithinAbs(rect->height, 0.01f));
    CHECK_THAT(rect->width, Catch::Matchers::WithinAbs(80.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// CharacterViewport
// ---------------------------------------------------------------------------

TEST_CASE("CharacterViewport emits DrawRect with opacity 0", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CharacterViewport {
                viewportId: "char-view-1";
                width: 200;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->opacity, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("CharacterViewport layout dimensions correct", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CharacterViewport {
                viewportId: "char-view-2";
                width: 200;
                height: 250;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));
    CHECK_THAT(rect->height, Catch::Matchers::WithinAbs(250.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// SceneViewport
// ---------------------------------------------------------------------------

TEST_CASE("SceneViewport emits DrawRect with opacity 0", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SceneViewport {
                viewportId: "scene-main";
                width: 400;
                height: 300;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->opacity, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

// ---------------------------------------------------------------------------
// MapViewport
// ---------------------------------------------------------------------------

TEST_CASE("MapViewport emits DrawRect with opacity 0", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            MapViewport {
                viewportId: "world-map";
                width: 300;
                height: 200;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->opacity, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

// ---------------------------------------------------------------------------
// Cross-viewport tests
// ---------------------------------------------------------------------------

TEST_CASE("All viewport components emit a DrawRect each", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                flex-direction: column;
                width: 400;
                height: 300;
                Portrait { viewportId: "p1"; width: 50; height: 50; }
                CharacterViewport { viewportId: "cv1"; width: 100; height: 150; }
                SceneViewport { viewportId: "sv1"; width: 200; height: 150; }
                MapViewport { viewportId: "mv1"; width: 150; height: 100; }
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    // 1 Panel rect + 4 viewport rects = 5 total
    CHECK(CountDrawRects(*result) >= 5);
}

TEST_CASE("Viewport components participate in flex layout (rect dimensions match)", "[media-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CharacterViewport {
                viewportId: "flex-char";
                width: 180;
                height: 220;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->width, Catch::Matchers::WithinAbs(180.0f, 1.0f));
    CHECK_THAT(rect->height, Catch::Matchers::WithinAbs(220.0f, 1.0f));
}
