#pragma once

#include "ohui/core/Result.h"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ohui::dsl {

struct IconRect {
    float u0{0}, v0{0}, u1{1}, v1{1};
};

struct IconEntry {
    std::string id;
    std::string atlasPath;
    IconRect uv;
};

class IconAtlas {
public:
    Result<void> LoadAtlas(std::string_view atlasId, std::span<const IconEntry> entries);
    Result<void> LoadSkinAtlas(std::string_view atlasId, std::span<const IconEntry> entries);
    void ClearSkinAtlas(std::string_view atlasId);
    void ClearAllSkinAtlases();

    const IconEntry* Resolve(std::string_view iconId) const;
    bool HasIcon(std::string_view iconId) const;
    size_t IconCount() const;

private:
    std::unordered_map<std::string, IconEntry> m_icons;
    std::unordered_map<std::string, IconEntry> m_skinIcons;
    // Track which icons came from which atlas for clearing
    std::unordered_map<std::string, std::vector<std::string>> m_skinAtlasIds;
};

}  // namespace ohui::dsl
