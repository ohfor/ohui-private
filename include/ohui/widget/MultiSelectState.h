#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ohui::widget {

class MultiSelectState {
public:
    void Add(std::string_view id);
    void Remove(std::string_view id);
    void Toggle(std::string_view id);
    void Clear();
    bool IsSelected(std::string_view id) const;
    std::vector<std::string> GetSelected() const;
    size_t Count() const;
    bool IsEmpty() const;

private:
    std::unordered_set<std::string> m_selected;
};

}  // namespace ohui::widget
