#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace ohui::dsl {

class ComponentStateStore {
public:
    using StateValue = std::variant<float, int64_t, bool, std::string>;

    template<typename T>
    void Set(std::string_view instanceId, std::string_view key, T value) {
        m_state[std::string(instanceId)][std::string(key)] = StateValue(std::move(value));
    }

    template<typename T>
    std::optional<T> Get(std::string_view instanceId, std::string_view key) const {
        auto iit = m_state.find(std::string(instanceId));
        if (iit == m_state.end()) return std::nullopt;
        auto kit = iit->second.find(std::string(key));
        if (kit == iit->second.end()) return std::nullopt;
        if (auto* p = std::get_if<T>(&kit->second)) return *p;
        return std::nullopt;
    }

    void ClearInstance(std::string_view instanceId) {
        m_state.erase(std::string(instanceId));
    }

    void ClearAll() {
        m_state.clear();
    }

private:
    std::unordered_map<std::string,
        std::unordered_map<std::string, StateValue>> m_state;
};

}  // namespace ohui::dsl
