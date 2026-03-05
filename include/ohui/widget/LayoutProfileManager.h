#pragma once

#include "ohui/widget/LayoutProfile.h"
#include "ohui/widget/WidgetRegistry.h"
#include "ohui/core/Result.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::widget {

class LayoutProfileManager {
public:
    explicit LayoutProfileManager(WidgetRegistry& registry);

    Result<void> Save(std::string_view name);
    Result<void> Load(std::string_view name);
    Result<void> Delete(std::string_view name);
    std::vector<std::string> List() const;
    std::string GetActiveProfileName() const;
    bool HasProfile(std::string_view name) const;

    void EnsureBuiltinProfiles();

    std::vector<uint8_t> Serialize() const;
    Result<void> Deserialize(std::span<const uint8_t> data);

    const LayoutProfile* GetProfile(std::string_view name) const;

    static constexpr const char* kDefaultProfile = "Default";
    static constexpr const char* kMinimalProfile = "Minimal";
    static constexpr const char* kControllerProfile = "Controller";

private:
    WidgetRegistry& m_registry;
    std::unordered_map<std::string, LayoutProfile> m_profiles;
    std::string m_activeProfileName;

    LayoutProfile GenerateDefaultProfile() const;
    LayoutProfile GenerateMinimalProfile() const;
    LayoutProfile GenerateControllerProfile() const;
};

}  // namespace ohui::widget
