#include <catch2/catch_test_macros.hpp>

#include "ohui/input/ButtonPromptResolver.h"

#include <memory>

using namespace ohui::input;

// Helper to set up a stack with an action bound to both key and button
static InputContextStack MakeStackWithAction(
    const std::string& actionId,
    KeyCode key,
    GamepadButton button)
{
    InputContextStack stack;
    stack.GetGameplayContext().RegisterAction(ActionBinding{
        actionId, actionId, key, button, nullptr
    });
    return stack;
}

// -- Device detection --

TEST_CASE("Default active device is Keyboard", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);
    CHECK(resolver.GetActiveDevice() == InputDevice::Keyboard);
}

TEST_CASE("OnInputEvent with keyboard switches to Keyboard", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);

    // Switch to gamepad first
    InputEvent gpEvent;
    gpEvent.button = GamepadButton::A;
    resolver.OnInputEvent(gpEvent);
    CHECK(resolver.GetActiveDevice() == InputDevice::Xbox);

    // Switch back to keyboard
    InputEvent kbEvent;
    kbEvent.key = KeyCode::A;
    resolver.OnInputEvent(kbEvent);
    CHECK(resolver.GetActiveDevice() == InputDevice::Keyboard);
}

TEST_CASE("OnInputEvent with gamepad switches to Xbox (default controller type)", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);

    InputEvent ev;
    ev.button = GamepadButton::A;
    resolver.OnInputEvent(ev);
    CHECK(resolver.GetActiveDevice() == InputDevice::Xbox);
}

TEST_CASE("SetControllerType changes controller type", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);
    resolver.SetControllerType(ControllerType::PlayStation);
    CHECK(resolver.GetControllerType() == ControllerType::PlayStation);

    // Gamepad input should now give PlayStation device
    InputEvent ev;
    ev.button = GamepadButton::A;
    resolver.OnInputEvent(ev);
    CHECK(resolver.GetActiveDevice() == InputDevice::PlayStation);
}

// -- Resolve --

TEST_CASE("Resolve returns GlyphInfo for keyboard when device is Keyboard", "[prompt]") {
    auto stack = MakeStackWithAction("jump", KeyCode::Space, GamepadButton::A);
    ButtonPromptResolver resolver(stack);

    auto glyph = resolver.Resolve("jump");
    REQUIRE(glyph.has_value());
    CHECK(glyph->displayName == "Space");
    CHECK(glyph->glyphId == "key.space");
    CHECK(glyph->device == InputDevice::Keyboard);
}

TEST_CASE("Resolve returns GlyphInfo for gamepad when device is Xbox", "[prompt]") {
    auto stack = MakeStackWithAction("jump", KeyCode::Space, GamepadButton::A);
    ButtonPromptResolver resolver(stack);

    InputEvent ev;
    ev.button = GamepadButton::A;
    resolver.OnInputEvent(ev);

    auto glyph = resolver.Resolve("jump");
    REQUIRE(glyph.has_value());
    CHECK(glyph->displayName == "A");
    CHECK(glyph->glyphId == "xbox.a");
    CHECK(glyph->device == InputDevice::Xbox);
}

TEST_CASE("Resolve returns nullopt for unknown action", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);
    CHECK(!resolver.Resolve("nonexistent").has_value());
}

TEST_CASE("Resolve uses current bindings (after rebind)", "[prompt]") {
    auto stack = MakeStackWithAction("use", KeyCode::E, GamepadButton::None);
    stack.GetGameplayContext().SetKeyBinding("use", KeyCode::F);
    ButtonPromptResolver resolver(stack);

    auto glyph = resolver.Resolve("use");
    REQUIRE(glyph.has_value());
    CHECK(glyph->displayName == "F");
    CHECK(glyph->glyphId == "key.f");
}

TEST_CASE("ResolveInContext looks up specific context", "[prompt]") {
    InputContextStack stack;
    auto menu = std::make_shared<InputContext>("menu");
    menu->RegisterAction(ActionBinding{"back", "Back", KeyCode::Escape, GamepadButton::B, nullptr});
    stack.Push(menu);

    ButtonPromptResolver resolver(stack);
    auto glyph = resolver.ResolveInContext("menu", "back");
    REQUIRE(glyph.has_value());
    CHECK(glyph->displayName == "Esc");
}

TEST_CASE("ResolveInContext returns nullopt for context not on stack", "[prompt]") {
    InputContextStack stack;
    ButtonPromptResolver resolver(stack);
    CHECK(!resolver.ResolveInContext("nonexistent", "action").has_value());
}

// -- GetKeyDisplayName --

TEST_CASE("GetKeyDisplayName for letter keys", "[prompt]") {
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::A) == "A");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Z) == "Z");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::E) == "E");
}

TEST_CASE("GetKeyDisplayName for special keys (Esc, Shift, Space, etc.)", "[prompt]") {
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Escape) == "Esc");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::LeftShift) == "Shift");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Space) == "Space");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Enter) == "Enter");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Tab) == "Tab");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::Delete) == "Del");
}

TEST_CASE("GetKeyDisplayName for mouse buttons", "[prompt]") {
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::MouseLeft) == "LMB");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::MouseRight) == "RMB");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::MouseMiddle) == "MMB");
    CHECK(ButtonPromptResolver::GetKeyDisplayName(KeyCode::MouseWheelUp) == "WheelUp");
}

// -- GetButtonDisplayName --

TEST_CASE("GetButtonDisplayName Xbox A/B/X/Y", "[prompt]") {
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::A, ControllerType::Xbox) == "A");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::B, ControllerType::Xbox) == "B");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::X, ControllerType::Xbox) == "X");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::Y, ControllerType::Xbox) == "Y");
}

TEST_CASE("GetButtonDisplayName PlayStation Cross/Circle/Square/Triangle", "[prompt]") {
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::A, ControllerType::PlayStation) == "Cross");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::B, ControllerType::PlayStation) == "Circle");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::X, ControllerType::PlayStation) == "Square");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::Y, ControllerType::PlayStation) == "Triangle");
}

TEST_CASE("GetButtonDisplayName SteamDeck", "[prompt]") {
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::A, ControllerType::SteamDeck) == "A");
    CHECK(ButtonPromptResolver::GetButtonDisplayName(GamepadButton::LeftShoulder, ControllerType::SteamDeck) == "LB");
}

// -- GetGlyphId --

TEST_CASE("GetGlyphId key returns key.{name}", "[prompt]") {
    CHECK(ButtonPromptResolver::GetGlyphId(KeyCode::E) == "key.e");
    CHECK(ButtonPromptResolver::GetGlyphId(KeyCode::Space) == "key.space");
    CHECK(ButtonPromptResolver::GetGlyphId(KeyCode::Escape) == "key.esc");
}

TEST_CASE("GetGlyphId Xbox button returns xbox.{name}", "[prompt]") {
    CHECK(ButtonPromptResolver::GetGlyphId(GamepadButton::A, ControllerType::Xbox) == "xbox.a");
    CHECK(ButtonPromptResolver::GetGlyphId(GamepadButton::LeftShoulder, ControllerType::Xbox) == "xbox.lb");
}

TEST_CASE("GetGlyphId PlayStation button returns ps.{name}", "[prompt]") {
    CHECK(ButtonPromptResolver::GetGlyphId(GamepadButton::A, ControllerType::PlayStation) == "ps.cross");
    CHECK(ButtonPromptResolver::GetGlyphId(GamepadButton::B, ControllerType::PlayStation) == "ps.circle");
}
