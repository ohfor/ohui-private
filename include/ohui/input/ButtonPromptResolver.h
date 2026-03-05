#pragma once
#include "ohui/input/InputTypes.h"
#include "ohui/input/InputContext.h"
#include "ohui/input/InputContextStack.h"
#include <optional>
#include <string>

namespace ohui::input {

enum class InputDevice { Keyboard, Xbox, PlayStation, SteamDeck };
enum class ControllerType { Xbox, PlayStation, SteamDeck, Unknown };

struct GlyphInfo {
    std::string displayName;     // e.g. "A", "Cross", "E", "LB"
    std::string glyphId;         // e.g. "xbox.a", "ps.cross", "key.e"
    InputDevice device;
};

class ButtonPromptResolver {
public:
    explicit ButtonPromptResolver(const InputContextStack& stack);

    // Device detection
    void OnInputEvent(const InputEvent& event);
    void SetControllerType(ControllerType type);
    InputDevice GetActiveDevice() const;
    ControllerType GetControllerType() const;

    // Resolve action to glyph for current device
    std::optional<GlyphInfo> Resolve(std::string_view actionId) const;
    std::optional<GlyphInfo> ResolveInContext(std::string_view contextId,
                                              std::string_view actionId) const;

    // Display name lookup (device-independent helpers)
    static std::string GetKeyDisplayName(KeyCode key);
    static std::string GetButtonDisplayName(GamepadButton button, ControllerType type);
    static std::string GetGlyphId(KeyCode key);
    static std::string GetGlyphId(GamepadButton button, ControllerType type);

private:
    const InputContextStack& m_stack;
    InputDevice m_activeDevice{InputDevice::Keyboard};
    ControllerType m_controllerType{ControllerType::Xbox};
};

}  // namespace ohui::input
