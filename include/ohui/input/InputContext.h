#pragma once
#include "ohui/input/InputTypes.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace ohui::input {

struct ActionBinding {
    std::string actionId;
    std::string displayName;
    KeyCode defaultKey{KeyCode::None};
    GamepadButton defaultButton{GamepadButton::None};
    ActionHandler handler;
};

class InputContext {
public:
    explicit InputContext(std::string id, std::string displayName = "");

    const std::string& GetId() const;
    const std::string& GetDisplayName() const;

    void RegisterAction(const ActionBinding& binding);
    bool HasAction(std::string_view actionId) const;
    const ActionBinding* GetAction(std::string_view actionId) const;
    std::vector<const ActionBinding*> GetAllActions() const;

    // Dispatch input to matching action handler using current bindings
    bool HandleInput(const InputEvent& event) const;

    // Rebinding: override the physical input for an action
    void SetKeyBinding(std::string_view actionId, KeyCode key);
    void SetButtonBinding(std::string_view actionId, GamepadButton button);
    KeyCode GetKeyBinding(std::string_view actionId) const;
    GamepadButton GetButtonBinding(std::string_view actionId) const;
    void ResetToDefaults();

    size_t ActionCount() const;

private:
    std::string m_id;
    std::string m_displayName;
    std::unordered_map<std::string, ActionBinding> m_actions;
    // Current bindings (may differ from defaults after rebinding)
    std::unordered_map<std::string, KeyCode> m_keyBindings;
    std::unordered_map<std::string, GamepadButton> m_buttonBindings;
};

}  // namespace ohui::input
