#include "ohui/mcm/MCM2HotReloadEngine.h"
#include "ohui/core/Log.h"

namespace ohui::mcm {

MCM2HotReloadEngine::MCM2HotReloadEngine(const IFileSystem& fs,
                                           MCM2DefinitionParser& parser,
                                           MCM2ConditionEngine& conditions,
                                           MCM2PersistenceManager& persistence)
    : m_fs(fs), m_parser(parser), m_conditions(conditions), m_persistence(persistence) {}

Result<void> MCM2HotReloadEngine::WatchFile(const std::filesystem::path& path) {
    auto modTime = m_fs.GetModificationTime(path);
    if (!modTime.has_value()) {
        return std::unexpected(Error{ErrorCode::FileNotFound,
            "file not found: " + path.string()});
    }

    // Read and parse the file
    auto data = m_fs.ReadFile(path);
    if (data.empty()) {
        return std::unexpected(Error{ErrorCode::FileNotFound,
            "could not read file: " + path.string()});
    }

    std::string_view content(reinterpret_cast<const char*>(data.data()), data.size());
    auto parseResult = m_parser.Parse(content, path.string());
    if (!parseResult.has_value()) {
        return std::unexpected(parseResult.error());
    }

    if (parseResult->hasErrors) {
        return std::unexpected(Error{ErrorCode::ParseError,
            "parse errors in: " + path.string()});
    }

    MCM2WatchedFile watched;
    watched.path = path;
    watched.mcmId = parseResult->definition.id;
    watched.lastModified = *modTime;
    watched.currentDefinition = std::move(parseResult->definition);

    // Register with persistence
    m_persistence.RegisterDefinition(watched.currentDefinition);
    m_persistence.ApplyDefaults(watched.currentDefinition);

    // Compile conditions
    m_conditions.CompileConditions(watched.currentDefinition);

    m_watchedFiles[path.string()] = std::move(watched);
    return {};
}

Result<void> MCM2HotReloadEngine::UnwatchFile(const std::filesystem::path& path) {
    auto it = m_watchedFiles.find(path.string());
    if (it == m_watchedFiles.end()) {
        return std::unexpected(Error{ErrorCode::FileNotFound,
            "not watching: " + path.string()});
    }
    m_watchedFiles.erase(it);
    return {};
}

void MCM2HotReloadEngine::UnwatchAll() {
    m_watchedFiles.clear();
}

void MCM2HotReloadEngine::CheckForChanges() {
    if (!m_enabled) return;

    for (auto& [key, file] : m_watchedFiles) {
        auto modTime = m_fs.GetModificationTime(file.path);
        if (!modTime.has_value()) continue;

        if (*modTime != file.lastModified) {
            auto result = ProcessFileChange(file);
            if (result.has_value() && m_callback) {
                m_callback(file.mcmId, *result);
            }
        }
    }
}

Result<MCM2DefinitionDelta> MCM2HotReloadEngine::ReloadFile(const std::filesystem::path& path) {
    auto it = m_watchedFiles.find(path.string());
    if (it == m_watchedFiles.end()) {
        return std::unexpected(Error{ErrorCode::FileNotFound,
            "not watching: " + path.string()});
    }
    return ProcessFileChange(it->second);
}

Result<MCM2DefinitionDelta> MCM2HotReloadEngine::ProcessFileChange(MCM2WatchedFile& file) {
    auto data = m_fs.ReadFile(file.path);
    if (data.empty()) {
        return std::unexpected(Error{ErrorCode::FileNotFound,
            "could not read file: " + file.path.string()});
    }

    std::string_view content(reinterpret_cast<const char*>(data.data()), data.size());
    auto parseResult = m_parser.Parse(content, file.path.string());
    if (!parseResult.has_value()) {
        return std::unexpected(parseResult.error());
    }

    if (parseResult->hasErrors) {
        // Parse error: keep existing definition
        return std::unexpected(Error{ErrorCode::ParseError,
            "parse errors in: " + file.path.string()});
    }

    // Compute diff
    auto delta = ComputeDiff(file.currentDefinition, parseResult->definition);

    // Update modification time
    auto modTime = m_fs.GetModificationTime(file.path);
    if (modTime.has_value()) {
        file.lastModified = *modTime;
    }

    // Apply delta
    if (delta.HasAnyChanges()) {
        ApplyDelta(file.mcmId, delta, parseResult->definition);
        file.currentDefinition = std::move(parseResult->definition);
        file.mcmId = file.currentDefinition.id;
    }

    return delta;
}

void MCM2HotReloadEngine::ApplyDelta(const std::string& /*mcmId*/,
                                       const MCM2DefinitionDelta& delta,
                                       const MCM2Definition& newDef) {
    if (delta.mcmIdChanged) {
        ohui::log::debug("MCM2 hot reload: MCM ID changed from '{}' to '{}' (destructive)",
                          delta.oldMcmId, delta.newMcmId);
    }

    // Register new definition and apply defaults for newly added controls
    m_persistence.RegisterDefinition(newDef);
    m_persistence.ApplyDefaults(newDef);

    // Recompile conditions
    m_conditions.CompileConditions(newDef);

    for (const auto& change : delta.controlChanges) {
        if (change.type == DiffChangeType::Destructive) {
            ohui::log::debug("MCM2 hot reload: destructive change for control '{}': {}",
                              change.controlId, change.description);
        }
    }
}

void MCM2HotReloadEngine::SetReloadCallback(HotReloadCallback callback) {
    m_callback = std::move(callback);
}

size_t MCM2HotReloadEngine::WatchedFileCount() const {
    return m_watchedFiles.size();
}

bool MCM2HotReloadEngine::IsWatching(const std::filesystem::path& path) const {
    return m_watchedFiles.contains(path.string());
}

const MCM2WatchedFile* MCM2HotReloadEngine::GetWatchedFile(const std::filesystem::path& path) const {
    auto it = m_watchedFiles.find(path.string());
    if (it != m_watchedFiles.end()) return &it->second;
    return nullptr;
}

void MCM2HotReloadEngine::SetEnabled(bool enabled) {
    m_enabled = enabled;
}

bool MCM2HotReloadEngine::IsEnabled() const {
    return m_enabled;
}

}  // namespace ohui::mcm
