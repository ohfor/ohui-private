#include "ohui/binding/DataBindingEngine.h"
#include "ohui/core/Log.h"

#include <algorithm>
#include <unordered_set>

namespace ohui::binding {

Result<void> DataBindingEngine::RegisterBinding(const BindingDefinition& def, PollFunction pollFn) {
    if (m_bindings.contains(def.id)) {
        ohui::log::debug("DataBindingEngine: duplicate binding '{}'", def.id);
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Binding already registered: " + def.id});
    }

    if (!pollFn) {
        return std::unexpected(Error{ErrorCode::InvalidBinding,
            "Null poll function for binding: " + def.id});
    }

    BindingEntry entry;
    entry.definition = def;
    entry.pollFn = std::move(pollFn);

    // Initialize default value based on type
    switch (def.type) {
        case BindingType::Float:  entry.currentValue = 0.0f; break;
        case BindingType::Int:    entry.currentValue = int64_t{0}; break;
        case BindingType::Bool:   entry.currentValue = false; break;
        case BindingType::String: entry.currentValue = std::string{}; break;
    }
    entry.previousValue = entry.currentValue;

    m_bindings[def.id] = std::move(entry);
    return {};
}

Result<void> DataBindingEngine::UnregisterBinding(std::string_view bindingId) {
    auto it = m_bindings.find(std::string(bindingId));
    if (it == m_bindings.end()) {
        return std::unexpected(Error{ErrorCode::BindingNotFound,
            "Binding not found: " + std::string(bindingId)});
    }
    auto subIt = std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
        [&](const Subscription& s) { return s.bindingId == bindingId; });
    m_subscriptions.erase(subIt, m_subscriptions.end());
    m_bindings.erase(it);
    return {};
}

bool DataBindingEngine::HasBinding(std::string_view bindingId) const {
    return m_bindings.contains(std::string(bindingId));
}

const BindingDefinition* DataBindingEngine::GetBindingDefinition(std::string_view bindingId) const {
    auto it = m_bindings.find(std::string(bindingId));
    if (it == m_bindings.end()) return nullptr;
    return &it->second.definition;
}

std::vector<std::string> DataBindingEngine::GetAllBindingIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_bindings.size());
    for (const auto& [id, entry] : m_bindings) {
        ids.push_back(id);
    }
    return ids;
}

Result<void> DataBindingEngine::Subscribe(std::string_view widgetId, std::string_view bindingId,
                                          const SubscriptionOptions& opts) {
    if (!m_bindings.contains(std::string(bindingId))) {
        return std::unexpected(Error{ErrorCode::BindingNotFound,
            "Binding not found: " + std::string(bindingId)});
    }

    Subscription sub;
    sub.widgetId = std::string(widgetId);
    sub.bindingId = std::string(bindingId);
    sub.options = opts;
    m_subscriptions.push_back(std::move(sub));
    return {};
}

Result<void> DataBindingEngine::Unsubscribe(std::string_view widgetId, std::string_view bindingId) {
    auto it = std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
        [&](const Subscription& s) {
            return s.widgetId == widgetId && s.bindingId == bindingId;
        });
    m_subscriptions.erase(it, m_subscriptions.end());
    return {};
}

Result<void> DataBindingEngine::UnsubscribeAll(std::string_view widgetId) {
    auto it = std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
        [&](const Subscription& s) { return s.widgetId == widgetId; });
    m_subscriptions.erase(it, m_subscriptions.end());
    return {};
}

std::vector<std::string> DataBindingEngine::Update(float deltaTime) {
    // Determine which bindings have active subscriptions
    std::unordered_set<std::string> subscribedBindings;
    for (const auto& sub : m_subscriptions) {
        subscribedBindings.insert(sub.bindingId);
    }

    // Poll only subscribed bindings
    for (auto& [id, entry] : m_bindings) {
        if (!subscribedBindings.contains(id)) continue;
        entry.previousValue = entry.currentValue;
        entry.currentValue = entry.pollFn();
        entry.hasBeenPolled = true;
    }

    // Evaluate subscriptions
    std::unordered_set<std::string> dirtyWidgets;
    for (auto& sub : m_subscriptions) {
        auto& entry = m_bindings.at(sub.bindingId);
        bool dirty = false;

        switch (sub.options.mode) {
            case UpdateMode::Reactive:
                dirty = (entry.currentValue != entry.previousValue);
                break;

            case UpdateMode::ThrottledReactive:
                sub.timeSinceLastNotify += deltaTime;
                if (entry.currentValue != entry.previousValue) {
                    float interval = (sub.options.maxUpdatesPerSecond > 0.0f)
                        ? (1.0f / sub.options.maxUpdatesPerSecond)
                        : 0.0f;
                    if (sub.timeSinceLastNotify >= interval) {
                        dirty = true;
                        sub.timeSinceLastNotify = 0.0f;
                    }
                }
                break;

            case UpdateMode::FrameDriven:
                dirty = true;
                break;
        }

        if (dirty) {
            dirtyWidgets.insert(sub.widgetId);
        }
    }

    return {dirtyWidgets.begin(), dirtyWidgets.end()};
}

const BindingValue* DataBindingEngine::GetCurrentValue(std::string_view bindingId) const {
    auto it = m_bindings.find(std::string(bindingId));
    if (it == m_bindings.end()) return nullptr;
    return &it->second.currentValue;
}

size_t DataBindingEngine::BindingCount() const {
    return m_bindings.size();
}

size_t DataBindingEngine::SubscriptionCount() const {
    return m_subscriptions.size();
}

}  // namespace ohui::binding
