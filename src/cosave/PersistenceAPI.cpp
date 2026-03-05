#include "ohui/cosave/PersistenceAPI.h"
#include "ohui/core/Log.h"

#include <cstring>

namespace ohui::cosave {

// --- Serialization helpers ---

static void WriteU8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

static void WriteU16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void WriteU32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

static void WriteU64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

static void WriteFloat(std::vector<uint8_t>& buf, float val) {
    uint32_t bits;
    std::memcpy(&bits, &val, sizeof(bits));
    WriteU32(buf, bits);
}

static void WriteString(std::vector<uint8_t>& buf, const std::string& s) {
    WriteU32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

static void WriteBytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data) {
    WriteU32(buf, static_cast<uint32_t>(data.size()));
    buf.insert(buf.end(), data.begin(), data.end());
}

static uint8_t ReadU8(const uint8_t*& p) {
    return *p++;
}

static uint16_t ReadU16(const uint8_t*& p) {
    uint16_t val = static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    p += 2;
    return val;
}

static uint32_t ReadU32(const uint8_t*& p) {
    uint32_t val = static_cast<uint32_t>(p[0])
                 | (static_cast<uint32_t>(p[1]) << 8)
                 | (static_cast<uint32_t>(p[2]) << 16)
                 | (static_cast<uint32_t>(p[3]) << 24);
    p += 4;
    return val;
}

static uint64_t ReadU64(const uint8_t*& p) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= static_cast<uint64_t>(p[i]) << (i * 8);
    }
    p += 8;
    return val;
}

static float ReadFloat(const uint8_t*& p) {
    uint32_t bits = ReadU32(p);
    float val;
    std::memcpy(&val, &bits, sizeof(val));
    return val;
}

static std::string ReadString(const uint8_t*& p) {
    uint32_t len = ReadU32(p);
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len;
    return s;
}

static std::vector<uint8_t> ReadBytesVec(const uint8_t*& p) {
    uint32_t len = ReadU32(p);
    std::vector<uint8_t> data(p, p + len);
    p += len;
    return data;
}

// --- Value type tags ---
static constexpr uint8_t kTypeBool = 0;
static constexpr uint8_t kTypeInt64 = 1;
static constexpr uint8_t kTypeFloat = 2;
static constexpr uint8_t kTypeString = 3;
static constexpr uint8_t kTypeBytes = 4;

// --- PersistenceAPI ---

PersistenceAPI::PersistenceAPI(CosaveManager& mgr) : m_mgr(mgr) {}

size_t PersistenceAPI::EstimateValueSize(const Value& v) {
    return std::visit([](const auto& val) -> size_t {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) return 1;
        else if constexpr (std::is_same_v<T, int64_t>) return 8;
        else if constexpr (std::is_same_v<T, float>) return 4;
        else if constexpr (std::is_same_v<T, std::string>) return val.size();
        else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) return val.size();
        else return 0;
    }, v);
}

Result<void> PersistenceAPI::Register(std::string_view modNamespace) {
    std::lock_guard lock(m_mutex);
    auto key = std::string(modNamespace);
    if (m_stores.contains(key)) {
        return std::unexpected(Error{ErrorCode::DuplicateRegistration,
            "Mod namespace already registered: " + key});
    }
    m_stores[key] = ModStore{};
    return {};
}

bool PersistenceAPI::IsRegistered(std::string_view modNamespace) const {
    std::lock_guard lock(m_mutex);
    return m_stores.contains(std::string(modNamespace));
}

// --- Typed setters ---

Result<void> PersistenceAPI::SetBool(std::string_view mod, std::string_view key, bool val) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    Value v = val;
    size_t newSize = EstimateValueSize(v);
    if (newSize > kMaxEntrySize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Entry too large"});

    auto& store = it->second;
    auto existing = store.entries.find(std::string(key));
    size_t oldSize = (existing != store.entries.end()) ? EstimateValueSize(existing->second) : 0;
    size_t newTotal = store.totalSize - oldSize + newSize;
    if (newTotal > kMaxModTotalSize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Mod total size exceeded"});

    store.totalSize = newTotal;
    store.entries[std::string(key)] = std::move(v);
    return {};
}

bool PersistenceAPI::GetBool(std::string_view mod, std::string_view key, bool def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return def;
    auto eit = it->second.entries.find(std::string(key));
    if (eit == it->second.entries.end()) return def;
    auto* p = std::get_if<bool>(&eit->second);
    return p ? *p : def;
}

Result<void> PersistenceAPI::SetInt(std::string_view mod, std::string_view key, int64_t val) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    Value v = val;
    size_t newSize = EstimateValueSize(v);
    if (newSize > kMaxEntrySize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Entry too large"});

    auto& store = it->second;
    auto existing = store.entries.find(std::string(key));
    size_t oldSize = (existing != store.entries.end()) ? EstimateValueSize(existing->second) : 0;
    size_t newTotal = store.totalSize - oldSize + newSize;
    if (newTotal > kMaxModTotalSize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Mod total size exceeded"});

    store.totalSize = newTotal;
    store.entries[std::string(key)] = std::move(v);
    return {};
}

int64_t PersistenceAPI::GetInt(std::string_view mod, std::string_view key, int64_t def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return def;
    auto eit = it->second.entries.find(std::string(key));
    if (eit == it->second.entries.end()) return def;
    auto* p = std::get_if<int64_t>(&eit->second);
    return p ? *p : def;
}

Result<void> PersistenceAPI::SetFloat(std::string_view mod, std::string_view key, float val) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    Value v = val;
    size_t newSize = EstimateValueSize(v);
    if (newSize > kMaxEntrySize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Entry too large"});

    auto& store = it->second;
    auto existing = store.entries.find(std::string(key));
    size_t oldSize = (existing != store.entries.end()) ? EstimateValueSize(existing->second) : 0;
    size_t newTotal = store.totalSize - oldSize + newSize;
    if (newTotal > kMaxModTotalSize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Mod total size exceeded"});

    store.totalSize = newTotal;
    store.entries[std::string(key)] = std::move(v);
    return {};
}

float PersistenceAPI::GetFloat(std::string_view mod, std::string_view key, float def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return def;
    auto eit = it->second.entries.find(std::string(key));
    if (eit == it->second.entries.end()) return def;
    auto* p = std::get_if<float>(&eit->second);
    return p ? *p : def;
}

Result<void> PersistenceAPI::SetString(std::string_view mod, std::string_view key, std::string_view val) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    Value v = std::string(val);
    size_t newSize = EstimateValueSize(v);
    if (newSize > kMaxEntrySize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Entry too large"});

    auto& store = it->second;
    auto existing = store.entries.find(std::string(key));
    size_t oldSize = (existing != store.entries.end()) ? EstimateValueSize(existing->second) : 0;
    size_t newTotal = store.totalSize - oldSize + newSize;
    if (newTotal > kMaxModTotalSize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Mod total size exceeded"});

    store.totalSize = newTotal;
    store.entries[std::string(key)] = std::move(v);
    return {};
}

std::string PersistenceAPI::GetString(std::string_view mod, std::string_view key, std::string_view def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::string(def);
    auto eit = it->second.entries.find(std::string(key));
    if (eit == it->second.entries.end()) return std::string(def);
    auto* p = std::get_if<std::string>(&eit->second);
    return p ? *p : std::string(def);
}

Result<void> PersistenceAPI::SetBytes(std::string_view mod, std::string_view key, std::span<const uint8_t> val) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    Value v = std::vector<uint8_t>(val.begin(), val.end());
    size_t newSize = EstimateValueSize(v);
    if (newSize > kMaxEntrySize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Entry too large"});

    auto& store = it->second;
    auto existing = store.entries.find(std::string(key));
    size_t oldSize = (existing != store.entries.end()) ? EstimateValueSize(existing->second) : 0;
    size_t newTotal = store.totalSize - oldSize + newSize;
    if (newTotal > kMaxModTotalSize) return std::unexpected(Error{ErrorCode::SizeLimitExceeded, "Mod total size exceeded"});

    store.totalSize = newTotal;
    store.entries[std::string(key)] = std::move(v);
    return {};
}

std::vector<uint8_t> PersistenceAPI::GetBytes(std::string_view mod, std::string_view key) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return {};
    auto eit = it->second.entries.find(std::string(key));
    if (eit == it->second.entries.end()) return {};
    auto* p = std::get_if<std::vector<uint8_t>>(&eit->second);
    return p ? *p : std::vector<uint8_t>{};
}

// --- Management ---

bool PersistenceAPI::Has(std::string_view mod, std::string_view key) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return false;
    return it->second.entries.contains(std::string(key));
}

Result<void> PersistenceAPI::Delete(std::string_view mod, std::string_view key) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    auto eit = it->second.entries.find(std::string(key));
    if (eit != it->second.entries.end()) {
        it->second.totalSize -= EstimateValueSize(eit->second);
        it->second.entries.erase(eit);
    }
    return {};
}

Result<void> PersistenceAPI::ClearAll(std::string_view mod) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    it->second.entries.clear();
    it->second.totalSize = 0;
    return {};
}

// --- Versioning ---

Result<void> PersistenceAPI::SetVersion(std::string_view mod, uint32_t version) {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    if (it == m_stores.end()) return std::unexpected(Error{ErrorCode::NotRegistered, "Mod not registered"});

    it->second.version = version;
    return {};
}

uint32_t PersistenceAPI::GetVersion(std::string_view mod) const {
    std::lock_guard lock(m_mutex);
    auto it = m_stores.find(std::string(mod));
    return it != m_stores.end() ? it->second.version : 0;
}

// --- Serialization ---

std::vector<uint8_t> PersistenceAPI::SerializeAll() const {
    std::lock_guard lock(m_mutex);
    std::vector<uint8_t> buf;

    // Format: [mod_count:u16] then for each mod:
    //   [namespace:string] [version:u32] [entry_count:u16]
    //   for each entry: [key:string] [type:u8] [value:typed]

    WriteU16(buf, static_cast<uint16_t>(m_stores.size()));

    for (const auto& [ns, store] : m_stores) {
        WriteString(buf, ns);
        WriteU32(buf, store.version);
        WriteU16(buf, static_cast<uint16_t>(store.entries.size()));

        for (const auto& [key, val] : store.entries) {
            WriteString(buf, key);

            std::visit([&buf](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, bool>) {
                    WriteU8(buf, kTypeBool);
                    WriteU8(buf, v ? 1 : 0);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    WriteU8(buf, kTypeInt64);
                    WriteU64(buf, static_cast<uint64_t>(v));
                } else if constexpr (std::is_same_v<T, float>) {
                    WriteU8(buf, kTypeFloat);
                    WriteFloat(buf, v);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    WriteU8(buf, kTypeString);
                    WriteString(buf, v);
                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    WriteU8(buf, kTypeBytes);
                    WriteBytes(buf, v);
                }
            }, val);
        }
    }

    return buf;
}

Result<void> PersistenceAPI::DeserializeAll(std::span<const uint8_t> data) {
    std::lock_guard lock(m_mutex);
    m_stores.clear();

    if (data.empty()) return {};

    const uint8_t* p = data.data();
    const uint8_t* end = data.data() + data.size();

    if (p + 2 > end) return std::unexpected(Error{ErrorCode::InvalidFormat, "Truncated persistence data"});
    uint16_t modCount = ReadU16(p);

    for (uint16_t i = 0; i < modCount && p < end; ++i) {
        if (p + 4 > end) break;
        std::string ns = ReadString(p);
        if (p + 6 > end) break;
        uint32_t version = ReadU32(p);
        uint16_t entryCount = ReadU16(p);

        ModStore store;
        store.version = version;

        for (uint16_t j = 0; j < entryCount && p < end; ++j) {
            if (p + 4 > end) break;
            std::string key = ReadString(p);
            if (p + 1 > end) break;
            uint8_t typeTag = ReadU8(p);

            Value val;
            switch (typeTag) {
                case kTypeBool:
                    if (p + 1 > end) break;
                    val = ReadU8(p) != 0;
                    break;
                case kTypeInt64:
                    if (p + 8 > end) break;
                    val = static_cast<int64_t>(ReadU64(p));
                    break;
                case kTypeFloat:
                    if (p + 4 > end) break;
                    val = ReadFloat(p);
                    break;
                case kTypeString:
                    if (p + 4 > end) break;
                    val = ReadString(p);
                    break;
                case kTypeBytes:
                    if (p + 4 > end) break;
                    val = ReadBytesVec(p);
                    break;
                default:
                    ohui::log::debug("Persistence: unknown type tag {}, skipping entry", typeTag);
                    continue;
            }

            store.totalSize += EstimateValueSize(val);
            store.entries[key] = std::move(val);
        }

        m_stores[ns] = std::move(store);
    }

    return {};
}

void PersistenceAPI::RevertAll() {
    std::lock_guard lock(m_mutex);
    m_stores.clear();
}

}  // namespace ohui::cosave
