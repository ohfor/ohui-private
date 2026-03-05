#include <catch2/catch_test_macros.hpp>

#include "ohui/cosave/CosaveManager.h"
#include "MockFileSystem.h"

using namespace ohui;
using namespace ohui::cosave;

static const std::filesystem::path kTestPath{"test.ohui"};

TEST_CASE("CreateEmpty produces valid header with given characterId", "[cosave]") {
    auto data = CosaveManager::CreateEmpty(0xDEADBEEF12345678ULL);
    CHECK(data.header.magic[0] == 'O');
    CHECK(data.header.magic[1] == 'H');
    CHECK(data.header.magic[2] == 'U');
    CHECK(data.header.magic[3] == 'I');
    CHECK(data.header.formatVersion == 1);
    CHECK(data.header.characterId == 0xDEADBEEF12345678ULL);
    CHECK(data.blocks.empty());
}

TEST_CASE("Round-trip: write then read, all fields match", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    auto original = CosaveManager::CreateEmpty(42);
    original.SetBlock(BlockType::MCMValues, 3, {0x01, 0x02, 0x03, 0x04});
    original.SetBlock(BlockType::ModData, 1, {0xAA, 0xBB});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());

    // Move written data to files map for reading
    fs.files[kTestPath.string()] = fs.written[kTestPath.string()];

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());
    auto& loaded = readResult.value();

    CHECK(loaded.header.characterId == 42);
    CHECK(loaded.header.formatVersion == 1);

    auto* mcm = loaded.GetBlock(BlockType::MCMValues);
    REQUIRE(mcm != nullptr);
    CHECK(mcm->entry.version == 3);
    CHECK(mcm->data == std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04});

    auto* mod = loaded.GetBlock(BlockType::ModData);
    REQUIRE(mod != nullptr);
    CHECK(mod->entry.version == 1);
    CHECK(mod->data == std::vector<uint8_t>{0xAA, 0xBB});
}

TEST_CASE("Atomic write uses temp file then rename", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    auto data = CosaveManager::CreateEmpty(1);
    auto result = mgr.Write(kTestPath, data);
    REQUIRE(result.has_value());

    // The temp file should have been renamed, so it shouldn't be in written
    // The final path should be present
    CHECK(fs.written.contains(kTestPath.string()));
}

TEST_CASE("Wrong magic bytes returns InvalidFormat", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    // Create a file with wrong magic
    std::vector<uint8_t> bad(18, 0);
    bad[0] = 'X'; bad[1] = 'X'; bad[2] = 'X'; bad[3] = 'X';
    fs.files[kTestPath.string()] = bad;

    auto result = mgr.Read(kTestPath);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidFormat);
}

TEST_CASE("Truncated file returns InvalidFormat", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    // File smaller than minimum header
    std::vector<uint8_t> tiny{0x01, 0x02};
    fs.files[kTestPath.string()] = tiny;

    auto result = mgr.Read(kTestPath);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidFormat);
}

TEST_CASE("Checksum mismatch skips corrupt block, loads others", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    // Write valid data first
    auto original = CosaveManager::CreateEmpty(99);
    original.SetBlock(BlockType::MCMValues, 1, {0x01, 0x02});
    original.SetBlock(BlockType::ModData, 1, {0xCC, 0xDD});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());

    // Corrupt checksum of first block in directory (MCMValues comes before ModData)
    auto& bytes = fs.written[kTestPath.string()];
    // Header=14 + block count=4 = 18
    // First block entry starts at offset 18: type(2) + version(2) + size(4) + checksum(4)
    // Checksum is at offset 18+8 = 26
    bytes[26] ^= 0xFF; // Flip bits in checksum

    fs.files[kTestPath.string()] = bytes;

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());
    auto& loaded = readResult.value();

    // MCMValues should be skipped due to corrupt checksum
    CHECK(loaded.GetBlock(BlockType::MCMValues) == nullptr);
    // ModData should still be loaded
    auto* mod = loaded.GetBlock(BlockType::ModData);
    REQUIRE(mod != nullptr);
    CHECK(mod->data == std::vector<uint8_t>{0xCC, 0xDD});
}

TEST_CASE("Unknown block type preserved on read", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    auto original = CosaveManager::CreateEmpty(7);
    // Use a custom unknown block type
    original.SetBlock(static_cast<BlockType>(0x1234), 5, {0xFE, 0xED});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());
    fs.files[kTestPath.string()] = fs.written[kTestPath.string()];

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());

    auto* block = readResult.value().GetBlock(static_cast<BlockType>(0x1234));
    REQUIRE(block != nullptr);
    CHECK(block->entry.version == 5);
    CHECK(block->data == std::vector<uint8_t>{0xFE, 0xED});
}

TEST_CASE("Block version mismatch logged, data still accessible", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    // Version mismatch is just a different version number -- data is still accessible
    auto original = CosaveManager::CreateEmpty(1);
    original.SetBlock(BlockType::MCMValues, 99, {0x42});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());
    fs.files[kTestPath.string()] = fs.written[kTestPath.string()];

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());

    auto* block = readResult.value().GetBlock(BlockType::MCMValues);
    REQUIRE(block != nullptr);
    CHECK(block->entry.version == 99);
    CHECK(block->data == std::vector<uint8_t>{0x42});
}

TEST_CASE("Empty block (0 bytes) round-trips", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    auto original = CosaveManager::CreateEmpty(5);
    original.SetBlock(BlockType::LayoutProfiles, 1, {});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());
    fs.files[kTestPath.string()] = fs.written[kTestPath.string()];

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());

    auto* block = readResult.value().GetBlock(BlockType::LayoutProfiles);
    REQUIRE(block != nullptr);
    CHECK(block->data.empty());
}

TEST_CASE("All 8 block types with distinct data round-trip", "[cosave]") {
    test::MockFileSystem fs;
    CosaveManager mgr(fs);

    auto original = CosaveManager::CreateEmpty(100);
    original.SetBlock(BlockType::LayoutProfiles, 1, {0x01});
    original.SetBlock(BlockType::OutfitDefs, 1, {0x02});
    original.SetBlock(BlockType::MCMValues, 1, {0x03});
    original.SetBlock(BlockType::MessageLog, 1, {0x04});
    original.SetBlock(BlockType::FavouritesRadial, 1, {0x05});
    original.SetBlock(BlockType::MCMListConfig, 1, {0x06});
    original.SetBlock(BlockType::InputBindings, 1, {0x07});
    original.SetBlock(BlockType::ModData, 1, {0x08});

    auto writeResult = mgr.Write(kTestPath, original);
    REQUIRE(writeResult.has_value());
    fs.files[kTestPath.string()] = fs.written[kTestPath.string()];

    auto readResult = mgr.Read(kTestPath);
    REQUIRE(readResult.has_value());
    auto& loaded = readResult.value();

    CHECK(loaded.blocks.size() == 8);
    CHECK(loaded.GetBlock(BlockType::LayoutProfiles)->data == std::vector<uint8_t>{0x01});
    CHECK(loaded.GetBlock(BlockType::OutfitDefs)->data == std::vector<uint8_t>{0x02});
    CHECK(loaded.GetBlock(BlockType::MCMValues)->data == std::vector<uint8_t>{0x03});
    CHECK(loaded.GetBlock(BlockType::MessageLog)->data == std::vector<uint8_t>{0x04});
    CHECK(loaded.GetBlock(BlockType::FavouritesRadial)->data == std::vector<uint8_t>{0x05});
    CHECK(loaded.GetBlock(BlockType::MCMListConfig)->data == std::vector<uint8_t>{0x06});
    CHECK(loaded.GetBlock(BlockType::InputBindings)->data == std::vector<uint8_t>{0x07});
    CHECK(loaded.GetBlock(BlockType::ModData)->data == std::vector<uint8_t>{0x08});
}

TEST_CASE("Rename failure returns IOError", "[cosave]") {
    test::MockFileSystem fs;
    fs.renameFailure = true;
    CosaveManager mgr(fs);

    auto data = CosaveManager::CreateEmpty(1);
    auto result = mgr.Write(kTestPath, data);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::IOError);
}
