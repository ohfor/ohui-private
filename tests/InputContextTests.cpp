#include <catch2/catch_test_macros.hpp>

#include "ohui/input/InputContextStack.h"

#include <memory>
#include <string>

using namespace ohui;
using namespace ohui::input;

// Helper to create a context with a single keyboard action
static std::shared_ptr<InputContext> makeContext(
    const std::string& id,
    const std::string& actionId = "",
    KeyCode key = KeyCode::None,
    ActionHandler handler = nullptr)
{
    auto ctx = std::make_shared<InputContext>(id);
    if (!actionId.empty()) {
        ActionBinding binding;
        binding.actionId = actionId;
        binding.displayName = actionId;
        binding.defaultKey = key;
        binding.handler = std::move(handler);
        ctx->RegisterAction(binding);
    }
    return ctx;
}

// -- InputContextStack tests --

TEST_CASE("Constructor creates gameplay context at bottom", "[input]") {
    InputContextStack stack;
    REQUIRE(stack.Depth() == 1);
    CHECK(stack.GetActiveContext()->GetId() == InputContextStack::kGameplayContextId);
    CHECK(stack.HasContext("gameplay"));
}

TEST_CASE("Push adds context to top of stack", "[input]") {
    InputContextStack stack;
    auto ctx = makeContext("menu");
    auto result = stack.Push(ctx);
    REQUIRE(result.has_value());
    CHECK(stack.Depth() == 2);
    CHECK(stack.GetActiveContext()->GetId() == "menu");
}

TEST_CASE("Push duplicate context returns DuplicateRegistration", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("menu"));
    auto result = stack.Push(makeContext("menu"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("Pop removes context from stack", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("menu"));
    auto result = stack.Pop("menu");
    REQUIRE(result.has_value());
    CHECK(stack.Depth() == 1);
    CHECK(stack.GetActiveContext()->GetId() == "gameplay");
}

TEST_CASE("Pop unknown context returns ContextNotFound", "[input]") {
    InputContextStack stack;
    auto result = stack.Pop("nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::ContextNotFound);
}

TEST_CASE("Pop gameplay context returns error", "[input]") {
    InputContextStack stack;
    auto result = stack.Pop("gameplay");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::ContextNotFound);
}

TEST_CASE("Pop non-top context logs warning and removes it", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("dialog"));
    stack.Push(makeContext("overlay"));
    // Pop dialog which is not on top -- should succeed with warning
    auto result = stack.Pop("dialog");
    REQUIRE(result.has_value());
    CHECK(stack.Depth() == 2);
    CHECK(stack.GetActiveContext()->GetId() == "overlay");
    CHECK(!stack.HasContext("dialog"));
}

TEST_CASE("GetActiveContext returns top of stack", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("a"));
    stack.Push(makeContext("b"));
    CHECK(stack.GetActiveContext()->GetId() == "b");
}

TEST_CASE("GetActiveContext returns gameplay when stack has only gameplay", "[input]") {
    InputContextStack stack;
    CHECK(stack.GetActiveContext()->GetId() == "gameplay");
}

TEST_CASE("HandleInput routes to top context only", "[input]") {
    InputContextStack stack;

    bool gameplayFired = false;
    stack.GetGameplayContext().RegisterAction(ActionBinding{
        "gp_action", "GP", KeyCode::E, GamepadButton::None,
        [&](const InputEvent&) { gameplayFired = true; }
    });

    bool menuFired = false;
    auto menu = makeContext("menu", "menu_action", KeyCode::E,
        [&](const InputEvent&) { menuFired = true; });
    stack.Push(menu);

    InputEvent ev;
    ev.key = KeyCode::E;
    ev.type = InputEventType::Press;
    bool handled = stack.HandleInput(ev);

    CHECK(handled);
    CHECK(menuFired);
    CHECK(!gameplayFired);
}

TEST_CASE("HandleInput does not fire handlers in lower contexts", "[input]") {
    InputContextStack stack;

    bool bottomFired = false;
    stack.GetGameplayContext().RegisterAction(ActionBinding{
        "bottom", "Bottom", KeyCode::A, GamepadButton::None,
        [&](const InputEvent&) { bottomFired = true; }
    });

    // Top context binds a different key
    auto top = makeContext("top", "top_action", KeyCode::B,
        [](const InputEvent&) {});
    stack.Push(top);

    InputEvent ev;
    ev.key = KeyCode::A;
    bool handled = stack.HandleInput(ev);

    CHECK(!handled);
    CHECK(!bottomFired);
}

TEST_CASE("HandleInput returns false when no action matches", "[input]") {
    InputContextStack stack;
    InputEvent ev;
    ev.key = KeyCode::Z;
    CHECK(!stack.HandleInput(ev));
}

// -- InputContext tests --

TEST_CASE("InputContext RegisterAction stores binding", "[input]") {
    InputContext ctx("test");
    ActionBinding binding;
    binding.actionId = "jump";
    binding.displayName = "Jump";
    binding.defaultKey = KeyCode::Space;
    ctx.RegisterAction(binding);

    CHECK(ctx.HasAction("jump"));
    CHECK(ctx.ActionCount() == 1);
    auto* action = ctx.GetAction("jump");
    REQUIRE(action != nullptr);
    CHECK(action->displayName == "Jump");
}

TEST_CASE("InputContext HandleInput matches key binding", "[input]") {
    InputContext ctx("test");
    bool fired = false;
    ctx.RegisterAction(ActionBinding{
        "attack", "Attack", KeyCode::MouseLeft, GamepadButton::None,
        [&](const InputEvent&) { fired = true; }
    });

    InputEvent ev;
    ev.key = KeyCode::MouseLeft;
    CHECK(ctx.HandleInput(ev));
    CHECK(fired);
}

TEST_CASE("InputContext HandleInput matches button binding", "[input]") {
    InputContext ctx("test");
    bool fired = false;
    ctx.RegisterAction(ActionBinding{
        "accept", "Accept", KeyCode::None, GamepadButton::A,
        [&](const InputEvent& e) { fired = true; }
    });

    InputEvent ev;
    ev.button = GamepadButton::A;
    CHECK(ctx.HandleInput(ev));
    CHECK(fired);
}

TEST_CASE("InputContext SetKeyBinding overrides default", "[input]") {
    InputContext ctx("test");
    bool fired = false;
    ctx.RegisterAction(ActionBinding{
        "use", "Use", KeyCode::E, GamepadButton::None,
        [&](const InputEvent&) { fired = true; }
    });

    ctx.SetKeyBinding("use", KeyCode::F);

    // Old key should not fire
    InputEvent oldEv;
    oldEv.key = KeyCode::E;
    CHECK(!ctx.HandleInput(oldEv));

    // New key should fire
    InputEvent newEv;
    newEv.key = KeyCode::F;
    CHECK(ctx.HandleInput(newEv));
    CHECK(fired);
}

TEST_CASE("InputContext SetButtonBinding overrides default", "[input]") {
    InputContext ctx("test");
    bool fired = false;
    ctx.RegisterAction(ActionBinding{
        "accept", "Accept", KeyCode::None, GamepadButton::A,
        [&](const InputEvent&) { fired = true; }
    });

    ctx.SetButtonBinding("accept", GamepadButton::B);

    InputEvent oldEv;
    oldEv.button = GamepadButton::A;
    CHECK(!ctx.HandleInput(oldEv));

    InputEvent newEv;
    newEv.button = GamepadButton::B;
    CHECK(ctx.HandleInput(newEv));
    CHECK(fired);
}

TEST_CASE("InputContext ResetToDefaults restores original bindings", "[input]") {
    InputContext ctx("test");
    bool fired = false;
    ctx.RegisterAction(ActionBinding{
        "use", "Use", KeyCode::E, GamepadButton::None,
        [&](const InputEvent&) { fired = true; }
    });

    ctx.SetKeyBinding("use", KeyCode::F);
    ctx.ResetToDefaults();

    InputEvent ev;
    ev.key = KeyCode::E;
    CHECK(ctx.HandleInput(ev));
    CHECK(fired);
}

TEST_CASE("GetStackOrder returns context IDs bottom to top", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("alpha"));
    stack.Push(makeContext("beta"));

    auto order = stack.GetStackOrder();
    REQUIRE(order.size() == 3);
    CHECK(order[0] == "gameplay");
    CHECK(order[1] == "alpha");
    CHECK(order[2] == "beta");
}

TEST_CASE("HasContext returns true for pushed, false for unknown", "[input]") {
    InputContextStack stack;
    stack.Push(makeContext("menu"));
    CHECK(stack.HasContext("menu"));
    CHECK(stack.HasContext("gameplay"));
    CHECK(!stack.HasContext("nonexistent"));
}
