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

static const DrawText* FindDrawText(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) {
            if (count == index) return &std::get<DrawText>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static const DrawLine* FindDrawLine(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Line) {
            if (count == index) return &std::get<DrawLine>(call.data);
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

static bool HasClipRect(const DrawCallList& list) {
    for (const auto& call : list.calls) {
        if (call.clip.has_value()) return true;
    }
    return false;
}

TEST_CASE("Panel emits DrawRect with fill and border", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: #FF0000;
                borderWidth: 2;
                borderColor: #00FF00;
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
    CHECK(rect->fillColor.r == 255);
    CHECK_THAT(rect->borderWidth, Catch::Matchers::WithinAbs(2.0f, 0.01f));
}

TEST_CASE("Panel with token-based surface color resolves", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--color-surface-base);
            }
        }
    )");
    TokenStore tokens;
    REQUIRE(tokens.LoadBaseTokens(":root { --color-surface-base: #1A1A1AE6; }").has_value());
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK(rect->fillColor.r == 0x1A);
    CHECK(rect->fillColor.a == 0xE6);
}

TEST_CASE("Panel with border radius uses shape token", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                borderRadius: token(--radius-md);
            }
        }
    )");
    TokenStore tokens;
    REQUIRE(tokens.LoadBaseTokens(":root { --radius-md: 4; }").has_value());
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->borderRadius, Catch::Matchers::WithinAbs(4.0f, 0.01f));
}

TEST_CASE("SplitPanel with 50/50 ratio produces two equal-width children", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SplitPanel {
                ratio: 0.5;
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
    auto* line = FindDrawLine(*result);
    REQUIRE(line != nullptr);
    // Default horizontal split at midpoint
    CHECK_THAT(line->x1, Catch::Matchers::WithinAbs(200.0f, 1.0f));
}

TEST_CASE("SplitPanel with 30/70 ratio adjusts child widths", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SplitPanel {
                ratio: 0.3;
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
    auto* line = FindDrawLine(*result);
    REQUIRE(line != nullptr);
    CHECK_THAT(line->x1, Catch::Matchers::WithinAbs(120.0f, 1.0f));
}

TEST_CASE("SplitPanel vertical orientation splits height", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SplitPanel {
                orientation: vertical;
                ratio: 0.5;
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
    auto* line = FindDrawLine(*result);
    REQUIRE(line != nullptr);
    // Horizontal divider at vertical midpoint
    CHECK_THAT(line->y1, Catch::Matchers::WithinAbs(150.0f, 1.0f));
}

TEST_CASE("SplitPanel divider emits DrawLine between panels", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            SplitPanel {
                ratio: 0.5;
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
    auto* line = FindDrawLine(*result);
    REQUIRE(line != nullptr);
    CHECK(line->color.r == 0x40);  // border-default
}

TEST_CASE("ScrollPanel sets clip rect on child draw calls", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollPanel {
                width: 200;
                height: 150;
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
    CHECK(HasClipRect(*result));
}

TEST_CASE("ScrollPanel scroll offset shifts child Y positions", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollPanel {
                width: 200;
                height: 150;
                fillColor: #333333;
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
    REQUIRE(HasClipRect(*result));
    // Verify clip rect dimensions match scroll panel
    for (const auto& call : result->calls) {
        if (call.clip.has_value()) {
            CHECK_THAT(call.clip->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));
            CHECK_THAT(call.clip->height, Catch::Matchers::WithinAbs(150.0f, 1.0f));
            break;
        }
    }
}

TEST_CASE("ScrollPanel with zero scroll shows top content", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ScrollPanel {
                width: 200;
                height: 150;
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
    // Background rect should be at origin
    auto* bg = FindDrawRect(*result);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->x, Catch::Matchers::WithinAbs(0.0f, 0.01f));
    CHECK_THAT(bg->y, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("Modal emits overlay DrawRect before content", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Modal {
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
    CHECK(CountDrawRects(*result) >= 2);
}

TEST_CASE("Modal overlay uses surface-overlay color", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Modal {
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
    // First rect (overlay) should be black with 0xCC alpha
    auto* overlay = FindDrawRect(*result, 0);
    REQUIRE(overlay != nullptr);
    CHECK(overlay->fillColor.r == 0);
    CHECK(overlay->fillColor.a == 0xCC);
}

TEST_CASE("Modal overlay has higher z-index than background", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Modal {
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
    // All Modal draw calls should have z-index >= 100
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Rect) {
            CHECK(call.zIndex >= 100);
        }
    }
}

TEST_CASE("Modal content centered within viewport", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Modal {
                width: 200;
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
    // Second rect (content) dimensions match specified
    auto* content = FindDrawRect(*result, 1);
    REQUIRE(content != nullptr);
    CHECK_THAT(content->width, Catch::Matchers::WithinAbs(200.0f, 1.0f));
    CHECK_THAT(content->height, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("Tooltip placed below anchor by default", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Tooltip {
                anchorX: 100;
                anchorY: 100;
                text: "Help text";
                width: 120;
                height: 30;
                screenWidth: 800;
                screenHeight: 600;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->x, Catch::Matchers::WithinAbs(100.0f, 1.0f));
    CHECK(bg->y > 100.0f);  // Below anchor
}

TEST_CASE("Tooltip repositions above when too close to bottom", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Tooltip {
                anchorX: 100;
                anchorY: 590;
                text: "Help text";
                width: 120;
                height: 30;
                screenWidth: 800;
                screenHeight: 600;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK(bg->y < 590.0f);  // Repositioned above
}

TEST_CASE("Tooltip repositions left when too close to right edge", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Tooltip {
                anchorX: 750;
                anchorY: 100;
                text: "Help text";
                width: 120;
                height: 30;
                screenWidth: 800;
                screenHeight: 600;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK(bg->x + bg->width <= 800.0f);  // Fits on screen
}

TEST_CASE("Drawer from left edge starts at x=0", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Drawer {
                edge: left;
                open: true;
                width: 200;
                height: 400;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->x, Catch::Matchers::WithinAbs(0.0f, 0.01f));
}

TEST_CASE("Drawer from right edge starts at viewport width - drawer width", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Drawer {
                edge: right;
                open: true;
                width: 200;
                height: 400;
                screenWidth: 800;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->x, Catch::Matchers::WithinAbs(600.0f, 1.0f));
}

TEST_CASE("Drawer closed state emits no draw calls", "[container-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Drawer {
                edge: left;
                open: false;
                width: 200;
                height: 400;
            }
        }
    )");
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    auto result = engine.Evaluate("TestWidget", {0, 0, 800, 600}, 0.0f);
    REQUIRE(result.has_value());
    CHECK(result->calls.empty());
}
