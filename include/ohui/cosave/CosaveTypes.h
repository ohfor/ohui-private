#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ohui::cosave {

enum class BlockType : uint16_t {
    LayoutProfiles = 0x0001,
    OutfitDefs = 0x0002,
    MCMValues = 0x0003,
    MessageLog = 0x0004,
    FavouritesRadial = 0x0005,
    MCMListConfig = 0x0006,
    InputBindings = 0x0007,
    ModData = 0x0008,
    Unknown = 0xFFFF,
};

struct BlockTypeHash {
    size_t operator()(BlockType bt) const noexcept {
        return std::hash<uint16_t>{}(static_cast<uint16_t>(bt));
    }
};

struct CosaveHeader {
    char magic[4]{'O', 'H', 'U', 'I'};
    uint16_t formatVersion{1};
    uint64_t characterId{0};
};

struct BlockEntry {
    BlockType type{BlockType::Unknown};
    uint16_t version{0};
    uint32_t size{0};
    uint32_t checksum{0};
};

struct DataBlock {
    BlockEntry entry;
    std::vector<uint8_t> data;
};

struct CosaveData {
    CosaveHeader header;
    std::unordered_map<BlockType, DataBlock, BlockTypeHash> blocks;

    const DataBlock* GetBlock(BlockType type) const {
        auto it = blocks.find(type);
        return it != blocks.end() ? &it->second : nullptr;
    }

    void SetBlock(BlockType type, uint16_t version, std::vector<uint8_t> data) {
        DataBlock block;
        block.entry.type = type;
        block.entry.version = version;
        block.entry.size = static_cast<uint32_t>(data.size());
        block.data = std::move(data);
        blocks[type] = std::move(block);
    }
};

}  // namespace ohui::cosave
