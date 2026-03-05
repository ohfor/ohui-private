#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace ohui::binding {

enum class BindingType { Float, Int, Bool, String };

using BindingValue = std::variant<float, int64_t, bool, std::string>;

enum class UpdateMode { Reactive, ThrottledReactive, FrameDriven };

struct BindingDefinition {
    std::string id;
    BindingType type;
    std::string description;
};

using PollFunction = std::function<BindingValue()>;

struct SubscriptionOptions {
    UpdateMode mode{UpdateMode::Reactive};
    float maxUpdatesPerSecond{0.0f};  // ThrottledReactive only
};

}  // namespace ohui::binding
