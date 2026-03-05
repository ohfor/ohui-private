#pragma once

#include "ohui/mcm/MCM2DefinitionDiff.h"
#include "ohui/mcm/MCM2DefinitionParser.h"
#include "ohui/mcm/MCM2ConditionEngine.h"
#include "ohui/mcm/MCM2PersistenceManager.h"
#include "ohui/core/IFileSystem.h"
#include "ohui/core/Result.h"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace ohui::mcm {

struct MCM2WatchedFile {
    std::filesystem::path path;
    std::string mcmId;
    std::filesystem::file_time_type lastModified;
    MCM2Definition currentDefinition;
};

using HotReloadCallback = std::function<void(const std::string& mcmId,
                                              const MCM2DefinitionDelta& delta)>;

class MCM2HotReloadEngine {
public:
    MCM2HotReloadEngine(const IFileSystem& fs,
                         MCM2DefinitionParser& parser,
                         MCM2ConditionEngine& conditions,
                         MCM2PersistenceManager& persistence);

    Result<void> WatchFile(const std::filesystem::path& path);
    Result<void> UnwatchFile(const std::filesystem::path& path);
    void UnwatchAll();

    void CheckForChanges();
    Result<MCM2DefinitionDelta> ReloadFile(const std::filesystem::path& path);

    void SetReloadCallback(HotReloadCallback callback);

    size_t WatchedFileCount() const;
    bool IsWatching(const std::filesystem::path& path) const;
    const MCM2WatchedFile* GetWatchedFile(const std::filesystem::path& path) const;

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

private:
    const IFileSystem& m_fs;
    MCM2DefinitionParser& m_parser;
    MCM2ConditionEngine& m_conditions;
    MCM2PersistenceManager& m_persistence;
    std::unordered_map<std::string, MCM2WatchedFile> m_watchedFiles;
    HotReloadCallback m_callback;
    bool m_enabled{false};

    Result<MCM2DefinitionDelta> ProcessFileChange(MCM2WatchedFile& file);
    void ApplyDelta(const std::string& mcmId, const MCM2DefinitionDelta& delta,
                     const MCM2Definition& newDef);
};

}  // namespace ohui::mcm
