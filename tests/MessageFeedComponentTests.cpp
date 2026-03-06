#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/dsl/ComponentHandler.h"
#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/dsl/WidgetParser.h"
#include "ohui/message/MessageStream.h"
#include "ohui/hud/HUDWidgetManager.h"
#include "ohui/widget/WidgetRegistry.h"

#include "../src/dsl/handlers/MessageFeedHandler.h"

using namespace ohui;
using namespace ohui::dsl;
using namespace ohui::binding;
using namespace ohui::message;

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

struct FeedHarness {
    MessageStream stream;
    TokenStore tokens;
    DataBindingEngine bindings;
    ComponentRegistry registry;
    DSLRuntimeEngine engine;
    float currentTime{10.0f};

    FeedHarness()
        : registry(MakeRegistry()),
          engine(tokens, bindings, registry)
    {
        // Inject message stream into the MessageFeed handler
        auto* handler = dynamic_cast<MessageFeedHandler*>(registry.Get("MessageFeed"));
        if (handler) handler->SetMessageStream(&stream);

        (void)bindings.RegisterBinding({"hud.realtime", BindingType::Float, ""},
            [&]() -> BindingValue { return currentTime; });
        for (const auto& id : bindings.GetAllBindingIds()) {
            (void)bindings.Subscribe("feed-test", id);
        }
        bindings.Update(0.0f);
    }

    void UpdateBindings() { bindings.Update(0.0f); }

    uint64_t Publish(const std::string& content, const std::string& typeId = "general",
                     float lifetime = 5.0f, double realTime = -1.0) {
        Message msg;
        msg.typeId = typeId;
        msg.content = content;
        msg.lifetimeHint = lifetime;
        msg.realTime = realTime >= 0.0 ? realTime : static_cast<double>(currentTime);
        return stream.Publish(std::move(msg));
    }

    DrawCallList Evaluate() {
        UpdateBindings();
        auto parsed = ParseInput(R"(
            widget TestFeed {
                MessageFeed {
                    maxVisible: 5;
                    fadeInDuration: 0.3;
                    fadeOutDuration: 0.5;
                    defaultLifetime: 5.0;
                    currentTime: bind(hud.realtime);
                    itemHeight: 24;
                    width: 350;
                    height: 130;
                }
            }
        )");
        REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
        auto result = engine.Evaluate("TestFeed", {0, 0, 400, 300}, 0.0f);
        REQUIRE(result.has_value());
        return std::move(*result);
    }

    DrawCallList EvaluateWithProps(int maxVis, const std::string& filterTypes = "") {
        UpdateBindings();
        std::string filterProp;
        if (!filterTypes.empty()) {
            filterProp = "filterTypes: " + filterTypes + ";";
        }
        auto parsed = ParseInput(
            "widget TestFeed { MessageFeed { maxVisible: " + std::to_string(maxVis) +
            "; fadeInDuration: 0.3; fadeOutDuration: 0.5; defaultLifetime: 5.0;"
            " currentTime: bind(hud.realtime); itemHeight: 24; width: 350; height: 130; " +
            filterProp + " } }");
        // Unload if previously loaded
        if (engine.HasWidget("TestFeed")) {
            // Re-create engine to clear state
        }
        REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
        auto result = engine.Evaluate("TestFeed", {0, 0, 400, 300}, 0.0f);
        REQUIRE(result.has_value());
        return std::move(*result);
    }
};

// ===========================================================================
// 1. No messages -> 0 draw calls
// ===========================================================================

TEST_CASE("MessageFeed no messages produces 0 draw calls", "[hud-message]") {
    FeedHarness h;
    auto result = h.Evaluate();
    CHECK(result.calls.empty());
}

// ===========================================================================
// 2. Single message -> 1 DrawRect + 1 DrawText with correct content
// ===========================================================================

TEST_CASE("MessageFeed single message produces 1 rect and 1 text", "[hud-message]") {
    FeedHarness h;
    h.Publish("Hello World", "general", 5.0f, 9.0);  // elapsed = 10 - 9 = 1.0
    auto result = h.Evaluate();
    CHECK(CountDrawRects(result) == 1);
    CHECK(CountDrawTexts(result) == 1);
    auto* text = FindDrawText(result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Hello World");
}

// ===========================================================================
// 3. Multiple messages stack vertically (y offset by itemHeight)
// ===========================================================================

TEST_CASE("MessageFeed multiple messages stack vertically", "[hud-message]") {
    FeedHarness h;
    h.Publish("Msg A", "general", 5.0f, 8.0);
    h.Publish("Msg B", "general", 5.0f, 9.0);
    auto result = h.Evaluate();
    CHECK(CountDrawRects(result) == 2);
    CHECK(CountDrawTexts(result) == 2);

    // GetAllMessages returns most-recent-first, so "Msg B" is first
    auto* bg0 = FindDrawRect(result, 0);
    auto* bg1 = FindDrawRect(result, 1);
    REQUIRE(bg0 != nullptr);
    REQUIRE(bg1 != nullptr);
    // Second message should be offset by itemHeight (24)
    CHECK_THAT(bg1->y - bg0->y, Catch::Matchers::WithinAbs(24.0f, 0.1f));
}

// ===========================================================================
// 4. maxVisible caps displayed messages
// ===========================================================================

TEST_CASE("MessageFeed maxVisible caps displayed messages", "[hud-message]") {
    FeedHarness h;
    for (int i = 0; i < 5; ++i) {
        h.Publish("Msg " + std::to_string(i), "general", 5.0f, 9.0);
    }
    auto result = h.EvaluateWithProps(2);
    CHECK(CountDrawRects(result) == 2);
    CHECK(CountDrawTexts(result) == 2);
}

// ===========================================================================
// 5. Fade-in opacity (elapsed < fadeInDuration -> partial opacity)
// ===========================================================================

TEST_CASE("MessageFeed fade-in produces partial opacity", "[hud-message]") {
    FeedHarness h;
    h.currentTime = 10.0f;
    // elapsed = 10.0 - 9.85 = 0.15, fadeIn = 0.3, opacity = 0.15/0.3 = 0.5
    h.Publish("Fading In", "general", 5.0f, 9.85);
    auto result = h.Evaluate();
    auto* bg = FindDrawRect(result);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(0.5f, 0.05f));
}

// ===========================================================================
// 6. Full opacity in steady state
// ===========================================================================

TEST_CASE("MessageFeed full opacity in steady state", "[hud-message]") {
    FeedHarness h;
    h.currentTime = 10.0f;
    // elapsed = 10.0 - 8.0 = 2.0, well within [0.3, 4.5] steady range
    h.Publish("Steady", "general", 5.0f, 8.0);
    auto result = h.Evaluate();
    auto* bg = FindDrawRect(result);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(1.0f, 0.01f));
}

// ===========================================================================
// 7. Fade-out opacity near end of lifetime
// ===========================================================================

TEST_CASE("MessageFeed fade-out produces partial opacity near end", "[hud-message]") {
    FeedHarness h;
    h.currentTime = 10.0f;
    // elapsed = 10.0 - 5.25 = 4.75, lifetime=5.0, fadeOut starts at 4.5
    // opacity = (5.0 - 4.75) / 0.5 = 0.5
    h.Publish("Fading Out", "general", 5.0f, 5.25);
    auto result = h.Evaluate();
    auto* bg = FindDrawRect(result);
    REQUIRE(bg != nullptr);
    CHECK_THAT(bg->opacity, Catch::Matchers::WithinAbs(0.5f, 0.05f));
}

// ===========================================================================
// 8. Expired messages not drawn
// ===========================================================================

TEST_CASE("MessageFeed expired messages not drawn", "[hud-message]") {
    FeedHarness h;
    h.currentTime = 10.0f;
    // elapsed = 10.0 - 4.0 = 6.0 >= lifetime 5.0 -> expired
    h.Publish("Expired", "general", 5.0f, 4.0);
    auto result = h.Evaluate();
    CHECK(result.calls.empty());
}

// ===========================================================================
// 9. Message content appears in DrawText
// ===========================================================================

TEST_CASE("MessageFeed message content appears in DrawText", "[hud-message]") {
    FeedHarness h;
    h.Publish("Quest Started: Dragonborn", "general", 5.0f, 9.0);
    auto result = h.Evaluate();
    auto* text = FindDrawText(result);
    REQUIRE(text != nullptr);
    CHECK(text->text == "Quest Started: Dragonborn");
}

// ===========================================================================
// 10. Null MessageStream -> 0 draw calls
// ===========================================================================

TEST_CASE("MessageFeed null stream produces 0 draw calls", "[hud-message]") {
    TokenStore tokens;
    DataBindingEngine bindings;
    auto registry = MakeRegistry();

    // Do NOT set the message stream - it stays null
    float currentTime = 10.0f;
    (void)bindings.RegisterBinding({"hud.realtime", BindingType::Float, ""},
        [&]() -> BindingValue { return currentTime; });
    for (const auto& id : bindings.GetAllBindingIds()) {
        (void)bindings.Subscribe("null-test", id);
    }
    bindings.Update(0.0f);

    DSLRuntimeEngine engine(tokens, bindings, registry);
    auto parsed = ParseInput(R"(
        widget TestFeed {
            MessageFeed {
                maxVisible: 5;
                currentTime: bind(hud.realtime);
                width: 350;
                height: 130;
            }
        }
    )");
    REQUIRE(engine.LoadWidget(parsed.ast.widgets[0]).has_value());
    auto result = engine.Evaluate("TestFeed", {0, 0, 400, 300}, 0.0f);
    REQUIRE(result.has_value());
    CHECK(result->calls.empty());
}

// ===========================================================================
// 11. Widget integrates through HUDWidgetManager
// ===========================================================================

TEST_CASE("MessageFeed integrates through HUDWidgetManager", "[hud-message]") {
    TokenStore tokens;
    DataBindingEngine bindings;
    ComponentRegistry registry;
    registry.RegisterBuiltins();

    MessageStream stream;
    auto* feedHandler = dynamic_cast<MessageFeedHandler*>(registry.Get("MessageFeed"));
    REQUIRE(feedHandler != nullptr);
    feedHandler->SetMessageStream(&stream);

    DSLRuntimeEngine dslEngine(tokens, bindings, registry);
    widget::WidgetRegistry widgetRegistry;
    hud::HUDWidgetManager manager(dslEngine, widgetRegistry, bindings);

    float currentTime = 10.0f;
    (void)bindings.RegisterBinding({"player.health.pct", BindingType::Float, ""}, []() -> BindingValue { return 0.75f; });
    (void)bindings.RegisterBinding({"player.stamina.pct", BindingType::Float, ""}, []() -> BindingValue { return 0.6f; });
    (void)bindings.RegisterBinding({"player.magicka.pct", BindingType::Float, ""}, []() -> BindingValue { return 0.5f; });
    (void)bindings.RegisterBinding({"player.heading", BindingType::Float, ""}, []() -> BindingValue { return 0.0f; });
    (void)bindings.RegisterBinding({"player.shout.cooldown.pct", BindingType::Float, ""}, []() -> BindingValue { return 0.0f; });
    (void)bindings.RegisterBinding({"equipped.shout.words", BindingType::Int, ""}, []() -> BindingValue { return int64_t{3}; });
    (void)bindings.RegisterBinding({"player.detection.level", BindingType::Float, ""}, []() -> BindingValue { return 0.0f; });
    (void)bindings.RegisterBinding({"enemy.name", BindingType::String, ""}, []() -> BindingValue { return std::string("Dragon"); });
    (void)bindings.RegisterBinding({"enemy.health.pct", BindingType::Float, ""}, []() -> BindingValue { return 0.8f; });
    (void)bindings.RegisterBinding({"enemy.level", BindingType::Int, ""}, []() -> BindingValue { return int64_t{50}; });
    (void)bindings.RegisterBinding({"player.breath.pct", BindingType::Float, ""}, []() -> BindingValue { return 1.0f; });
    (void)bindings.RegisterBinding({"player.sneaking", BindingType::Bool, ""}, []() -> BindingValue { return false; });
    (void)bindings.RegisterBinding({"player.has.target", BindingType::Bool, ""}, []() -> BindingValue { return false; });
    (void)bindings.RegisterBinding({"hud.realtime", BindingType::Float, ""}, [&]() -> BindingValue { return currentTime; });
    for (const auto& id : bindings.GetAllBindingIds()) {
        (void)bindings.Subscribe("hud-test", id);
    }
    bindings.Update(0.0f);

    REQUIRE(manager.LoadDefaults().has_value());
    CHECK(manager.HasWidget("ohui_hud_messages"));

    // Publish a message and evaluate
    Message msg;
    msg.content = "Item Added";
    msg.lifetimeHint = 5.0f;
    msg.realTime = 9.0;
    stream.Publish(std::move(msg));

    auto result = manager.Evaluate("ohui_hud_messages", 1280, 720, 0.0f);
    REQUIRE(result.has_value());
    CHECK(CountDrawRects(*result) >= 1);
    CHECK(CountDrawTexts(*result) >= 1);
}

// ===========================================================================
// 12. Filter by type
// ===========================================================================

TEST_CASE("MessageFeed filterTypes shows only matching messages", "[hud-message]") {
    FeedHarness h;
    h.stream.RegisterType({"quest", "Quest", true, 5.0f});
    h.stream.RegisterType({"combat", "Combat", true, 5.0f});

    h.Publish("Quest started", "quest", 5.0f, 9.0);
    h.Publish("Enemy hit", "combat", 5.0f, 9.0);
    h.Publish("Quest completed", "quest", 5.0f, 9.0);

    auto result = h.EvaluateWithProps(5, "quest");
    // Only quest messages should appear (2 quest, 0 combat)
    CHECK(CountDrawTexts(result) == 2);

    bool foundQuest = false;
    bool foundCombat = false;
    for (const auto& call : result.calls) {
        if (call.type == DrawCallType::Text) {
            const auto& dt = std::get<DrawText>(call.data);
            if (dt.text.find("Quest") != std::string::npos) foundQuest = true;
            if (dt.text.find("Enemy") != std::string::npos) foundCombat = true;
        }
    }
    CHECK(foundQuest);
    CHECK_FALSE(foundCombat);
}
