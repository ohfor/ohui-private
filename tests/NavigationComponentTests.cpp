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

// --- TabBar tests ---

TEST_CASE("TabBar with 3 tabs emits 3 DrawRect and 3 DrawText", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TabBar {
                tabs: "Tab1,Tab2,Tab3";
                activeIndex: 0;
                width: 400;
                height: 40;
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
    CHECK(CountDrawRects(*result) >= 3);
    CHECK(CountDrawTexts(*result) >= 3);
    auto* t0 = FindDrawText(*result, 0);
    auto* t1 = FindDrawText(*result, 1);
    auto* t2 = FindDrawText(*result, 2);
    REQUIRE(t0 != nullptr);
    REQUIRE(t1 != nullptr);
    REQUIRE(t2 != nullptr);
    CHECK(t0->text == "Tab1");
    CHECK(t1->text == "Tab2");
    CHECK(t2->text == "Tab3");
}

TEST_CASE("TabBar active tab has accent color", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TabBar {
                tabs: "Tab1,Tab2,Tab3";
                activeIndex: 1;
                width: 400;
                height: 40;
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
    auto* activeText = FindDrawText(*result, 1);
    REQUIRE(activeText != nullptr);
    CHECK(activeText->color.r == 0x80);
}

TEST_CASE("TabBar inactive tabs have secondary color", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TabBar {
                tabs: "Tab1,Tab2,Tab3";
                activeIndex: 1;
                width: 400;
                height: 40;
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
    auto* inactiveText = FindDrawText(*result, 0);
    REQUIRE(inactiveText != nullptr);
    CHECK(inactiveText->color.r == 0xA0);
}

TEST_CASE("TabBar active indicator underline on selected tab", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            TabBar {
                tabs: "Tab1,Tab2,Tab3";
                activeIndex: 0;
                width: 400;
                height: 40;
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
}

// --- Breadcrumb tests ---

TEST_CASE("Breadcrumb with 3 levels emits text + separator + text + separator + text", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Breadcrumb {
                levels: "Home,Items,Weapons";
                width: 400;
                height: 30;
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
    // 3 level texts + 2 separators = 5 DrawText calls minimum
    CHECK(CountDrawTexts(*result) >= 5);
}

TEST_CASE("Breadcrumb separator uses > by default", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Breadcrumb {
                levels: "Home,Items,Weapons";
                width: 400;
                height: 30;
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
    // Check that at least one DrawText contains the ">" separator
    bool foundSeparator = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text.find(">") != std::string::npos) {
                foundSeparator = true;
                break;
            }
        }
    }
    CHECK(foundSeparator);
}

TEST_CASE("Breadcrumb last item uses primary color, others use secondary", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Breadcrumb {
                levels: "Home,Items,Weapons";
                width: 400;
                height: 30;
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
    // Pattern: text(0)=Home, text(1)=>, text(2)=Items, text(3)=>, text(4)=Weapons
    auto* firstLevel = FindDrawText(*result, 0);
    auto* lastLevel = FindDrawText(*result, 4);
    REQUIRE(firstLevel != nullptr);
    REQUIRE(lastLevel != nullptr);
    CHECK(firstLevel->color.r == 0xA0);   // secondary color
    CHECK(lastLevel->color.r == 0xE8);    // primary color
}

// --- Pagination tests ---

TEST_CASE("Pagination with 5 pages emits page dots or numbers", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Pagination {
                totalPages: 5;
                currentPage: 3;
                width: 300;
                height: 30;
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
    // prev arrow + 5 page numbers + next arrow = 7 DrawText calls minimum
    CHECK(CountDrawTexts(*result) >= 7);
}

TEST_CASE("Pagination current page highlighted with accent color", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Pagination {
                totalPages: 5;
                currentPage: 3;
                width: 300;
                height: 30;
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
    // Index 0 = "<", indices 1-5 = page numbers, index 6 = ">"
    // currentPage 3 corresponds to index 3
    auto* currentPageText = FindDrawText(*result, 3);
    REQUIRE(currentPageText != nullptr);
    CHECK(currentPageText->color.r == 0x80);  // accent color
}

TEST_CASE("Pagination prev and next arrows present", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Pagination {
                totalPages: 5;
                currentPage: 3;
                width: 300;
                height: 30;
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
    bool foundPrev = false;
    bool foundNext = false;
    for (const auto& call : result->calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text == "<") foundPrev = true;
            if (dt.text == ">") foundNext = true;
        }
    }
    CHECK(foundPrev);
    CHECK(foundNext);
}

TEST_CASE("Pagination at first page prev arrow has disabled color", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Pagination {
                totalPages: 5;
                currentPage: 1;
                width: 300;
                height: 30;
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
    // First DrawText should be the "<" prev arrow
    auto* prevArrow = FindDrawText(*result, 0);
    REQUIRE(prevArrow != nullptr);
    CHECK(prevArrow->text == "<");
    CHECK(prevArrow->color.r == 0x60);  // disabled color
}

TEST_CASE("Pagination at last page next arrow has disabled color", "[nav-component]") {
    auto parsed = ParseInput(R"(
        widget TestWidget {
            Pagination {
                totalPages: 5;
                currentPage: 5;
                width: 300;
                height: 30;
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
    // Last DrawText should be the ">" next arrow
    size_t textCount = CountDrawTexts(*result);
    REQUIRE(textCount > 0);
    auto* nextArrow = FindDrawText(*result, textCount - 1);
    REQUIRE(nextArrow != nullptr);
    CHECK(nextArrow->text == ">");
    CHECK(nextArrow->color.r == 0x60);  // disabled color
}
