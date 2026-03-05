#include "ohui/message/MessageStream.h"

#include <algorithm>
#include <cstring>

namespace ohui::message {

// --- Type registration ---

Result<void> MessageStream::RegisterType(const MessageTypeInfo& info) {
    if (m_types.contains(info.id)) {
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Message type already registered: " + info.id});
    }
    m_types[info.id] = info;
    return {};
}

bool MessageStream::HasType(std::string_view typeId) const {
    return m_types.contains(std::string(typeId));
}

const MessageTypeInfo* MessageStream::GetTypeInfo(std::string_view typeId) const {
    auto it = m_types.find(std::string(typeId));
    if (it == m_types.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> MessageStream::GetAllTypeIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_types.size());
    for (const auto& [id, info] : m_types) {
        ids.push_back(id);
    }
    return ids;
}

size_t MessageStream::TypeCount() const {
    return m_types.size();
}

// --- Publishing ---

uint64_t MessageStream::Publish(Message msg) {
    msg.id = m_nextId++;
    uint64_t assignedId = msg.id;

    // Notify matching subscribers
    for (const auto& sub : m_subscribers) {
        if (MatchesFilter(msg, sub.filter)) {
            sub.callback(msg);
        }
    }

    m_messages.push_back(std::move(msg));
    EvictOldest();

    return assignedId;
}

// --- Subscribing ---

Result<void> MessageStream::Subscribe(std::string_view subscriberId,
                                       SubscriptionFilter filter, MessageCallback callback) {
    for (const auto& sub : m_subscribers) {
        if (sub.id == subscriberId) {
            return std::unexpected(Error{ErrorCode::DuplicateRegistration,
                "Subscriber already registered: " + std::string(subscriberId)});
        }
    }

    SubscriberEntry entry;
    entry.id = std::string(subscriberId);
    entry.filter = std::move(filter);
    entry.callback = std::move(callback);
    m_subscribers.push_back(std::move(entry));
    return {};
}

Result<void> MessageStream::Unsubscribe(std::string_view subscriberId) {
    auto it = std::find_if(m_subscribers.begin(), m_subscribers.end(),
        [&](const SubscriberEntry& s) { return s.id == subscriberId; });
    if (it == m_subscribers.end()) {
        return std::unexpected(Error{ErrorCode::NotRegistered,
            "Subscriber not found: " + std::string(subscriberId)});
    }
    m_subscribers.erase(it);
    return {};
}

bool MessageStream::HasSubscriber(std::string_view subscriberId) const {
    return std::any_of(m_subscribers.begin(), m_subscribers.end(),
        [&](const SubscriberEntry& s) { return s.id == subscriberId; });
}

size_t MessageStream::SubscriberCount() const {
    return m_subscribers.size();
}

// --- Query ---

const Message* MessageStream::GetMessage(uint64_t messageId) const {
    for (const auto& msg : m_messages) {
        if (msg.id == messageId) return &msg;
    }
    return nullptr;
}

std::vector<const Message*> MessageStream::GetMessages(std::string_view typeId,
                                                         size_t limit) const {
    std::vector<const Message*> result;
    // Iterate in reverse for most-recent-first when limit is specified
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
        if (it->typeId == typeId) {
            result.push_back(&(*it));
            if (limit > 0 && result.size() >= limit) break;
        }
    }
    return result;
}

std::vector<const Message*> MessageStream::GetAllMessages(size_t limit) const {
    std::vector<const Message*> result;
    // Most-recent-first when limit is specified
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
        result.push_back(&(*it));
        if (limit > 0 && result.size() >= limit) break;
    }
    return result;
}

size_t MessageStream::MessageCount() const {
    return m_messages.size();
}

size_t MessageStream::MessageCount(std::string_view typeId) const {
    return static_cast<size_t>(std::count_if(m_messages.begin(), m_messages.end(),
        [&](const Message& msg) { return msg.typeId == typeId; }));
}

// --- Session management ---

void MessageStream::Clear() {
    m_messages.clear();
    m_nextId = 1;
}

void MessageStream::SetMaxMessages(size_t max) {
    m_maxMessages = max;
    EvictOldest();
}

size_t MessageStream::GetMaxMessages() const {
    return m_maxMessages;
}

// --- Serialization ---

// Binary format helpers
static void WriteU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

static void WriteU8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

static void WriteF32(std::vector<uint8_t>& buf, float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    WriteU32(buf, bits);
}

static void WriteF64(std::vector<uint8_t>& buf, double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    WriteU32(buf, static_cast<uint32_t>(bits & 0xFFFFFFFF));
    WriteU32(buf, static_cast<uint32_t>((bits >> 32) & 0xFFFFFFFF));
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& s) {
    WriteU32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

struct ReadCursor {
    std::span<const uint8_t> data;
    size_t pos{0};

    bool HasBytes(size_t n) const { return pos + n <= data.size(); }

    uint32_t ReadU32() {
        uint32_t v = 0;
        v |= static_cast<uint32_t>(data[pos]);
        v |= static_cast<uint32_t>(data[pos + 1]) << 8;
        v |= static_cast<uint32_t>(data[pos + 2]) << 16;
        v |= static_cast<uint32_t>(data[pos + 3]) << 24;
        pos += 4;
        return v;
    }

    uint8_t ReadU8() { return data[pos++]; }

    float ReadF32() {
        uint32_t bits = ReadU32();
        float v;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    double ReadF64() {
        uint64_t lo = ReadU32();
        uint64_t hi = ReadU32();
        uint64_t bits = lo | (hi << 32);
        double v;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    std::string ReadString() {
        uint32_t len = ReadU32();
        std::string s(reinterpret_cast<const char*>(data.data() + pos), len);
        pos += len;
        return s;
    }
};

std::vector<uint8_t> MessageStream::SerializeRecent(size_t count) const {
    std::vector<uint8_t> buf;

    // Version
    WriteU32(buf, 1);

    // Determine messages to serialize (most recent N)
    size_t startIdx = 0;
    if (count < m_messages.size()) {
        startIdx = m_messages.size() - count;
    }
    uint32_t msgCount = static_cast<uint32_t>(m_messages.size() - startIdx);
    WriteU32(buf, msgCount);

    for (size_t i = startIdx; i < m_messages.size(); ++i) {
        const auto& msg = m_messages[i];
        WriteString(buf, msg.typeId);
        WriteString(buf, msg.content);
        WriteString(buf, msg.source);
        WriteU8(buf, static_cast<uint8_t>(msg.priority));
        WriteF32(buf, msg.lifetimeHint);
        WriteString(buf, msg.speaker);
        WriteString(buf, msg.questId);
        WriteF64(buf, msg.gameTime);
        WriteF64(buf, msg.realTime);
    }

    return buf;
}

Result<size_t> MessageStream::DeserializeInto(std::span<const uint8_t> data) {
    if (data.size() < 8) {
        return std::unexpected(Error{ErrorCode::InvalidFormat,
            "Serialized data too small"});
    }

    ReadCursor cursor{data};

    uint32_t version = cursor.ReadU32();
    if (version != 1) {
        return std::unexpected(Error{ErrorCode::VersionMismatch,
            "Unsupported serialization version: " + std::to_string(version)});
    }

    uint32_t count = cursor.ReadU32();
    size_t loaded = 0;

    for (uint32_t i = 0; i < count; ++i) {
        if (!cursor.HasBytes(4)) break;

        Message msg;
        msg.typeId = cursor.ReadString();
        msg.content = cursor.ReadString();
        msg.source = cursor.ReadString();
        msg.priority = static_cast<MessagePriority>(cursor.ReadU8());
        msg.lifetimeHint = cursor.ReadF32();
        msg.speaker = cursor.ReadString();
        msg.questId = cursor.ReadString();
        msg.gameTime = cursor.ReadF64();
        msg.realTime = cursor.ReadF64();

        // Assign new IDs to deserialized messages
        msg.id = m_nextId++;
        m_messages.push_back(std::move(msg));
        ++loaded;
    }

    EvictOldest();
    return loaded;
}

// --- Private ---

bool MessageStream::MatchesFilter(const Message& msg,
                                    const SubscriptionFilter& filter) const {
    // Priority check
    if (static_cast<int>(msg.priority) < static_cast<int>(filter.minPriority)) {
        return false;
    }

    // Type filter: empty = all types
    if (!filter.typeIds.empty()) {
        bool found = false;
        for (const auto& t : filter.typeIds) {
            if (t == msg.typeId) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}

void MessageStream::EvictOldest() {
    if (m_messages.size() > m_maxMessages) {
        size_t excess = m_messages.size() - m_maxMessages;
        m_messages.erase(m_messages.begin(), m_messages.begin() + static_cast<ptrdiff_t>(excess));
    }
}

}  // namespace ohui::message
