#pragma once

#include "ohui/core/IFileSystem.h"

#include <string>
#include <unordered_map>

namespace ohui::test {

class MockFileSystem : public IFileSystem {
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    mutable std::unordered_map<std::string, std::vector<uint8_t>> written;
    bool renameFailure = false;
    mutable std::unordered_map<std::string, std::filesystem::file_time_type> modTimes;

    std::vector<uint8_t> ReadFile(
            const std::filesystem::path& path) const override {
        auto key = path.string();
        // Check written first (for reads after writes in the same test)
        auto wit = written.find(key);
        if (wit != written.end()) return wit->second;
        auto it = files.find(key);
        if (it != files.end()) return it->second;
        return {};
    }

    bool WriteFile(
            const std::filesystem::path& path,
            std::span<const uint8_t> data) const override {
        written[path.string()] = {data.begin(), data.end()};
        return true;
    }

    bool FileExists(
            const std::filesystem::path& path) const override {
        auto key = path.string();
        return files.contains(key) || written.contains(key);
    }

    bool RenameFile(
            const std::filesystem::path& from,
            const std::filesystem::path& to) const override {
        if (renameFailure) return false;
        auto key = from.string();
        auto wit = written.find(key);
        if (wit != written.end()) {
            written[to.string()] = std::move(wit->second);
            written.erase(wit);
            return true;
        }
        auto it = files.find(key);
        if (it != files.end()) {
            written[to.string()] = it->second;
            return true;
        }
        return false;
    }

    std::optional<std::filesystem::file_time_type>
        GetModificationTime(const std::filesystem::path& path) const override {
        auto key = path.string();
        auto it = modTimes.find(key);
        if (it != modTimes.end()) return it->second;
        if (files.contains(key) || written.contains(key)) {
            return std::filesystem::file_time_type::clock::now();
        }
        return std::nullopt;
    }
};

}  // namespace ohui::test
