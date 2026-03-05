#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ohui::mcm {

using ConditionValue = std::variant<bool, int64_t, double, std::string>;

class IConditionStateProvider {
public:
    virtual ~IConditionStateProvider() = default;
    virtual std::string ProviderPrefix() const = 0;
    virtual std::optional<ConditionValue> Resolve(std::string_view key) const = 0;
};

enum class ConditionOp { Eq, Neq, Gt, Lt, Gte, Lte };
enum class LogicalOp { And, Or };

struct ConditionLiteral { ConditionValue value; };
struct ConditionControlRef { std::string controlId; };
struct ConditionProviderCall {
    std::string providerPrefix;
    std::string key;
    std::string argument;
};

struct ConditionNot;
struct ConditionComparison;
struct ConditionLogical;

using ConditionNode = std::variant<
    ConditionLiteral, ConditionControlRef, ConditionProviderCall,
    std::unique_ptr<ConditionNot>,
    std::unique_ptr<ConditionComparison>,
    std::unique_ptr<ConditionLogical>>;

struct ConditionNot { ConditionNode operand; };
struct ConditionComparison { ConditionNode left; ConditionOp op; ConditionNode right; };
struct ConditionLogical { ConditionNode left; LogicalOp op; ConditionNode right; };

struct CompiledCondition {
    ConditionNode root;
    std::vector<std::string> referencedControlIds;
    std::vector<std::string> referencedProviderKeys;
    bool hasCycle{false};
};

enum class ControlVisibility { Visible, Hidden };

}  // namespace ohui::mcm
