#include "ohui/cosave/CosaveManager.h"
#include "ohui/core/Log.h"

#include <cstring>

namespace ohui::cosave {

// --- CRC32 (standard polynomial 0xEDB88320) ---

static uint32_t Crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (~(crc & 1) + 1));
        }
    }
    return ~crc;
}

// --- Binary serialization helpers ---

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

static uint16_t ReadU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t ReadU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

static uint64_t ReadU64(const uint8_t* p) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= static_cast<uint64_t>(p[i]) << (i * 8);
    }
    return val;
}

// --- Header constants ---
static constexpr size_t kHeaderSize = 14;        // magic(4) + version(2) + charId(8)
static constexpr size_t kBlockCountSize = 4;
static constexpr size_t kBlockEntrySize = 12;    // type(2) + version(2) + size(4) + checksum(4)

// --- CosaveManager ---

CosaveManager::CosaveManager(const IFileSystem& fs) : m_fs(fs) {}

CosaveData CosaveManager::CreateEmpty(uint64_t characterId) {
    CosaveData data;
    data.header.magic[0] = 'O';
    data.header.magic[1] = 'H';
    data.header.magic[2] = 'U';
    data.header.magic[3] = 'I';
    data.header.formatVersion = 1;
    data.header.characterId = characterId;
    return data;
}

Result<CosaveData> CosaveManager::Read(const std::filesystem::path& path) const {
    auto bytes = m_fs.ReadFile(path);

    if (bytes.size() < kHeaderSize + kBlockCountSize) {
        return std::unexpected(Error{ErrorCode::InvalidFormat, "File too small for cosave header"});
    }

    // Parse header
    if (bytes[0] != 'O' || bytes[1] != 'H' || bytes[2] != 'U' || bytes[3] != 'I') {
        return std::unexpected(Error{ErrorCode::InvalidFormat, "Invalid magic bytes"});
    }

    CosaveData result;
    std::memcpy(result.header.magic, bytes.data(), 4);
    result.header.formatVersion = ReadU16(bytes.data() + 4);
    result.header.characterId = ReadU64(bytes.data() + 6);

    // Block count
    size_t offset = kHeaderSize;
    uint32_t blockCount = ReadU32(bytes.data() + offset);
    offset += kBlockCountSize;

    // Validate directory fits
    size_t directoryEnd = offset + static_cast<size_t>(blockCount) * kBlockEntrySize;
    if (directoryEnd > bytes.size()) {
        return std::unexpected(Error{ErrorCode::InvalidFormat, "File truncated in block directory"});
    }

    // Parse block directory
    std::vector<BlockEntry> entries;
    entries.reserve(blockCount);
    for (uint32_t i = 0; i < blockCount; ++i) {
        BlockEntry entry;
        const uint8_t* p = bytes.data() + offset;
        entry.type = static_cast<BlockType>(ReadU16(p));
        entry.version = ReadU16(p + 2);
        entry.size = ReadU32(p + 4);
        entry.checksum = ReadU32(p + 8);
        entries.push_back(entry);
        offset += kBlockEntrySize;
    }

    // Parse block data
    for (const auto& entry : entries) {
        if (offset + entry.size > bytes.size()) {
            ohui::log::debug("Cosave: block 0x{:04X} truncated, skipping", static_cast<uint16_t>(entry.type));
            break;
        }

        const uint8_t* blockData = bytes.data() + offset;
        uint32_t computed = Crc32(blockData, entry.size);

        if (computed != entry.checksum) {
            ohui::log::debug("Cosave: block 0x{:04X} checksum mismatch (expected 0x{:08X}, got 0x{:08X}), skipping",
                static_cast<uint16_t>(entry.type), entry.checksum, computed);
            offset += entry.size;
            continue;
        }

        DataBlock block;
        block.entry = entry;
        block.data.assign(blockData, blockData + entry.size);
        result.blocks[entry.type] = std::move(block);

        offset += entry.size;
    }

    return result;
}

Result<void> CosaveManager::Write(const std::filesystem::path& path, const CosaveData& data) const {
    std::vector<uint8_t> buf;

    // Reserve approximate size
    size_t approxSize = kHeaderSize + kBlockCountSize + data.blocks.size() * kBlockEntrySize;
    for (const auto& [type, block] : data.blocks) {
        approxSize += block.data.size();
    }
    buf.reserve(approxSize);

    // Write header
    buf.push_back(static_cast<uint8_t>(data.header.magic[0]));
    buf.push_back(static_cast<uint8_t>(data.header.magic[1]));
    buf.push_back(static_cast<uint8_t>(data.header.magic[2]));
    buf.push_back(static_cast<uint8_t>(data.header.magic[3]));
    WriteU16(buf, data.header.formatVersion);
    WriteU64(buf, data.header.characterId);

    // Collect blocks in deterministic order (sorted by type)
    std::vector<const DataBlock*> sortedBlocks;
    for (const auto& [type, block] : data.blocks) {
        sortedBlocks.push_back(&block);
    }
    std::sort(sortedBlocks.begin(), sortedBlocks.end(),
        [](const DataBlock* a, const DataBlock* b) {
            return static_cast<uint16_t>(a->entry.type) < static_cast<uint16_t>(b->entry.type);
        });

    // Write block count
    WriteU32(buf, static_cast<uint32_t>(sortedBlocks.size()));

    // Write block directory
    for (const auto* block : sortedBlocks) {
        WriteU16(buf, static_cast<uint16_t>(block->entry.type));
        WriteU16(buf, block->entry.version);
        WriteU32(buf, static_cast<uint32_t>(block->data.size()));
        WriteU32(buf, Crc32(block->data.data(), block->data.size()));
    }

    // Write block data
    for (const auto* block : sortedBlocks) {
        buf.insert(buf.end(), block->data.begin(), block->data.end());
    }

    // Atomic write: temp file + rename
    auto tempPath = path;
    tempPath += ".tmp";

    if (!m_fs.WriteFile(tempPath, buf)) {
        return std::unexpected(Error{ErrorCode::IOError, "Failed to write temp cosave file"});
    }

    if (!m_fs.RenameFile(tempPath, path)) {
        return std::unexpected(Error{ErrorCode::IOError, "Failed to rename temp cosave file"});
    }

    return {};
}

}  // namespace ohui::cosave
