#include "ohui/binding/ModBindingAPI.h"
#include "ohui/core/Log.h"

namespace ohui::binding {

ModBindingAPI::ModBindingAPI(DataBindingEngine& engine)
    : m_engine(engine) {}

void ModBindingAPI::OpenRegistrationWindow() {
    m_windowState = RegistrationWindow::Open;
}

void ModBindingAPI::LockRegistrationWindow() {
    m_windowState = RegistrationWindow::Locked;
}

RegistrationWindow ModBindingAPI::GetRegistrationWindowState() const {
    return m_windowState;
}

Result<void> ModBindingAPI::RegisterBinding(const ModBindingDefinition& def) {
    if (m_windowState != RegistrationWindow::Open) {
        return std::unexpected(Error{ErrorCode::RegistrationWindowClosed,
            "Registration window is not open"});
    }

    if (def.modId.empty()) {
        return std::unexpected(Error{ErrorCode::InvalidModId,
            "Mod ID cannot be empty"});
    }

    // Namespace validation: binding ID must start with modId + "."
    std::string expectedPrefix = def.modId + ".";
    if (!def.id.starts_with(expectedPrefix)) {
        return std::unexpected(Error{ErrorCode::InvalidNamespace,
            "Binding ID '" + def.id + "' must start with '" + expectedPrefix + "'"});
    }

    if (!def.pollFn) {
        return std::unexpected(Error{ErrorCode::InvalidBinding,
            "Null poll function for binding: " + def.id});
    }

    // Supersede semantics: if binding already exists, unregister the old one
    if (m_modBindings.contains(def.id)) {
        ohui::log::debug("ModBindingAPI: superseding binding '{}' from mod '{}'",
                         def.id, def.modId);
        (void)m_engine.UnregisterBinding(def.id);
        m_modBindings.erase(def.id);
    }

    BindingDefinition engineDef;
    engineDef.id = def.id;
    engineDef.type = def.type;
    engineDef.description = def.description;

    auto result = m_engine.RegisterBinding(engineDef, def.pollFn);
    if (!result.has_value()) {
        return result;
    }

    m_modBindings[def.id] = ModBindingEntry{def.modId};
    return {};
}

Result<void> ModBindingAPI::UnregisterBinding(std::string_view bindingId) {
    auto it = m_modBindings.find(std::string(bindingId));
    if (it == m_modBindings.end()) {
        return std::unexpected(Error{ErrorCode::BindingNotFound,
            "Mod binding not found: " + std::string(bindingId)});
    }

    (void)m_engine.UnregisterBinding(bindingId);
    m_modBindings.erase(it);
    return {};
}

Result<size_t> ModBindingAPI::RegisterBatch(std::span<const ModBindingDefinition> defs) {
    size_t count = 0;
    for (const auto& def : defs) {
        auto result = RegisterBinding(def);
        if (!result.has_value()) {
            if (count == 0) {
                return std::unexpected(result.error());
            }
            return count;
        }
        ++count;
    }
    return count;
}

bool ModBindingAPI::IsModBinding(std::string_view bindingId) const {
    return m_modBindings.contains(std::string(bindingId));
}

std::string_view ModBindingAPI::GetOwnerMod(std::string_view bindingId) const {
    auto it = m_modBindings.find(std::string(bindingId));
    if (it == m_modBindings.end()) return {};
    return it->second.modId;
}

std::vector<std::string> ModBindingAPI::GetBindingsByMod(std::string_view modId) const {
    std::vector<std::string> result;
    for (const auto& [id, entry] : m_modBindings) {
        if (entry.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

size_t ModBindingAPI::ModBindingCount() const {
    return m_modBindings.size();
}

}  // namespace ohui::binding
