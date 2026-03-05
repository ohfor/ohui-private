#pragma once

#include "ohui/mcm/MCM2ConditionTypes.h"
#include "ohui/mcm/MCM2Types.h"
#include "ohui/core/Result.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ohui::mcm {

class MCM2ConditionEngine {
public:
    Result<void> CompileConditions(const MCM2Definition& definition);

    void RegisterStateProvider(std::shared_ptr<IConditionStateProvider> provider);
    void UnregisterStateProvider(std::string_view prefix);

    void SetControlValue(std::string_view controlId, const ConditionValue& value);
    std::optional<ConditionValue> GetControlValue(std::string_view controlId) const;

    ControlVisibility EvaluateVisibility(std::string_view controlId) const;
    std::vector<std::string> GetAffectedControls(std::string_view changedControlId) const;
    std::vector<std::string> ReEvaluateAll() const;

    bool HasCondition(std::string_view controlId) const;
    bool HasCycle() const;
    std::vector<std::string> GetCyclicControls() const;

    Result<ConditionValue> EvaluateExpression(std::string_view expr) const;
    static Result<CompiledCondition> ParseCondition(std::string_view expr);

private:
    std::unordered_map<std::string, CompiledCondition> m_conditions;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_dependencyGraph;
    std::unordered_map<std::string, ConditionValue> m_controlValues;
    mutable std::unordered_map<std::string, ControlVisibility> m_visibilityCache;
    mutable bool m_cacheValid{false};
    std::unordered_map<std::string, std::shared_ptr<IConditionStateProvider>> m_providers;

    void BuildDependencyGraph();
    bool DetectCycles() const;
    ConditionValue Evaluate(const ConditionNode& node) const;
    static bool ToBool(const ConditionValue& val);
    static int CompareValues(const ConditionValue& a, const ConditionValue& b);
};

}  // namespace ohui::mcm
