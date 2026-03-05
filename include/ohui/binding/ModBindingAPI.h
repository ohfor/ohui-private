#pragma once

#include "ohui/binding/BindingTypes.h"
#include "ohui/binding/DataBindingEngine.h"
#include "ohui/core/RegistrationTypes.h"
#include "ohui/core/Result.h"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::binding {

enum class PollRateCategory;  // forward declared from BindingSchema.h

struct ModBindingDefinition {
    std::string modId;
    std::string id;
    BindingType type;
    std::string description;
    PollFunction pollFn;
};

class ModBindingAPI {
public:
    explicit ModBindingAPI(DataBindingEngine& engine);

    void OpenRegistrationWindow();
    void LockRegistrationWindow();
    RegistrationWindow GetRegistrationWindowState() const;

    Result<void> RegisterBinding(const ModBindingDefinition& def);
    Result<void> UnregisterBinding(std::string_view bindingId);
    Result<size_t> RegisterBatch(std::span<const ModBindingDefinition> defs);

    bool IsModBinding(std::string_view bindingId) const;
    std::string_view GetOwnerMod(std::string_view bindingId) const;
    std::vector<std::string> GetBindingsByMod(std::string_view modId) const;
    size_t ModBindingCount() const;

private:
    DataBindingEngine& m_engine;
    RegistrationWindow m_windowState{RegistrationWindow::Closed};
    struct ModBindingEntry { std::string modId; };
    std::unordered_map<std::string, ModBindingEntry> m_modBindings;
};

}  // namespace ohui::binding
