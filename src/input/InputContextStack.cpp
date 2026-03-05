#include "ohui/input/InputContextStack.h"
#include "ohui/core/Log.h"
#include <algorithm>

namespace ohui::input {

// -- InputContext --

InputContext::InputContext(std::string id, std::string displayName)
    : m_id(std::move(id))
    , m_displayName(std::move(displayName)) {}

const std::string& InputContext::GetId() const { return m_id; }
const std::string& InputContext::GetDisplayName() const { return m_displayName; }

void InputContext::RegisterAction(const ActionBinding& binding) {
    m_keyBindings[binding.actionId] = binding.defaultKey;
    m_buttonBindings[binding.actionId] = binding.defaultButton;
    m_actions[binding.actionId] = binding;
}

bool InputContext::HasAction(std::string_view actionId) const {
    return m_actions.contains(std::string(actionId));
}

const ActionBinding* InputContext::GetAction(std::string_view actionId) const {
    auto it = m_actions.find(std::string(actionId));
    return it != m_actions.end() ? &it->second : nullptr;
}

std::vector<const ActionBinding*> InputContext::GetAllActions() const {
    std::vector<const ActionBinding*> result;
    result.reserve(m_actions.size());
    for (const auto& [id, action] : m_actions) {
        result.push_back(&action);
    }
    return result;
}

bool InputContext::HandleInput(const InputEvent& event) const {
    if (event.IsKeyboard()) {
        for (const auto& [actionId, key] : m_keyBindings) {
            if (key == event.key) {
                auto it = m_actions.find(actionId);
                if (it != m_actions.end() && it->second.handler) {
                    it->second.handler(event);
                    return true;
                }
            }
        }
    }
    if (event.IsGamepad()) {
        for (const auto& [actionId, button] : m_buttonBindings) {
            if (button == event.button) {
                auto it = m_actions.find(actionId);
                if (it != m_actions.end() && it->second.handler) {
                    it->second.handler(event);
                    return true;
                }
            }
        }
    }
    return false;
}

void InputContext::SetKeyBinding(std::string_view actionId, KeyCode key) {
    std::string id(actionId);
    if (m_actions.contains(id)) {
        m_keyBindings[id] = key;
    }
}

void InputContext::SetButtonBinding(std::string_view actionId, GamepadButton button) {
    std::string id(actionId);
    if (m_actions.contains(id)) {
        m_buttonBindings[id] = button;
    }
}

KeyCode InputContext::GetKeyBinding(std::string_view actionId) const {
    auto it = m_keyBindings.find(std::string(actionId));
    return it != m_keyBindings.end() ? it->second : KeyCode::None;
}

GamepadButton InputContext::GetButtonBinding(std::string_view actionId) const {
    auto it = m_buttonBindings.find(std::string(actionId));
    return it != m_buttonBindings.end() ? it->second : GamepadButton::None;
}

void InputContext::ResetToDefaults() {
    for (const auto& [actionId, action] : m_actions) {
        m_keyBindings[actionId] = action.defaultKey;
        m_buttonBindings[actionId] = action.defaultButton;
    }
}

size_t InputContext::ActionCount() const { return m_actions.size(); }

// -- InputContextStack --

InputContextStack::InputContextStack() {
    m_gameplayContext = std::make_shared<InputContext>(kGameplayContextId, "Gameplay");
    m_stack.push_back(m_gameplayContext);
}

Result<void> InputContextStack::Push(std::shared_ptr<InputContext> context) {
    if (!context) {
        return std::unexpected(Error{ErrorCode::ContextNotFound, "Cannot push null context"});
    }
    if (HasContext(context->GetId())) {
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Context already on stack: " + context->GetId()});
    }
    m_stack.push_back(std::move(context));
    return {};
}

Result<void> InputContextStack::Pop(std::string_view contextId) {
    if (contextId == kGameplayContextId) {
        return std::unexpected(Error{ErrorCode::ContextNotFound,
            "Cannot pop the gameplay context"});
    }
    auto it = std::find_if(m_stack.begin(), m_stack.end(),
        [&](const auto& ctx) { return ctx->GetId() == contextId; });
    if (it == m_stack.end()) {
        return std::unexpected(Error{ErrorCode::ContextNotFound,
            "Context not on stack: " + std::string(contextId)});
    }
    if (it != m_stack.end() - 1) {
        ohui::log::debug("Out-of-order pop of context '{}' (not top of stack)", contextId);
    }
    m_stack.erase(it);
    return {};
}

InputContext* InputContextStack::GetActiveContext() {
    return m_stack.back().get();
}

const InputContext* InputContextStack::GetActiveContext() const {
    return m_stack.back().get();
}

bool InputContextStack::HandleInput(const InputEvent& event) {
    return m_stack.back()->HandleInput(event);
}

size_t InputContextStack::Depth() const { return m_stack.size(); }

bool InputContextStack::HasContext(std::string_view contextId) const {
    return std::any_of(m_stack.begin(), m_stack.end(),
        [&](const auto& ctx) { return ctx->GetId() == contextId; });
}

const InputContext* InputContextStack::FindContext(std::string_view contextId) const {
    auto it = std::find_if(m_stack.begin(), m_stack.end(),
        [&](const auto& ctx) { return ctx->GetId() == contextId; });
    return it != m_stack.end() ? it->get() : nullptr;
}

std::vector<std::string> InputContextStack::GetStackOrder() const {
    std::vector<std::string> order;
    order.reserve(m_stack.size());
    for (const auto& ctx : m_stack) {
        order.push_back(ctx->GetId());
    }
    return order;
}

InputContext& InputContextStack::GetGameplayContext() {
    return *m_gameplayContext;
}

}  // namespace ohui::input
