#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCM2HotReloadEngine.h"
#include "MockFileSystem.h"

using namespace ohui;
using namespace ohui::mcm;
using namespace ohui::cosave;

static const char* kBaseDef = R"(
mcm "com.test.mod" {
    displayName: "Test Mod"
    version: "1.0.0"
    page "general" {
        displayName: "General"
        section "main" {
            displayName: "Main"
            toggle "enabled" {
                label: "Enabled"
                default: true
            }
            slider "strength" {
                label: "Strength"
                min: 0.0
                max: 100.0
                step: 1.0
                default: 50.0
            }
        }
    }
}
)";

struct TestStack {
    test::MockFileSystem fs;
    CosaveManager mgr;
    PersistenceAPI api;
    MCM2DefinitionParser parser;
    MCM2ConditionEngine conditions;
    MCM2PersistenceManager persistence;
    MCM2HotReloadEngine engine;

    TestStack()
        : mgr(fs), api(mgr), persistence(api),
          engine(fs, parser, conditions, persistence) {}

    void SetupFile(const std::string& path, const std::string& content,
                   std::filesystem::file_time_type time = std::filesystem::file_time_type::clock::now()) {
        std::vector<uint8_t> data(content.begin(), content.end());
        fs.files[path] = data;
        fs.modTimes[path] = time;
    }

    void UpdateFile(const std::string& path, const std::string& content) {
        std::vector<uint8_t> data(content.begin(), content.end());
        fs.files[path] = data;
        fs.modTimes[path] = std::filesystem::file_time_type::clock::now() + std::chrono::seconds(1);
    }
};

// 1. WatchFile succeeds for existing file
TEST_CASE("HotReload: WatchFile succeeds", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    REQUIRE(s.engine.WatchFile("test.mcm").has_value());
    CHECK(s.engine.WatchedFileCount() == 1);
    CHECK(s.engine.IsWatching("test.mcm"));
}

// 2. WatchFile fails for non-existent file
TEST_CASE("HotReload: WatchFile fails for missing file", "[mcm2-hotreload]") {
    TestStack s;
    auto result = s.engine.WatchFile("missing.mcm");
    CHECK(!result.has_value());
    CHECK(result.error().code == ErrorCode::FileNotFound);
}

// 3. UnwatchFile removes watch
TEST_CASE("HotReload: UnwatchFile removes", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");
    REQUIRE(s.engine.UnwatchFile("test.mcm").has_value());
    CHECK(s.engine.WatchedFileCount() == 0);
}

// 4. UnwatchAll clears all
TEST_CASE("HotReload: UnwatchAll clears", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("a.mcm", kBaseDef);
    s.SetupFile("b.mcm", kBaseDef);
    s.engine.WatchFile("a.mcm");
    s.engine.WatchFile("b.mcm");
    s.engine.UnwatchAll();
    CHECK(s.engine.WatchedFileCount() == 0);
}

// 5. CheckForChanges: unchanged timestamp → no reload
TEST_CASE("HotReload: unchanged timestamp no reload", "[mcm2-hotreload]") {
    TestStack s;
    auto time = std::filesystem::file_time_type::clock::now();
    s.SetupFile("test.mcm", kBaseDef, time);
    s.engine.WatchFile("test.mcm");
    s.engine.SetEnabled(true);

    bool callbackFired = false;
    s.engine.SetReloadCallback([&](const std::string&, const MCM2DefinitionDelta&) {
        callbackFired = true;
    });

    s.engine.CheckForChanges();
    CHECK(!callbackFired);
}

// 6. CheckForChanges: changed timestamp → triggers reload
TEST_CASE("HotReload: changed timestamp triggers reload", "[mcm2-hotreload]") {
    TestStack s;
    auto time1 = std::filesystem::file_time_type::clock::now();
    s.SetupFile("test.mcm", kBaseDef, time1);
    s.engine.WatchFile("test.mcm");
    s.engine.SetEnabled(true);

    bool callbackFired = false;
    s.engine.SetReloadCallback([&](const std::string&, const MCM2DefinitionDelta&) {
        callbackFired = true;
    });

    // Change timestamp
    s.fs.modTimes["test.mcm"] = time1 + std::chrono::seconds(10);
    s.engine.CheckForChanges();
    CHECK(callbackFired);
}

// 7. CheckForChanges when disabled: no reload
TEST_CASE("HotReload: disabled no reload", "[mcm2-hotreload]") {
    TestStack s;
    auto time1 = std::filesystem::file_time_type::clock::now();
    s.SetupFile("test.mcm", kBaseDef, time1);
    s.engine.WatchFile("test.mcm");
    // Not enabled

    bool callbackFired = false;
    s.engine.SetReloadCallback([&](const std::string&, const MCM2DefinitionDelta&) {
        callbackFired = true;
    });

    s.fs.modTimes["test.mcm"] = time1 + std::chrono::seconds(10);
    s.engine.CheckForChanges();
    CHECK(!callbackFired);
}

// 8. SetEnabled/IsEnabled toggle
TEST_CASE("HotReload: SetEnabled IsEnabled", "[mcm2-hotreload]") {
    TestStack s;
    CHECK(!s.engine.IsEnabled());
    s.engine.SetEnabled(true);
    CHECK(s.engine.IsEnabled());
    s.engine.SetEnabled(false);
    CHECK(!s.engine.IsEnabled());
}

// 9. Reload with parse error: existing definition remains
TEST_CASE("HotReload: parse error keeps existing def", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    // Replace with broken content
    s.UpdateFile("test.mcm", "this is not valid mcm");
    auto result = s.engine.ReloadFile("test.mcm");
    CHECK(!result.has_value());

    // Original definition should still be there
    auto* watched = s.engine.GetWatchedFile("test.mcm");
    REQUIRE(watched != nullptr);
    CHECK(watched->currentDefinition.id == "com.test.mod");
}

// 10. Reload with valid change: new definition replaces old
TEST_CASE("HotReload: valid change replaces definition", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* newDef = R"(
        mcm "com.test.mod" {
            displayName: "Updated Mod"
            version: "2.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", newDef);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());

    auto* watched = s.engine.GetWatchedFile("test.mcm");
    REQUIRE(watched != nullptr);
    CHECK(watched->currentDefinition.displayName == "Updated Mod");
}

// 11. Reload callback fires with correct delta
TEST_CASE("HotReload: callback fires with delta", "[mcm2-hotreload]") {
    TestStack s;
    auto time1 = std::filesystem::file_time_type::clock::now();
    s.SetupFile("test.mcm", kBaseDef, time1);
    s.engine.WatchFile("test.mcm");
    s.engine.SetEnabled(true);

    std::string callbackMcmId;
    bool hasChanges = false;
    s.engine.SetReloadCallback([&](const std::string& mcmId, const MCM2DefinitionDelta& delta) {
        callbackMcmId = mcmId;
        hasChanges = delta.HasAnyChanges();
    });

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Changed"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                    slider "strength" { min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                }
            }
        }
    )";
    std::vector<uint8_t> data(updated, updated + strlen(updated));
    s.fs.files["test.mcm"] = data;
    s.fs.modTimes["test.mcm"] = time1 + std::chrono::seconds(5);

    s.engine.CheckForChanges();
    CHECK(callbackMcmId == "com.test.mod");
    CHECK(hasChanges);
}

// 12. Added control appears after reload
TEST_CASE("HotReload: added control after reload", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                    slider "strength" { min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                    text "newField" { label: "New" default: "hi" }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());

    bool found = false;
    for (const auto& c : result->controlChanges) {
        if (c.type == DiffChangeType::Added && c.controlId == "newField") found = true;
    }
    CHECK(found);
}

// 13. Removed control gone after reload
TEST_CASE("HotReload: removed control after reload", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());

    bool found = false;
    for (const auto& c : result->controlChanges) {
        if (c.type == DiffChangeType::Removed && c.controlId == "strength") found = true;
    }
    CHECK(found);
}

// 14. Modified label updates after reload
TEST_CASE("HotReload: modified label after reload", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { label: "New Label" default: true }
                    slider "strength" { label: "Strength" min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());

    bool found = false;
    for (const auto& c : result->controlChanges) {
        if (c.type == DiffChangeType::Modified && c.controlId == "enabled" &&
            c.fieldName == "label") found = true;
    }
    CHECK(found);
}

// 15. Destructive change (ID rename): logged
TEST_CASE("HotReload: destructive ID rename", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "isEnabled" { label: "Enabled" default: true }
                    slider "strength" { label: "Strength" min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());
    CHECK(result->HasDestructiveChanges());
}

// 16. MCM ID change: flagged destructive
TEST_CASE("HotReload: MCM ID change is destructive", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.newmod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                    slider "strength" { min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());
    CHECK(result->mcmIdChanged);
    CHECK(result->HasDestructiveChanges());
}

// 17. ReloadFile manual reload works
TEST_CASE("HotReload: manual ReloadFile works", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Manual Reload"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                    slider "strength" { min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    auto result = s.engine.ReloadFile("test.mcm");
    REQUIRE(result.has_value());
    CHECK(result->displayNameChanged);
}

// 18. Multiple watched files: only changed file reloaded
TEST_CASE("HotReload: only changed file reloaded", "[mcm2-hotreload]") {
    TestStack s;
    auto time1 = std::filesystem::file_time_type::clock::now();

    const char* defA = R"(mcm "modA" { displayName: "A" version: "1.0.0" })";
    const char* defB = R"(mcm "modB" { displayName: "B" version: "1.0.0" })";

    s.SetupFile("a.mcm", defA, time1);
    s.SetupFile("b.mcm", defB, time1);
    s.engine.WatchFile("a.mcm");
    s.engine.WatchFile("b.mcm");
    s.engine.SetEnabled(true);

    std::vector<std::string> reloadedIds;
    s.engine.SetReloadCallback([&](const std::string& mcmId, const MCM2DefinitionDelta&) {
        reloadedIds.push_back(mcmId);
    });

    // Only change file A
    const char* updatedA = R"(mcm "modA" { displayName: "A Updated" version: "1.0.0" })";
    std::vector<uint8_t> data(updatedA, updatedA + strlen(updatedA));
    s.fs.files["a.mcm"] = data;
    s.fs.modTimes["a.mcm"] = time1 + std::chrono::seconds(5);

    s.engine.CheckForChanges();
    REQUIRE(reloadedIds.size() == 1);
    CHECK(reloadedIds[0] == "modA");
}

// 19. Conditions recompiled after reload
TEST_CASE("HotReload: conditions recompiled after reload", "[mcm2-hotreload]") {
    TestStack s;
    const char* def1 = R"(
        mcm "com.test.mod" {
            displayName: "Test"
            version: "1.0.0"
            page "p" {
                section "s" {
                    toggle "gate" { default: true }
                    slider "val" {
                        min: 0.0 max: 100.0 step: 1.0 default: 50.0
                        condition: "gate == true"
                    }
                }
            }
        }
    )";
    s.SetupFile("test.mcm", def1);
    s.engine.WatchFile("test.mcm");

    // After initial load, condition should be compiled
    CHECK(s.conditions.HasCondition("val"));

    // Reload with no condition on val
    const char* def2 = R"(
        mcm "com.test.mod" {
            displayName: "Test"
            version: "1.0.0"
            page "p" {
                section "s" {
                    toggle "gate" { default: true }
                    slider "val" {
                        min: 0.0 max: 100.0 step: 1.0 default: 50.0
                    }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", def2);
    s.engine.ReloadFile("test.mcm");

    // Condition should be gone
    CHECK(!s.conditions.HasCondition("val"));
}

// 20. Persistence defaults applied for newly added controls
TEST_CASE("HotReload: defaults applied for new controls", "[mcm2-hotreload]") {
    TestStack s;
    s.SetupFile("test.mcm", kBaseDef);
    s.engine.WatchFile("test.mcm");

    const char* updated = R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
            page "general" {
                section "main" {
                    toggle "enabled" { default: true }
                    slider "strength" { min: 0.0 max: 100.0 step: 1.0 default: 50.0 }
                    toggle "newToggle" { default: true }
                }
            }
        }
    )";
    s.UpdateFile("test.mcm", updated);
    s.engine.ReloadFile("test.mcm");

    // New toggle should have its default persisted
    CHECK(s.persistence.GetBool("com.test.mod", "newToggle") == true);
}
