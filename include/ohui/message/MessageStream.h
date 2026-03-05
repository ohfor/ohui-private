#pragma once

#include "ohui/message/MessageTypes.h"
#include "ohui/core/Result.h"

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ohui::message {

using MessageCallback = std::function<void(const Message&)>;

struct SubscriptionFilter {
    std::vector<std::string> typeIds;
    MessagePriority minPriority{MessagePriority::Low};
};

class MessageStream {
public:
    // Type registration
    Result<void> RegisterType(const MessageTypeInfo& info);
    bool HasType(std::string_view typeId) const;
    const MessageTypeInfo* GetTypeInfo(std::string_view typeId) const;
    std::vector<std::string> GetAllTypeIds() const;
    size_t TypeCount() const;

    // Publishing
    uint64_t Publish(Message msg);

    // Subscribing
    Result<void> Subscribe(std::string_view subscriberId,
                           SubscriptionFilter filter, MessageCallback callback);
    Result<void> Unsubscribe(std::string_view subscriberId);
    bool HasSubscriber(std::string_view subscriberId) const;
    size_t SubscriberCount() const;

    // Query
    const Message* GetMessage(uint64_t messageId) const;
    std::vector<const Message*> GetMessages(std::string_view typeId,
                                             size_t limit = 0) const;
    std::vector<const Message*> GetAllMessages(size_t limit = 0) const;
    size_t MessageCount() const;
    size_t MessageCount(std::string_view typeId) const;

    // Session management
    void Clear();
    void SetMaxMessages(size_t max);
    size_t GetMaxMessages() const;

    // Serialization
    std::vector<uint8_t> SerializeRecent(size_t count) const;
    Result<size_t> DeserializeInto(std::span<const uint8_t> data);

private:
    std::vector<Message> m_messages;
    uint64_t m_nextId{1};
    size_t m_maxMessages{10000};
    std::unordered_map<std::string, MessageTypeInfo> m_types;

    struct SubscriberEntry {
        std::string id;
        SubscriptionFilter filter;
        MessageCallback callback;
    };
    std::vector<SubscriberEntry> m_subscribers;

    bool MatchesFilter(const Message& msg, const SubscriptionFilter& filter) const;
    void EvictOldest();
};

}  // namespace ohui::message
