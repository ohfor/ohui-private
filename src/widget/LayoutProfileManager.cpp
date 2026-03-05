#include "ohui/widget/LayoutProfileManager.h"
#include "ohui/core/Log.h"

#include <cstring>

namespace ohui::widget {

// --- Binary serialization helpers (LE, same as CosaveManager) ---

static void WriteU8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

static void WriteU16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void WriteF32(std::vector<uint8_t>& buf, float val) {
    uint8_t bytes[4];
    std::memcpy(bytes, &val, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& str) {
    WriteU16(buf, static_cast<uint16_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

static uint16_t ReadU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static float ReadF32(const uint8_t* p) {
    float val;
    std::memcpy(&val, p, 4);
    return val;
}

static std::string ReadString(const uint8_t* p, size_t& offset, size_t totalSize) {
    if (offset + 2 > totalSize) return {};
    uint16_t len = ReadU16(p + offset);
    offset += 2;
    if (offset + len > totalSize) return {};
    std::string result(reinterpret_cast<const char*>(p + offset), len);
    offset += len;
    return result;
}

// --- LayoutProfileManager ---

LayoutProfileManager::LayoutProfileManager(WidgetRegistry& registry)
    : m_registry(registry) {}

Result<void> LayoutProfileManager::Save(std::string_view name) {
    LayoutProfile profile;
    profile.name = std::string(name);

    auto allWidgets = m_registry.GetAllWidgets();
    for (const auto* state : allWidgets) {
        WidgetLayoutEntry entry;
        entry.position = state->position;
        entry.size = state->size;
        entry.visible = state->visible;
        profile.widgets[state->id] = entry;
    }

    m_profiles[std::string(name)] = std::move(profile);
    m_activeProfileName = std::string(name);
    return {};
}

Result<void> LayoutProfileManager::Load(std::string_view name) {
    auto it = m_profiles.find(std::string(name));
    if (it == m_profiles.end()) {
        return std::unexpected(Error{ErrorCode::ProfileNotFound,
            "Profile not found: " + std::string(name)});
    }

    const auto& profile = it->second;
    for (const auto& [widgetId, entry] : profile.widgets) {
        if (m_registry.HasWidget(widgetId)) {
            m_registry.Move(widgetId, entry.position);
            m_registry.Resize(widgetId, entry.size);
            m_registry.SetVisible(widgetId, entry.visible);
        }
        // Entries for unregistered widgets are preserved in the profile
    }

    m_activeProfileName = std::string(name);
    return {};
}

Result<void> LayoutProfileManager::Delete(std::string_view name) {
    auto it = m_profiles.find(std::string(name));
    if (it == m_profiles.end()) {
        return std::unexpected(Error{ErrorCode::ProfileNotFound,
            "Profile not found: " + std::string(name)});
    }
    m_profiles.erase(it);
    if (m_activeProfileName == name) {
        m_activeProfileName.clear();
    }
    return {};
}

std::vector<std::string> LayoutProfileManager::List() const {
    std::vector<std::string> names;
    names.reserve(m_profiles.size());
    for (const auto& [name, profile] : m_profiles) {
        names.push_back(name);
    }
    return names;
}

std::string LayoutProfileManager::GetActiveProfileName() const {
    return m_activeProfileName;
}

bool LayoutProfileManager::HasProfile(std::string_view name) const {
    return m_profiles.contains(std::string(name));
}

void LayoutProfileManager::EnsureBuiltinProfiles() {
    if (!m_profiles.contains(kDefaultProfile)) {
        m_profiles[kDefaultProfile] = GenerateDefaultProfile();
    }
    if (!m_profiles.contains(kMinimalProfile)) {
        m_profiles[kMinimalProfile] = GenerateMinimalProfile();
    }
    if (!m_profiles.contains(kControllerProfile)) {
        m_profiles[kControllerProfile] = GenerateControllerProfile();
    }
}

std::vector<uint8_t> LayoutProfileManager::Serialize() const {
    std::vector<uint8_t> buf;

    // [profile_count:u16][active_name:string]
    WriteU16(buf, static_cast<uint16_t>(m_profiles.size()));
    WriteString(buf, m_activeProfileName);

    // Per profile: [name:string][widget_count:u16]
    // Per widget: [id:string][x:f32][y:f32][w:f32][h:f32][visible:u8]
    for (const auto& [name, profile] : m_profiles) {
        WriteString(buf, profile.name);
        WriteU16(buf, static_cast<uint16_t>(profile.widgets.size()));
        for (const auto& [widgetId, entry] : profile.widgets) {
            WriteString(buf, widgetId);
            WriteF32(buf, entry.position.x);
            WriteF32(buf, entry.position.y);
            WriteF32(buf, entry.size.x);
            WriteF32(buf, entry.size.y);
            WriteU8(buf, entry.visible ? 1 : 0);
        }
    }

    return buf;
}

Result<void> LayoutProfileManager::Deserialize(std::span<const uint8_t> data) {
    m_profiles.clear();
    m_activeProfileName.clear();

    if (data.empty()) return {};

    const uint8_t* p = data.data();
    size_t totalSize = data.size();
    size_t offset = 0;

    if (offset + 2 > totalSize) {
        return std::unexpected(Error{ErrorCode::InvalidFormat, "Truncated profile data"});
    }

    uint16_t profileCount = ReadU16(p + offset);
    offset += 2;

    m_activeProfileName = ReadString(p, offset, totalSize);

    for (uint16_t i = 0; i < profileCount; ++i) {
        LayoutProfile profile;
        profile.name = ReadString(p, offset, totalSize);

        if (offset + 2 > totalSize) {
            return std::unexpected(Error{ErrorCode::InvalidFormat, "Truncated widget count"});
        }
        uint16_t widgetCount = ReadU16(p + offset);
        offset += 2;

        for (uint16_t j = 0; j < widgetCount; ++j) {
            std::string widgetId = ReadString(p, offset, totalSize);
            if (offset + 17 > totalSize) {  // 4*4 floats + 1 bool = 17 bytes
                return std::unexpected(Error{ErrorCode::InvalidFormat, "Truncated widget entry"});
            }

            WidgetLayoutEntry entry;
            entry.position.x = ReadF32(p + offset); offset += 4;
            entry.position.y = ReadF32(p + offset); offset += 4;
            entry.size.x = ReadF32(p + offset); offset += 4;
            entry.size.y = ReadF32(p + offset); offset += 4;
            entry.visible = (p[offset] != 0); offset += 1;

            profile.widgets[widgetId] = entry;
        }

        m_profiles[profile.name] = std::move(profile);
    }

    return {};
}

const LayoutProfile* LayoutProfileManager::GetProfile(std::string_view name) const {
    auto it = m_profiles.find(std::string(name));
    if (it == m_profiles.end()) return nullptr;
    return &it->second;
}

LayoutProfile LayoutProfileManager::GenerateDefaultProfile() const {
    LayoutProfile profile;
    profile.name = kDefaultProfile;

    auto allWidgets = m_registry.GetAllWidgets();
    for (const auto* state : allWidgets) {
        const auto* manifest = m_registry.GetManifest(state->id);
        if (!manifest) continue;
        WidgetLayoutEntry entry;
        entry.position = manifest->defaultPosition;
        entry.size = manifest->defaultSize;
        entry.visible = true;
        profile.widgets[state->id] = entry;
    }

    return profile;
}

LayoutProfile LayoutProfileManager::GenerateMinimalProfile() const {
    // Minimal: all at defaults, only essential widgets visible
    // Essential set is hardcoded — these IDs match first-party widget IDs (Phase 14)
    static const std::unordered_set<std::string> essentialWidgets = {
        "health", "magicka", "stamina", "compass", "crosshair"
    };

    LayoutProfile profile;
    profile.name = kMinimalProfile;

    auto allWidgets = m_registry.GetAllWidgets();
    for (const auto* state : allWidgets) {
        const auto* manifest = m_registry.GetManifest(state->id);
        if (!manifest) continue;
        WidgetLayoutEntry entry;
        entry.position = manifest->defaultPosition;
        entry.size = manifest->defaultSize;
        entry.visible = essentialWidgets.contains(state->id);
        profile.widgets[state->id] = entry;
    }

    return profile;
}

LayoutProfile LayoutProfileManager::GenerateControllerProfile() const {
    // Controller: all visible, positions offset for gamepad ergonomics
    static constexpr Vec2 kControllerOffset{20.0f, 15.0f};

    LayoutProfile profile;
    profile.name = kControllerProfile;

    auto allWidgets = m_registry.GetAllWidgets();
    for (const auto* state : allWidgets) {
        const auto* manifest = m_registry.GetManifest(state->id);
        if (!manifest) continue;
        WidgetLayoutEntry entry;
        entry.position = {manifest->defaultPosition.x + kControllerOffset.x,
                          manifest->defaultPosition.y + kControllerOffset.y};
        entry.size = manifest->defaultSize;
        entry.visible = true;
        profile.widgets[state->id] = entry;
    }

    return profile;
}

}  // namespace ohui::widget
