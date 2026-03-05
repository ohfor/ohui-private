#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ohui::mcm {

// Option ID encoding -- matches SkyUI: (pageIndex * 256) + optionIndex
// Max 128 options per page (SkyUI uses indices 0-127, top bit reserved for flags)
constexpr int32_t kMaxOptionsPerPage = 128;
constexpr int32_t kOptionIdPageMultiplier = 256;

inline int32_t EncodeOptionId(int32_t pageIndex, int32_t optionIndex) {
    return (pageIndex * kOptionIdPageMultiplier) + optionIndex;
}
inline int32_t DecodePageIndex(int32_t optionId) {
    return optionId / kOptionIdPageMultiplier;
}
inline int32_t DecodeOptionIndex(int32_t optionId) {
    return optionId % kOptionIdPageMultiplier;
}

enum class MCMOptionType {
    Toggle, Slider, Menu, Text, Color, KeyMap, Header, Empty
};

enum class MCMOptionFlag : uint32_t {
    None     = 0,
    Disabled = 1 << 0,
    Hidden   = 1 << 1,
};

inline MCMOptionFlag operator|(MCMOptionFlag a, MCMOptionFlag b) {
    return static_cast<MCMOptionFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline MCMOptionFlag operator&(MCMOptionFlag a, MCMOptionFlag b) {
    return static_cast<MCMOptionFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline MCMOptionFlag operator~(MCMOptionFlag a) {
    return static_cast<MCMOptionFlag>(~static_cast<uint32_t>(a));
}
inline MCMOptionFlag& operator|=(MCMOptionFlag& a, MCMOptionFlag b) {
    a = a | b;
    return a;
}
inline MCMOptionFlag& operator&=(MCMOptionFlag& a, MCMOptionFlag b) {
    a = a & b;
    return a;
}
inline bool HasFlag(MCMOptionFlag flags, MCMOptionFlag test) {
    return (flags & test) == test;
}

struct MCMOptionData {
    int32_t id{-1};
    MCMOptionType type{MCMOptionType::Empty};
    std::string name;
    MCMOptionFlag flags{MCMOptionFlag::None};

    // Type-specific current values
    bool boolValue{false};
    float floatValue{0.0f};
    int32_t intValue{0};
    std::string stringValue;
    std::string formatString;
};

struct MCMPageData {
    std::string name;
    int32_t pageIndex{-1};
    std::vector<MCMOptionData> options;
};

enum class MCMModState {
    Registered,
    Building,
    Ready,
};

struct MCMModData {
    std::string modName;
    std::vector<MCMPageData> pages;
    MCMModState state{MCMModState::Registered};
    int32_t activeBuildPageIndex{-1};
};

struct SliderDialogParams {
    float defaultValue{0.0f};
    float minValue{0.0f};
    float maxValue{100.0f};
    float stepSize{1.0f};
    float currentValue{0.0f};
};

struct MenuDialogParams {
    std::vector<std::string> menuItems;
    int32_t defaultIndex{0};
    int32_t currentIndex{0};
};

struct InfoTextData {
    std::string text;
};

}  // namespace ohui::mcm
