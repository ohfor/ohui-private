#include "ohui/dsl/IconAtlas.h"

namespace ohui::dsl {

Result<void> IconAtlas::LoadAtlas(std::string_view /*atlasId*/, std::span<const IconEntry> entries) {
    for (const auto& entry : entries) {
        m_icons[entry.id] = entry;
    }
    return {};
}

Result<void> IconAtlas::LoadSkinAtlas(std::string_view atlasId, std::span<const IconEntry> entries) {
    auto& ids = m_skinAtlasIds[std::string(atlasId)];
    for (const auto& entry : entries) {
        m_skinIcons[entry.id] = entry;
        ids.push_back(entry.id);
    }
    return {};
}

void IconAtlas::ClearSkinAtlas(std::string_view atlasId) {
    auto it = m_skinAtlasIds.find(std::string(atlasId));
    if (it != m_skinAtlasIds.end()) {
        for (const auto& id : it->second) {
            m_skinIcons.erase(id);
        }
        m_skinAtlasIds.erase(it);
    }
}

void IconAtlas::ClearAllSkinAtlases() {
    m_skinIcons.clear();
    m_skinAtlasIds.clear();
}

const IconEntry* IconAtlas::Resolve(std::string_view iconId) const {
    // Skin layer takes priority
    auto sit = m_skinIcons.find(std::string(iconId));
    if (sit != m_skinIcons.end()) return &sit->second;

    auto it = m_icons.find(std::string(iconId));
    if (it != m_icons.end()) return &it->second;

    return nullptr;
}

bool IconAtlas::HasIcon(std::string_view iconId) const {
    return m_skinIcons.contains(std::string(iconId)) ||
           m_icons.contains(std::string(iconId));
}

size_t IconAtlas::IconCount() const {
    // Count unique icons across both layers
    size_t count = m_icons.size();
    for (const auto& [id, _] : m_skinIcons) {
        if (!m_icons.contains(id)) ++count;
    }
    return count;
}

}  // namespace ohui::dsl
