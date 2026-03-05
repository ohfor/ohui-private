#pragma once

#include "ohui/binding/BindingTypes.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::binding {

class DataBindingEngine {
public:
    Result<void> RegisterBinding(const BindingDefinition& def, PollFunction pollFn);
    bool HasBinding(std::string_view bindingId) const;
    const BindingDefinition* GetBindingDefinition(std::string_view bindingId) const;
    std::vector<std::string> GetAllBindingIds() const;

    Result<void> Subscribe(std::string_view widgetId, std::string_view bindingId,
                           const SubscriptionOptions& opts = {});
    Result<void> Unsubscribe(std::string_view widgetId, std::string_view bindingId);
    Result<void> UnsubscribeAll(std::string_view widgetId);

    std::vector<std::string> Update(float deltaTime);

    const BindingValue* GetCurrentValue(std::string_view bindingId) const;
    size_t BindingCount() const;
    size_t SubscriptionCount() const;

private:
    struct BindingEntry {
        BindingDefinition definition;
        PollFunction pollFn;
        BindingValue currentValue;
        BindingValue previousValue;
        bool hasBeenPolled{false};
    };
    struct Subscription {
        std::string widgetId;
        std::string bindingId;
        SubscriptionOptions options;
        float timeSinceLastNotify{0.0f};
    };
    std::unordered_map<std::string, BindingEntry> m_bindings;
    std::vector<Subscription> m_subscriptions;
};

}  // namespace ohui::binding
