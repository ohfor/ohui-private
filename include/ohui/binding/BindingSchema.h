#pragma once

#include "ohui/binding/BindingTypes.h"
#include "ohui/binding/DataBindingEngine.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <vector>

namespace ohui::binding {

enum class PollRateCategory { PerFrame, ThrottledFast, ThrottledSlow, OnEvent };

struct SchemaEntry {
    std::string id;
    BindingType type;
    std::string description;
    PollRateCategory pollRate;
};

class BindingSchema {
public:
    static const std::vector<SchemaEntry>& GetBuiltinBindings();
    static Result<void> RegisterBuiltinBindings(DataBindingEngine& engine);
    static Result<void> RegisterCustomBinding(
        DataBindingEngine& engine, std::string_view namespacePrefix,
        std::string_view id, BindingType type,
        std::string_view description, PollFunction pollFn);
    static size_t BuiltinBindingCount();
};

}  // namespace ohui::binding
