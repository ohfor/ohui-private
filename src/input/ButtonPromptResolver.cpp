#include "ohui/input/ButtonPromptResolver.h"

namespace ohui::input {

ButtonPromptResolver::ButtonPromptResolver(const InputContextStack& stack)
    : m_stack(stack) {}

void ButtonPromptResolver::OnInputEvent(const InputEvent& event) {
    if (event.IsKeyboard()) {
        m_activeDevice = InputDevice::Keyboard;
    } else if (event.IsGamepad()) {
        switch (m_controllerType) {
            case ControllerType::PlayStation: m_activeDevice = InputDevice::PlayStation; break;
            case ControllerType::SteamDeck:   m_activeDevice = InputDevice::SteamDeck; break;
            default:                          m_activeDevice = InputDevice::Xbox; break;
        }
    }
}

void ButtonPromptResolver::SetControllerType(ControllerType type) {
    m_controllerType = type;
}

InputDevice ButtonPromptResolver::GetActiveDevice() const { return m_activeDevice; }
ControllerType ButtonPromptResolver::GetControllerType() const { return m_controllerType; }

std::optional<GlyphInfo> ButtonPromptResolver::Resolve(std::string_view actionId) const {
    const auto* ctx = m_stack.GetActiveContext();
    if (!ctx) return std::nullopt;

    const auto* action = ctx->GetAction(actionId);
    if (!action) return std::nullopt;

    GlyphInfo info;
    info.device = m_activeDevice;

    if (m_activeDevice == InputDevice::Keyboard) {
        auto key = ctx->GetKeyBinding(actionId);
        if (key == KeyCode::None) return std::nullopt;
        info.displayName = GetKeyDisplayName(key);
        info.glyphId = GetGlyphId(key);
    } else {
        auto button = ctx->GetButtonBinding(actionId);
        if (button == GamepadButton::None) return std::nullopt;
        info.displayName = GetButtonDisplayName(button, m_controllerType);
        info.glyphId = GetGlyphId(button, m_controllerType);
    }
    return info;
}

std::optional<GlyphInfo> ButtonPromptResolver::ResolveInContext(
    std::string_view contextId, std::string_view actionId) const {
    const auto* ctx = m_stack.FindContext(contextId);
    if (!ctx) return std::nullopt;

    const auto* action = ctx->GetAction(actionId);
    if (!action) return std::nullopt;

    GlyphInfo info;
    info.device = m_activeDevice;

    if (m_activeDevice == InputDevice::Keyboard) {
        auto key = ctx->GetKeyBinding(actionId);
        if (key == KeyCode::None) return std::nullopt;
        info.displayName = GetKeyDisplayName(key);
        info.glyphId = GetGlyphId(key);
    } else {
        auto button = ctx->GetButtonBinding(actionId);
        if (button == GamepadButton::None) return std::nullopt;
        info.displayName = GetButtonDisplayName(button, m_controllerType);
        info.glyphId = GetGlyphId(button, m_controllerType);
    }
    return info;
}

std::string ButtonPromptResolver::GetKeyDisplayName(KeyCode key) {
    switch (key) {
        case KeyCode::A: return "A"; case KeyCode::B: return "B";
        case KeyCode::C: return "C"; case KeyCode::D: return "D";
        case KeyCode::E: return "E"; case KeyCode::F: return "F";
        case KeyCode::G: return "G"; case KeyCode::H: return "H";
        case KeyCode::I: return "I"; case KeyCode::J: return "J";
        case KeyCode::K: return "K"; case KeyCode::L: return "L";
        case KeyCode::M: return "M"; case KeyCode::N: return "N";
        case KeyCode::O: return "O"; case KeyCode::P: return "P";
        case KeyCode::Q: return "Q"; case KeyCode::R: return "R";
        case KeyCode::S: return "S"; case KeyCode::T: return "T";
        case KeyCode::U: return "U"; case KeyCode::V: return "V";
        case KeyCode::W: return "W"; case KeyCode::X: return "X";
        case KeyCode::Y: return "Y"; case KeyCode::Z: return "Z";
        case KeyCode::Num0: return "0"; case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2"; case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4"; case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6"; case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8"; case KeyCode::Num9: return "9";
        case KeyCode::F1: return "F1"; case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3"; case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5"; case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7"; case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9"; case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11"; case KeyCode::F12: return "F12";
        case KeyCode::Escape: return "Esc";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Space: return "Space";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Del";
        case KeyCode::Insert: return "Ins";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PgUp";
        case KeyCode::PageDown: return "PgDn";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::LeftShift: return "Shift";
        case KeyCode::RightShift: return "RShift";
        case KeyCode::LeftCtrl: return "Ctrl";
        case KeyCode::RightCtrl: return "RCtrl";
        case KeyCode::LeftAlt: return "Alt";
        case KeyCode::RightAlt: return "RAlt";
        case KeyCode::Tilde: return "~";
        case KeyCode::Minus: return "-";
        case KeyCode::Equals: return "=";
        case KeyCode::LeftBracket: return "[";
        case KeyCode::RightBracket: return "]";
        case KeyCode::Backslash: return "\\";
        case KeyCode::Semicolon: return ";";
        case KeyCode::Apostrophe: return "'";
        case KeyCode::Comma: return ",";
        case KeyCode::Period: return ".";
        case KeyCode::Slash: return "/";
        case KeyCode::MouseLeft: return "LMB";
        case KeyCode::MouseRight: return "RMB";
        case KeyCode::MouseMiddle: return "MMB";
        case KeyCode::Mouse4: return "Mouse4";
        case KeyCode::Mouse5: return "Mouse5";
        case KeyCode::MouseWheelUp: return "WheelUp";
        case KeyCode::MouseWheelDown: return "WheelDn";
        default: return "?";
    }
}

static std::string ToLower(std::string_view s) {
    std::string result(s);
    for (auto& c : result) {
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    return result;
}

std::string ButtonPromptResolver::GetButtonDisplayName(GamepadButton button, ControllerType type) {
    switch (button) {
        case GamepadButton::A:
            return type == ControllerType::PlayStation ? "Cross" : "A";
        case GamepadButton::B:
            return type == ControllerType::PlayStation ? "Circle" : "B";
        case GamepadButton::X:
            return type == ControllerType::PlayStation ? "Square" : "X";
        case GamepadButton::Y:
            return type == ControllerType::PlayStation ? "Triangle" : "Y";
        case GamepadButton::DpadUp: return "DpadUp";
        case GamepadButton::DpadDown: return "DpadDown";
        case GamepadButton::DpadLeft: return "DpadLeft";
        case GamepadButton::DpadRight: return "DpadRight";
        case GamepadButton::Start:
            return type == ControllerType::PlayStation ? "Options" : "Start";
        case GamepadButton::Back:
            return type == ControllerType::PlayStation ? "Share" : "Back";
        case GamepadButton::LeftThumb: return "LS";
        case GamepadButton::RightThumb: return "RS";
        case GamepadButton::LeftShoulder:
            return type == ControllerType::PlayStation ? "L1" : "LB";
        case GamepadButton::RightShoulder:
            return type == ControllerType::PlayStation ? "R1" : "RB";
        case GamepadButton::LeftTrigger:
            return type == ControllerType::PlayStation ? "L2" : "LT";
        case GamepadButton::RightTrigger:
            return type == ControllerType::PlayStation ? "R2" : "RT";
        default: return "?";
    }
}

std::string ButtonPromptResolver::GetGlyphId(KeyCode key) {
    return "key." + ToLower(GetKeyDisplayName(key));
}

std::string ButtonPromptResolver::GetGlyphId(GamepadButton button, ControllerType type) {
    std::string prefix;
    switch (type) {
        case ControllerType::PlayStation: prefix = "ps."; break;
        case ControllerType::SteamDeck:   prefix = "deck."; break;
        default:                          prefix = "xbox."; break;
    }
    return prefix + ToLower(GetButtonDisplayName(button, type));
}

}  // namespace ohui::input
