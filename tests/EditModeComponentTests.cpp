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

static size_t CountDrawTexts(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Text) ++count;
    }
    return count;
}

static size_t CountDrawLines(const DrawCallList& list) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Line) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// WidgetBoundingBox
// ---------------------------------------------------------------------------

TEST_CASE("WidgetBoundingBox emits border DrawRect around widget", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            WidgetBoundingBox {
                selected: true;
                width: 100;
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
    CHECK(CountDrawRects(*result) >= 1);
    auto* border = FindDrawRect(*result, 0);
    REQUIRE(border != nullptr);
    // Border rect has transparent fill and non-zero borderWidth
    CHECK(border->fillColor.a == 0);
    CHECK(border->borderWidth > 0.0f);
}

TEST_CASE("WidgetBoundingBox emits 4 corner handle DrawRects", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            WidgetBoundingBox {
                selected: true;
                width: 100;
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
    // 1 border + 4 handles = 5 DrawRects
    CHECK(CountDrawRects(*result) == 5);
    // Verify handles are 6x6
    for (size_t i = 1; i <= 4; ++i) {
        auto* handle = FindDrawRect(*result, i);
        REQUIRE(handle != nullptr);
        CHECK_THAT(handle->width, Catch::Matchers::WithinAbs(6.0f, 0.01f));
        CHECK_THAT(handle->height, Catch::Matchers::WithinAbs(6.0f, 0.01f));
    }
}

TEST_CASE("WidgetBoundingBox emits label DrawText above widget", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            WidgetBoundingBox {
                selected: true;
                label: "TestLabel";
                width: 100;
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
    CHECK(CountDrawTexts(*result) >= 1);
    auto* label = FindDrawText(*result, 0);
    REQUIRE(label != nullptr);
    CHECK(label->text == "TestLabel");
    // Label should be above the widget (y position less than widget absY)
    auto* border = FindDrawRect(*result, 0);
    REQUIRE(border != nullptr);
    CHECK(label->y < border->y);
}

TEST_CASE("WidgetBoundingBox selected state uses accent color", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            WidgetBoundingBox {
                selected: true;
                width: 100;
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
    auto* border = FindDrawRect(*result, 0);
    REQUIRE(border != nullptr);
    CHECK(border->borderColor.r == 0x80);
    CHECK(border->borderColor.g == 0x90);
    CHECK(border->borderColor.b == 0xA0);
}

TEST_CASE("WidgetBoundingBox unselected state uses border-default color", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            WidgetBoundingBox {
                width: 100;
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
    auto* border = FindDrawRect(*result, 0);
    REQUIRE(border != nullptr);
    CHECK(border->borderColor.r == 0x40);
    CHECK(border->borderColor.g == 0x40);
    CHECK(border->borderColor.b == 0x40);
}

// ---------------------------------------------------------------------------
// AlignmentGuide
// ---------------------------------------------------------------------------

TEST_CASE("AlignmentGuide emits DrawLine when edges align", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlignmentGuide {
                axis: horizontal;
                position: 50;
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
    CHECK(CountDrawLines(*result) == 1);
    auto* line = FindDrawLine(*result);
    REQUIRE(line != nullptr);
    CHECK_THAT(line->y1, Catch::Matchers::WithinAbs(50.0f, 0.01f));
    CHECK_THAT(line->y2, Catch::Matchers::WithinAbs(50.0f, 0.01f));
}

TEST_CASE("AlignmentGuide no lines when no alignment", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlignmentGuide {
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
    CHECK(CountDrawLines(*result) == 0);
}

TEST_CASE("AlignmentGuide horizontal alignment", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlignmentGuide {
                axis: horizontal;
                position: 100;
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
    CHECK_THAT(line->y1, Catch::Matchers::WithinAbs(100.0f, 0.01f));
    CHECK_THAT(line->y2, Catch::Matchers::WithinAbs(100.0f, 0.01f));
}

TEST_CASE("AlignmentGuide vertical alignment", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            AlignmentGuide {
                axis: vertical;
                position: 150;
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
    CHECK_THAT(line->x1, Catch::Matchers::WithinAbs(150.0f, 0.01f));
    CHECK_THAT(line->x2, Catch::Matchers::WithinAbs(150.0f, 0.01f));
}

// ---------------------------------------------------------------------------
// EditModeToolbar
// ---------------------------------------------------------------------------

TEST_CASE("EditModeToolbar emits Panel with coordinate text", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            EditModeToolbar {
                coordText: "X:100 Y:200";
                width: 400;
                height: 32;
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
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 1);
    auto* bg = FindDrawRect(*result, 0);
    REQUIRE(bg != nullptr);
    CHECK(bg->fillColor.r == 0x2A);
    CHECK(bg->fillColor.g == 0x2A);
    CHECK(bg->fillColor.b == 0x2A);
    // Find coord text
    auto* coordText = FindDrawText(*result, 0);
    REQUIRE(coordText != nullptr);
    CHECK(coordText->text == "X:100 Y:200");
}

TEST_CASE("EditModeToolbar shows snap toggle state", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            EditModeToolbar {
                coordText: "X:0 Y:0";
                snapEnabled: true;
                width: 400;
                height: 32;
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
    // Look for snap text containing "ON"
    bool foundSnapOn = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text.find("ON") != std::string::npos) {
                foundSnapOn = true;
                break;
            }
        }
    }
    CHECK(foundSnapOn);
}

// ---------------------------------------------------------------------------
// GridOverlay
// ---------------------------------------------------------------------------

TEST_CASE("GridOverlay emits grid DrawLines at snap spacing", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            GridOverlay {
                spacing: 32;
                width: 128;
                height: 128;
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
    CHECK(CountDrawLines(*result) > 0);
}

TEST_CASE("GridOverlay with 16px spacing on 256px viewport", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            GridOverlay {
                spacing: 16;
                width: 256;
                height: 256;
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
    // 256/16 = 16 intervals, lines at 16,32,...,240 = 15 vertical + 15 horizontal = 30
    CHECK(CountDrawLines(*result) == 30);
}

TEST_CASE("GridOverlay hidden when visible is false", "[edit-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            GridOverlay {
                visible: false;
                spacing: 16;
                width: 256;
                height: 256;
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
    CHECK(result->calls.empty());
}
