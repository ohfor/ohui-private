#pragma once

#include "ohui/core/IFileSystem.h"

#include <string>
#include <unordered_map>

namespace ohui::test {

class MockFileSystem : public IFileSystem {
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    std::unordered_map<std::string, std::vector<uint8_t>> written;

    std::vector<uint8_t> ReadFile(
            const std::filesystem::path& path) const override {
        auto it = files.find(path.string());
        if (it != files.end()) return it->second;
        return {};
    }

    bool WriteFile(
            const std::filesystem::path& path,
            std::span<const uint8_t> data) const override {
        const_cast<MockFileSystem*>(this)->written[path.string()] =
            {data.begin(), data.end()};
        return true;
    }

    bool FileExists(
            const std::filesystem::path& path) const override {
        return files.contains(path.string());
    }
};

}  // namespace ohui::test
