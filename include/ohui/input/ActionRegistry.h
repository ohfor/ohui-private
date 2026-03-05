#pragma once
#include "ohui/input/InputTypes.h"
#include "ohui/input/InputContext.h"
#include "ohui/core/Result.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ohui::input {

struct ActionDefinition {
    std::string contextId;
    std::string actionId;
    std::string displayName;
    KeyCode defaultKey{KeyCode::None};
    GamepadButton defaultButton{GamepadButton::None};
    ActionHandler handler;
};

class ActionRegistry {
public:
    Result<void> RegisterAction(const ActionDefinition& def);
    bool HasAction(std::string_view contextId, std::string_view actionId) const;
    const ActionDefinition* GetAction(std::string_view contextId, std::string_view actionId) const;
    std::vector<const ActionDefinition*> GetActionsForContext(std::string_view contextId) const;
    std::vector<std::string> GetContextIds() const;
    size_t ActionCount() const;

    // Apply all registered actions to an InputContext
    void PopulateContext(InputContext& context, std::string_view contextId) const;

private:
    // key = "contextId.actionId"
    std::unordered_map<std::string, ActionDefinition> m_actions;
};

}  // namespace ohui::input
