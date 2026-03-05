#pragma once

#include "ohui/cosave/CosaveTypes.h"
#include "ohui/core/IFileSystem.h"
#include "ohui/core/Result.h"

#include <filesystem>

namespace ohui::cosave {

class CosaveManager {
public:
    explicit CosaveManager(const IFileSystem& fs);

    Result<CosaveData> Read(const std::filesystem::path& path) const;
    Result<void> Write(const std::filesystem::path& path, const CosaveData& data) const;

    static CosaveData CreateEmpty(uint64_t characterId);

private:
    const IFileSystem& m_fs;
};

}  // namespace ohui::cosave
