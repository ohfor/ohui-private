#include "ohui/widget/MultiSelectState.h"

namespace ohui::widget {

void MultiSelectState::Add(std::string_view id) {
    m_selected.emplace(id);
}

void MultiSelectState::Remove(std::string_view id) {
    m_selected.erase(std::string(id));
}

void MultiSelectState::Toggle(std::string_view id) {
    auto key = std::string(id);
    if (m_selected.contains(key)) {
        m_selected.erase(key);
    } else {
        m_selected.insert(key);
    }
}

void MultiSelectState::Clear() {
    m_selected.clear();
}

bool MultiSelectState::IsSelected(std::string_view id) const {
    return m_selected.contains(std::string(id));
}

std::vector<std::string> MultiSelectState::GetSelected() const {
    return {m_selected.begin(), m_selected.end()};
}

size_t MultiSelectState::Count() const {
    return m_selected.size();
}

bool MultiSelectState::IsEmpty() const {
    return m_selected.empty();
}

}  // namespace ohui::widget
