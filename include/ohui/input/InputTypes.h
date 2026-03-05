#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace ohui::input {

enum class KeyCode : uint16_t {
    None = 0,
    // Letters
    A = 0x1E, B = 0x30, C = 0x2E, D = 0x20, E = 0x12, F = 0x21,
    G = 0x22, H = 0x23, I = 0x17, J = 0x24, K = 0x25, L = 0x26,
    M = 0x32, N = 0x31, O = 0x18, P = 0x19, Q = 0x10, R = 0x13,
    S = 0x1F, T = 0x14, U = 0x16, V = 0x2F, W = 0x11, X = 0x2D,
    Y = 0x15, Z = 0x2C,
    // Numbers
    Num1 = 0x02, Num2 = 0x03, Num3 = 0x04, Num4 = 0x05,
    Num5 = 0x06, Num6 = 0x07, Num7 = 0x08, Num8 = 0x09,
    Num9 = 0x0A, Num0 = 0x0B,
    // Function keys
    F1 = 0x3B, F2 = 0x3C, F3 = 0x3D, F4 = 0x3E,
    F5 = 0x3F, F6 = 0x40, F7 = 0x41, F8 = 0x42,
    F9 = 0x43, F10 = 0x44, F11 = 0x57, F12 = 0x58,
    // Modifiers
    LeftShift = 0x2A, RightShift = 0x36,
    LeftCtrl = 0x1D, RightCtrl = 0x9D,
    LeftAlt = 0x38, RightAlt = 0xB8,
    // Navigation
    Escape = 0x01, Tab = 0x0F, Enter = 0x1C, Space = 0x39,
    Backspace = 0x0E, Delete = 0xD3, Insert = 0xD2,
    Home = 0xC7, End = 0xCF, PageUp = 0xC9, PageDown = 0xD1,
    Up = 0xC8, Down = 0xD0, Left = 0xCB, Right = 0xCD,
    // Misc
    Tilde = 0x29, Minus = 0x0C, Equals = 0x0D,
    LeftBracket = 0x1A, RightBracket = 0x1B, Backslash = 0x2B,
    Semicolon = 0x27, Apostrophe = 0x28, Comma = 0x33,
    Period = 0x34, Slash = 0x35,
    // Mouse (offset by 0x100 to avoid scan code collision)
    MouseLeft = 0x100, MouseRight = 0x101, MouseMiddle = 0x102,
    Mouse4 = 0x103, Mouse5 = 0x104,
    MouseWheelUp = 0x108, MouseWheelDown = 0x109,
};

enum class GamepadButton : uint16_t {
    None = 0,
    DpadUp    = 0x0001,
    DpadDown  = 0x0002,
    DpadLeft  = 0x0004,
    DpadRight = 0x0008,
    Start     = 0x0010,
    Back      = 0x0020,
    LeftThumb = 0x0040,
    RightThumb = 0x0080,
    LeftShoulder  = 0x0100,
    RightShoulder = 0x0200,
    A = 0x1000,
    B = 0x2000,
    X = 0x4000,
    Y = 0x8000,
    LeftTrigger  = 0x0009,
    RightTrigger = 0x000A,
};

enum class InputEventType { Press, Release };

struct InputEvent {
    KeyCode key{KeyCode::None};
    GamepadButton button{GamepadButton::None};
    InputEventType type{InputEventType::Press};

    bool IsKeyboard() const { return key != KeyCode::None; }
    bool IsGamepad() const { return button != GamepadButton::None; }
};

using ActionHandler = std::function<void(const InputEvent&)>;

}  // namespace ohui::input
