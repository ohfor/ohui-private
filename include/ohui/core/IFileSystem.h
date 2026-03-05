#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace ohui {

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual std::vector<uint8_t> ReadFile(
        const std::filesystem::path& path) const = 0;
    virtual bool WriteFile(
        const std::filesystem::path& path,
        std::span<const uint8_t> data) const = 0;
    virtual bool FileExists(
        const std::filesystem::path& path) const = 0;
    virtual bool RenameFile(
        const std::filesystem::path& from,
        const std::filesystem::path& to) const = 0;
    virtual std::optional<std::filesystem::file_time_type>
        GetModificationTime(const std::filesystem::path& path) const = 0;
};

}  // namespace ohui
