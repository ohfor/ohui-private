#pragma once

#include <cstdint>
#include <string>

namespace ohui::message {

enum class MessagePriority { Low, Normal, High, Critical };

struct MessageTypeInfo {
    std::string id;
    std::string displayName;
    bool defaultVisible{true};
    float defaultLifetime{5.0f};
};

struct Message {
    uint64_t id{0};
    std::string typeId;
    std::string content;
    std::string source;
    MessagePriority priority{MessagePriority::Normal};
    float lifetimeHint{5.0f};
    std::string speaker;
    std::string questId;
    double gameTime{0.0};
    double realTime{0.0};
};

}  // namespace ohui::message
