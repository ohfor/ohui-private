#include <catch2/catch_test_macros.hpp>

#include "ohui/binding/DataBindingEngine.h"

using namespace ohui;
using namespace ohui::binding;

TEST_CASE("RegisterBinding stores definition and poll function", "[binding]") {
    DataBindingEngine engine;
    float value = 100.0f;
    auto result = engine.RegisterBinding(
        {"player.health", BindingType::Float, "Player health"},
        [&]() -> BindingValue { return value; });
    REQUIRE(result.has_value());
    CHECK(engine.HasBinding("player.health"));
    CHECK(engine.BindingCount() == 1);
}

TEST_CASE("Duplicate binding returns DuplicateRegistration", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        []() -> BindingValue { return 0.0f; });
    auto result = engine.RegisterBinding({"player.health", BindingType::Float, "Health2"},
        []() -> BindingValue { return 0.0f; });
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("Null poll function returns InvalidBinding", "[binding]") {
    DataBindingEngine engine;
    auto result = engine.RegisterBinding(
        {"player.health", BindingType::Float, "Health"}, nullptr);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidBinding);
}

TEST_CASE("Subscribe links widget to binding", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        []() -> BindingValue { return 100.0f; });
    auto result = engine.Subscribe("health_bar", "player.health");
    REQUIRE(result.has_value());
    CHECK(engine.SubscriptionCount() == 1);
}

TEST_CASE("Subscribe to unknown binding returns BindingNotFound", "[binding]") {
    DataBindingEngine engine;
    auto result = engine.Subscribe("widget", "nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::BindingNotFound);
}

TEST_CASE("Reactive: widget dirty only when value changes", "[binding]") {
    DataBindingEngine engine;
    float value = 100.0f;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        [&]() -> BindingValue { return value; });
    engine.Subscribe("health_bar", "player.health");

    // First poll initializes, previous was default 0.0f, current becomes 100.0f -> dirty
    auto dirty1 = engine.Update(0.016f);
    CHECK(dirty1.size() == 1);

    // Same value, no change -> not dirty
    auto dirty2 = engine.Update(0.016f);
    CHECK(dirty2.empty());
}

TEST_CASE("Reactive: widget not dirty when value unchanged", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        []() -> BindingValue { return 100.0f; });
    engine.Subscribe("health_bar", "player.health");

    engine.Update(0.016f);  // initial poll
    auto dirty = engine.Update(0.016f);
    CHECK(dirty.empty());
}

TEST_CASE("ThrottledReactive: respects max updates per second", "[binding]") {
    DataBindingEngine engine;
    int counter = 0;
    engine.RegisterBinding({"counter", BindingType::Int, "Counter"},
        [&]() -> BindingValue { return int64_t{++counter}; });

    SubscriptionOptions opts;
    opts.mode = UpdateMode::ThrottledReactive;
    opts.maxUpdatesPerSecond = 2.0f;  // max 2/sec = every 0.5s
    engine.Subscribe("widget", "counter", opts);

    // First update: value changes (0 -> 1), timer starts at 0 and interval is 0.5s
    // timeSinceLastNotify = 0 + 0.1 = 0.1, which is < 0.5 -> not dirty
    auto dirty1 = engine.Update(0.1f);
    CHECK(dirty1.empty());

    // Accumulate more time: 0.1 + 0.5 = 0.6 >= 0.5 -> dirty
    auto dirty2 = engine.Update(0.5f);
    CHECK(dirty2.size() == 1);
}

TEST_CASE("ThrottledReactive: fires when throttle window elapsed", "[binding]") {
    DataBindingEngine engine;
    int counter = 0;
    engine.RegisterBinding({"counter", BindingType::Int, "Counter"},
        [&]() -> BindingValue { return int64_t{++counter}; });

    SubscriptionOptions opts;
    opts.mode = UpdateMode::ThrottledReactive;
    opts.maxUpdatesPerSecond = 1.0f;  // 1/sec = every 1.0s
    engine.Subscribe("widget", "counter", opts);

    // Accumulate 1.1 seconds -> should fire
    auto dirty = engine.Update(1.1f);
    CHECK(dirty.size() == 1);
}

TEST_CASE("FrameDriven: widget always dirty", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"constant", BindingType::Float, "Constant"},
        []() -> BindingValue { return 42.0f; });

    SubscriptionOptions opts;
    opts.mode = UpdateMode::FrameDriven;
    engine.Subscribe("widget", "constant", opts);

    auto dirty1 = engine.Update(0.016f);
    CHECK(dirty1.size() == 1);

    auto dirty2 = engine.Update(0.016f);
    CHECK(dirty2.size() == 1);
}

TEST_CASE("Multiple widgets subscribe to same binding", "[binding]") {
    DataBindingEngine engine;
    float value = 100.0f;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        [&]() -> BindingValue { return value; });
    engine.Subscribe("widget_a", "player.health");
    engine.Subscribe("widget_b", "player.health");

    CHECK(engine.SubscriptionCount() == 2);

    // Both widgets dirty on first poll (value 0 -> 100)
    auto dirty = engine.Update(0.016f);
    CHECK(dirty.size() == 2);
}

TEST_CASE("Widget subscribes to multiple bindings", "[binding]") {
    DataBindingEngine engine;
    float health = 100.0f;
    float magicka = 200.0f;
    engine.RegisterBinding({"player.health", BindingType::Float, "Health"},
        [&]() -> BindingValue { return health; });
    engine.RegisterBinding({"player.magicka", BindingType::Float, "Magicka"},
        [&]() -> BindingValue { return magicka; });
    engine.Subscribe("hud", "player.health");
    engine.Subscribe("hud", "player.magicka");

    CHECK(engine.SubscriptionCount() == 2);

    // Both change -> hud is dirty (deduplicated)
    auto dirty = engine.Update(0.016f);
    CHECK(dirty.size() == 1);
    CHECK(dirty[0] == "hud");
}

TEST_CASE("Unsubscribe removes specific subscription", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"a", BindingType::Float, "A"},
        []() -> BindingValue { return 1.0f; });
    engine.RegisterBinding({"b", BindingType::Float, "B"},
        []() -> BindingValue { return 2.0f; });
    engine.Subscribe("widget", "a");
    engine.Subscribe("widget", "b");
    CHECK(engine.SubscriptionCount() == 2);

    engine.Unsubscribe("widget", "a");
    CHECK(engine.SubscriptionCount() == 1);
}

TEST_CASE("UnsubscribeAll removes all subscriptions for widget", "[binding]") {
    DataBindingEngine engine;
    engine.RegisterBinding({"a", BindingType::Float, "A"},
        []() -> BindingValue { return 1.0f; });
    engine.RegisterBinding({"b", BindingType::Float, "B"},
        []() -> BindingValue { return 2.0f; });
    engine.Subscribe("widget", "a");
    engine.Subscribe("widget", "b");
    engine.Subscribe("other", "a");

    engine.UnsubscribeAll("widget");
    CHECK(engine.SubscriptionCount() == 1);
}

TEST_CASE("Binding with no subscribers is not polled", "[binding]") {
    DataBindingEngine engine;
    int pollCount = 0;
    engine.RegisterBinding({"unsubscribed", BindingType::Float, "Unused"},
        [&]() -> BindingValue { ++pollCount; return 0.0f; });

    engine.Update(0.016f);
    CHECK(pollCount == 0);
}

TEST_CASE("Update returns deduplicated widget IDs", "[binding]") {
    DataBindingEngine engine;
    float val1 = 1.0f;
    float val2 = 2.0f;
    engine.RegisterBinding({"a", BindingType::Float, "A"},
        [&]() -> BindingValue { return val1; });
    engine.RegisterBinding({"b", BindingType::Float, "B"},
        [&]() -> BindingValue { return val2; });
    engine.Subscribe("widget", "a");
    engine.Subscribe("widget", "b");

    // Both change from default -> widget appears only once
    auto dirty = engine.Update(0.016f);
    CHECK(dirty.size() == 1);
}

TEST_CASE("GetCurrentValue returns latest polled value", "[binding]") {
    DataBindingEngine engine;
    float value = 42.0f;
    engine.RegisterBinding({"val", BindingType::Float, "Val"},
        [&]() -> BindingValue { return value; });
    engine.Subscribe("widget", "val");

    engine.Update(0.016f);

    auto* current = engine.GetCurrentValue("val");
    REQUIRE(current != nullptr);
    CHECK(std::get<float>(*current) == 42.0f);
}

TEST_CASE("GetCurrentValue returns null for unknown binding", "[binding]") {
    DataBindingEngine engine;
    CHECK(engine.GetCurrentValue("nonexistent") == nullptr);
}

TEST_CASE("Change detection: string binding detects change", "[binding]") {
    DataBindingEngine engine;
    std::string name = "Dragonborn";
    engine.RegisterBinding({"player.name", BindingType::String, "Name"},
        [&]() -> BindingValue { return name; });
    engine.Subscribe("widget", "player.name");

    auto dirty1 = engine.Update(0.016f);  // "" -> "Dragonborn"
    CHECK(dirty1.size() == 1);

    auto dirty2 = engine.Update(0.016f);  // unchanged
    CHECK(dirty2.empty());

    name = "Alduin";
    auto dirty3 = engine.Update(0.016f);  // "Dragonborn" -> "Alduin"
    CHECK(dirty3.size() == 1);
}

TEST_CASE("Change detection: bool binding detects change", "[binding]") {
    DataBindingEngine engine;
    bool sneaking = false;
    engine.RegisterBinding({"player.sneaking", BindingType::Bool, "Sneaking"},
        [&]() -> BindingValue { return sneaking; });
    engine.Subscribe("widget", "player.sneaking");

    auto dirty1 = engine.Update(0.016f);  // false -> false (default matches)
    CHECK(dirty1.empty());

    sneaking = true;
    auto dirty2 = engine.Update(0.016f);  // false -> true
    CHECK(dirty2.size() == 1);
}
