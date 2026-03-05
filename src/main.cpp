#include "Version.h"

#include <ShlObj.h>  // For SHGetKnownFolderPath, FOLDERID_Documents

// Plugin version info for SKSE
extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;
    v.PluginVersion(REL::Version(Version::MAJOR, Version::MINOR, Version::PATCH, 0));
    v.PluginName(Version::NAME);
    v.AuthorName(Version::AUTHOR);
    v.UsesAddressLibrary();  // Resolve addresses via Address Library (version-independent)
    v.UsesNoStructs();       // Don't depend on game struct layouts
    return v;
}();

namespace {
    /// Get the correct SKSE log directory by deriving the game name from the DLL's own path.
    /// The DLL lives in {GameRoot}\Data\SKSE\Plugins\, so we extract the game folder name.
    /// This works for SSE, VR, GOG, or any variant without hardcoding paths.
    std::optional<std::filesystem::path> GetLogDirectory() {
        HMODULE hModule = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&GetLogDirectory), &hModule)) {
            return std::nullopt;
        }

        wchar_t dllPath[MAX_PATH];
        if (GetModuleFileNameW(hModule, dllPath, MAX_PATH) == 0) {
            return std::nullopt;
        }

        // DLL path: {GameRoot}\Data\SKSE\Plugins\OHUI.dll
        // Navigate up 4 levels to get game root, then extract folder name
        std::filesystem::path dllLocation = dllPath;
        auto gameRoot = dllLocation.parent_path()  // Plugins
                                   .parent_path()  // SKSE
                                   .parent_path()  // Data
                                   .parent_path(); // GameRoot (e.g., "Skyrim Special Edition")

        auto gameFolderName = gameRoot.filename();

        // Get Documents folder
        wchar_t* docPath = nullptr;
        if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docPath))) {
            return std::nullopt;
        }

        std::filesystem::path result = docPath;
        CoTaskMemFree(docPath);

        // Build: Documents\My Games\{GameFolderName}\SKSE
        result /= "My Games";
        result /= gameFolderName;
        result /= "SKSE";
        return result;
    }

    void InitializeLogging() {
        auto logDir = GetLogDirectory();
        if (!logDir) {
            SKSE::stl::report_and_fail("Could not determine Documents folder for logging");
        }

        std::filesystem::create_directories(*logDir);

        auto path = *logDir / fmt::format(FMT_STRING("{}.log"), Version::NAME);

        try {
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
            auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
            log->set_level(spdlog::level::info);
            log->flush_on(spdlog::level::info);

            spdlog::set_default_logger(std::move(log));
            spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v"s);

            logger::info("{} v{}.{}.{} loaded", Version::NAME, Version::MAJOR, Version::MINOR, Version::PATCH);

            // Log runtime variant (SE/AE/VR)
            auto ver = REL::Module::get().version();
            const char* runtime = "Unknown";
            if (REL::Module::IsVR()) {
                runtime = "VR";
            } else if (ver >= SKSE::RUNTIME_SSE_1_6_317) {
                runtime = "AE";
            } else {
                runtime = "SE";
            }
            logger::info("Game: Skyrim {} v{}.{}.{}.{}", runtime, ver[0], ver[1], ver[2], ver[3]);
        } catch (const std::exception& ex) {
            SKSE::stl::report_and_fail(fmt::format("Log init failed: {}", ex.what()));
        }
    }

    void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                logger::info("Data loaded, initializing...");

                // Initialize services here (INI settings, registries, etc.)
                // Order matters: load config first, then services that depend on it.

                // Register event handlers here (menu events, activation events, etc.)

                // Register Papyrus native functions (if applicable):
                // if (auto* papyrus = SKSE::GetPapyrusInterface()) {
                //     papyrus->Register(MyPapyrusNamespace::RegisterFunctions);
                // }

                break;

            case SKSE::MessagingInterface::kPostLoadGame:
                logger::info("Game loaded");
                // Post-load tasks (grant spells, refresh state, etc.)
                break;

            case SKSE::MessagingInterface::kNewGame:
                logger::info("New game started");
                break;
        }
    }
}

// Legacy Query entry point for SE 1.5.97 (old SKSE64 looks for this)
extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = Version::NAME.data();
    a_info->version = Version::MAJOR;
    return true;
}

// Main load entry point
SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    InitializeLogging();

    logger::info("{} is loading...", Version::NAME);

    // Register for SKSE messages
    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener(MessageHandler)) {
        logger::error("Failed to register messaging listener");
        return false;
    }

    // Cosave serialization (uncomment if your plugin persists data in save games):
    // auto serialization = SKSE::GetSerializationInterface();
    // serialization->SetUniqueID('OHUI');  // 4-char unique ID
    // serialization->SetSaveCallback(MyService::OnGameSaved);
    // serialization->SetLoadCallback(MyService::OnGameLoaded);
    // serialization->SetRevertCallback(MyService::OnRevert);  // Critical for save reload

    // Install hooks (uncomment when ready):
    // if (!MyHooks::Install()) {
    //     // Return true anyway -- don't fail SKSE load, just disable this mod.
    //     // Returning false shows a scary "SKSE fatal error" dialog to the user.
    //     logger::critical("{} DISABLED - hook installation failed", Version::NAME);
    //     return true;
    // }

    logger::info("{} loaded successfully", Version::NAME);
    return true;
}
