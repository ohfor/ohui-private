#include "ohui/dsl/ComponentHandler.h"

#include <algorithm>

namespace ohui::dsl {

void ComponentRegistry::Register(std::string_view typeName, std::unique_ptr<IComponentHandler> handler) {
    m_handlers[std::string(typeName)] = std::move(handler);
}

IComponentHandler* ComponentRegistry::Get(std::string_view typeName) const {
    auto it = m_handlers.find(std::string(typeName));
    if (it != m_handlers.end()) return it->second.get();
    return nullptr;
}

bool ComponentRegistry::Has(std::string_view typeName) const {
    return m_handlers.contains(std::string(typeName));
}

size_t ComponentRegistry::Count() const {
    return m_handlers.size();
}

std::vector<std::string> ComponentRegistry::GetTypeNames() const {
    std::vector<std::string> names;
    names.reserve(m_handlers.size());
    for (const auto& [name, _] : m_handlers) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace ohui::dsl
