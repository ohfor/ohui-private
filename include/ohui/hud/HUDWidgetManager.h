#pragma once

#include "ohui/dsl/DSLRuntimeEngine.h"
#include "ohui/dsl/WidgetAST.h"
#include "ohui/widget/WidgetRegistry.h"
#include "ohui/binding/DataBindingEngine.h"
#include "ohui/core/Result.h"

#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::hud {

class HUDWidgetManager {
public:
    using VisibilityPredicate = std::function<bool(const binding::DataBindingEngine&)>;

    HUDWidgetManager(dsl::DSLRuntimeEngine& dslEngine,
                     widget::WidgetRegistry& widgetRegistry,
                     binding::DataBindingEngine& bindings);

    Result<void> LoadDefaults();
    void UpdateVisibility();

    Result<dsl::DrawCallList> Evaluate(std::string_view widgetId,
                                       float screenWidth, float screenHeight,
                                       float deltaTime);

    std::vector<std::string> GetWidgetIds() const;
    size_t WidgetCount() const;
    bool HasWidget(std::string_view id) const;

private:
    struct HUDEntry {
        std::string id;
        std::string_view dslSource;
        widget::WidgetManifest manifest;
        VisibilityPredicate predicate;
    };

    static const std::vector<HUDEntry>& GetDefaultEntries();

    dsl::DSLRuntimeEngine& m_dslEngine;
    widget::WidgetRegistry& m_widgetRegistry;
    binding::DataBindingEngine& m_bindings;
    std::vector<std::string> m_loadedIds;
    std::deque<dsl::ParseResult> m_parsedDefs;  // owns AST lifetime, stable pointers
    std::unordered_map<std::string, VisibilityPredicate> m_predicates;
};

}  // namespace ohui::hud
