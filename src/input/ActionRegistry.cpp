#include "ohui/input/ActionRegistry.h"
#include <algorithm>
#include <unordered_set>

namespace ohui::input {

static std::string MakeKey(std::string_view contextId, std::string_view actionId) {
    std::string key;
    key.reserve(contextId.size() + 1 + actionId.size());
    key.append(contextId);
    key.push_back('.');
    key.append(actionId);
    return key;
}

Result<void> ActionRegistry::RegisterAction(const ActionDefinition& def) {
    auto key = MakeKey(def.contextId, def.actionId);
    if (m_actions.contains(key)) {
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Action already registered: " + key});
    }
    m_actions[key] = def;
    return {};
}

bool ActionRegistry::HasAction(std::string_view contextId, std::string_view actionId) const {
    return m_actions.contains(MakeKey(contextId, actionId));
}

const ActionDefinition* ActionRegistry::GetAction(std::string_view contextId,
                                                   std::string_view actionId) const {
    auto it = m_actions.find(MakeKey(contextId, actionId));
    return it != m_actions.end() ? &it->second : nullptr;
}

std::vector<const ActionDefinition*> ActionRegistry::GetActionsForContext(
    std::string_view contextId) const {
    std::vector<const ActionDefinition*> result;
    for (const auto& [key, def] : m_actions) {
        if (def.contextId == contextId) {
            result.push_back(&def);
        }
    }
    return result;
}

std::vector<std::string> ActionRegistry::GetContextIds() const {
    std::unordered_set<std::string> ids;
    for (const auto& [key, def] : m_actions) {
        ids.insert(def.contextId);
    }
    return {ids.begin(), ids.end()};
}

size_t ActionRegistry::ActionCount() const { return m_actions.size(); }

void ActionRegistry::PopulateContext(InputContext& context, std::string_view contextId) const {
    for (const auto& [key, def] : m_actions) {
        if (def.contextId == contextId) {
            ActionBinding binding;
            binding.actionId = def.actionId;
            binding.displayName = def.displayName;
            binding.defaultKey = def.defaultKey;
            binding.defaultButton = def.defaultButton;
            binding.handler = def.handler;
            context.RegisterAction(binding);
        }
    }
}

}  // namespace ohui::input
