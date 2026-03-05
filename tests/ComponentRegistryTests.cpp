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

static const DrawIcon* FindDrawIcon(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Icon) {
            if (count == index) return &std::get<DrawIcon>(call.data);
            ++count;
        }
    }
    return nullptr;
}

static const DrawImage* FindDrawImage(const DrawCallList& list, size_t index = 0) {
    size_t count = 0;
    for (const auto& call : list.calls) {
        if (call.type == DrawCallType::Image) {
            if (count == index) return &std::get<DrawImage>(call.data);
            ++count;
        }
    }
    return nullptr;
}

TEST_CASE("RegisterBuiltins registers all builtin handlers", "[component-registry]") {
    auto registry = MakeRegistry();
    // Count grows as handlers are added. Just verify it's non-trivial.
    CHECK(registry.Count() >= 8);
}

TEST_CASE("Get returns correct handler for Panel", "[component-registry]") {
    auto registry = MakeRegistry();
    CHECK(registry.Get("Panel") != nullptr);
    CHECK(registry.Get("Label") != nullptr);
    CHECK(registry.Get("ValueBar") != nullptr);
    CHECK(registry.Get("Icon") != nullptr);
    CHECK(registry.Get("Image") != nullptr);
}

TEST_CASE("Get returns nullptr for unknown type", "[component-registry]") {
    auto registry = MakeRegistry();
    CHECK(registry.Get("NonExistent") == nullptr);
}

TEST_CASE("Has returns true for registered, false for unknown", "[component-registry]") {
    auto registry = MakeRegistry();
    CHECK(registry.Has("Panel"));
    CHECK(registry.Has("Label"));
    CHECK_FALSE(registry.Has("UnknownWidget"));
}

TEST_CASE("Panel handler emits DrawRect with correct properties", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Panel {
                fillColor: #FF0000;
                borderWidth: 2;
                borderColor: #00FF00;
                borderRadius: 4;
                opacity: 0.5;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK(rect->fillColor.r == 255);
    CHECK(rect->fillColor.g == 0);
    CHECK_THAT(rect->borderWidth, Catch::Matchers::WithinAbs(2.0f, 0.01f));
    CHECK(rect->borderColor.g == 255);
    CHECK_THAT(rect->borderRadius, Catch::Matchers::WithinAbs(4.0f, 0.01f));
    CHECK_THAT(rect->opacity, Catch::Matchers::WithinAbs(0.5f, 0.01f));
}

TEST_CASE("Label handler emits DrawText with correct properties", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Label {
                text: "Hello World";
                fontSize: 18;
                fontFamily: "Arial";
                color: #AABBCC;
                textAlign: center;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Hello World");
    CHECK_THAT(text->fontSize, Catch::Matchers::WithinAbs(18.0f, 0.01f));
    CHECK(text->color.r == 0xAA);
    CHECK(text->color.g == 0xBB);
    CHECK(text->align == TextAlign::Center);
}

TEST_CASE("ValueBar handler emits 2 DrawRects (background + fill)", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            ValueBar {
                value: 60;
                maxValue: 120;
                width: 200;
                height: 20;
                fillColor: #FF0000;
                backgroundColor: #333333;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* bg = FindDrawRect(*result, 0);
    const auto* fill = FindDrawRect(*result, 1);
    REQUIRE(bg != nullptr);
    REQUIRE(fill != nullptr);
    CHECK(bg->fillColor.r == 0x33);
    CHECK(fill->fillColor.r == 255);
    // 60/120 = 0.5, 200 * 0.5 = 100
    CHECK_THAT(fill->width, Catch::Matchers::WithinAbs(100.0f, 1.0f));
}

TEST_CASE("Icon handler emits DrawIcon with source and tint", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Icon {
                source: "icons/sword.png";
                tint: #FF0000;
                opacity: 0.8;
                width: 32;
                height: 32;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* icon = FindDrawIcon(*result);
    REQUIRE(icon != nullptr);
    CHECK(icon->source == "icons/sword.png");
    CHECK(icon->tint.r == 255);
    CHECK_THAT(icon->opacity, Catch::Matchers::WithinAbs(0.8f, 0.01f));
}

TEST_CASE("Image handler emits DrawImage with source", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Image {
                source: "textures/background.dds";
                opacity: 0.9;
                width: 256;
                height: 128;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* img = FindDrawImage(*result);
    REQUIRE(img != nullptr);
    CHECK(img->source == "textures/background.dds");
    CHECK_THAT(img->opacity, Catch::Matchers::WithinAbs(0.9f, 0.01f));
}

TEST_CASE("Unknown component type skips draw calls, still renders children", "[component-registry]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            CustomThing {
                Label {
                    text: "child of unknown";
                }
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();
    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    // No DrawRect for CustomThing (unknown), but child Label should still render
    const auto* text = FindDrawText(*result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "child of unknown");
}

TEST_CASE("Custom handler registered and invoked", "[component-registry]") {
    class TestHandler : public IComponentHandler {
    public:
        void Emit(const ComponentContext& ctx, DrawCallList& output) override {
            DrawRect dr;
            dr.x = ctx.absX;
            dr.y = ctx.absY;
            dr.width = 42.0f;
            dr.height = 42.0f;
            dr.fillColor = {1, 2, 3, 255};
            output.calls.push_back({DrawCallType::Rect, dr, 0});
        }
    };

    auto parsed = ParseInput(R"(
        widget TestWidget {
            MyCustom {
                width: 100;
                height: 100;
            }
        }
    )");

    TokenStore tokens;
    DataBindingEngine bindings;
    ComponentRegistry registry;
    registry.RegisterBuiltins();
    registry.Register("MyCustom", std::make_unique<TestHandler>());

    DSLRuntimeEngine engine(tokens, bindings, registry);
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());

    ViewportRect vp{0, 0, 400, 300};
    auto result = engine.Evaluate("TestWidget", vp, 0.0f);
    REQUIRE(result.has_value());

    const auto* rect = FindDrawRect(*result);
    REQUIRE(rect != nullptr);
    CHECK_THAT(rect->width, Catch::Matchers::WithinAbs(42.0f, 0.01f));
    CHECK(rect->fillColor.r == 1);
    CHECK(rect->fillColor.g == 2);
    CHECK(rect->fillColor.b == 3);
}
