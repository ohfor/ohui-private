#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/mcm/MCM2PersistenceManager.h"
#include "MockFileSystem.h"

using namespace ohui;
using namespace ohui::mcm;
using namespace ohui::cosave;
using Catch::Matchers::WithinAbs;

static auto MakeStack() {
    struct Stack {
        test::MockFileSystem fs;
        CosaveManager mgr;
        PersistenceAPI api;
        MCM2PersistenceManager pm;
        Stack() : mgr(fs), api(mgr), pm(api) {}
    };
    return Stack{};
}

static MCM2Definition MakeDef(const std::string& id = "com.test.mod") {
    MCM2Definition def;
    def.id = id;
    def.displayName = "Test Mod";
    def.version = {1, 0, 0};

    MCM2PageDef page;
    page.id = "general";
    MCM2SectionDef section;
    section.id = "main";

    MCM2ControlDef toggle;
    toggle.type = MCM2ControlType::Toggle;
    toggle.id = "enabled";
    toggle.properties = MCM2ToggleProps{true};
    section.controls.push_back(std::move(toggle));

    MCM2ControlDef slider;
    slider.type = MCM2ControlType::Slider;
    slider.id = "strength";
    slider.properties = MCM2SliderProps{0.0f, 100.0f, 1.0f, 50.0f, "{0}%"};
    section.controls.push_back(std::move(slider));

    MCM2ControlDef dropdown;
    dropdown.type = MCM2ControlType::Dropdown;
    dropdown.id = "mode";
    dropdown.properties = MCM2DropdownProps{{"Subtle", "Normal", "Aggressive"}, "Normal"};
    section.controls.push_back(std::move(dropdown));

    MCM2ControlDef keybind;
    keybind.type = MCM2ControlType::KeyBind;
    keybind.id = "key";
    keybind.properties = MCM2KeyBindProps{42};
    section.controls.push_back(std::move(keybind));

    MCM2ControlDef colour;
    colour.type = MCM2ControlType::Colour;
    colour.id = "color";
    colour.properties = MCM2ColourProps{0xFF0000};
    section.controls.push_back(std::move(colour));

    MCM2ControlDef text;
    text.type = MCM2ControlType::Text;
    text.id = "status";
    text.properties = MCM2TextProps{"Ready"};
    section.controls.push_back(std::move(text));

    MCM2ControlDef header;
    header.type = MCM2ControlType::Header;
    header.id = "hdr";
    header.properties = MCM2HeaderProps{};
    section.controls.push_back(std::move(header));

    MCM2ControlDef empty;
    empty.type = MCM2ControlType::Empty;
    empty.properties = MCM2EmptyProps{};
    section.controls.push_back(std::move(empty));

    page.sections.push_back(std::move(section));
    def.pages.push_back(std::move(page));
    return def;
}

// 1. RegisterDefinition succeeds
TEST_CASE("MCM2 Persistence: register succeeds", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    REQUIRE(s.pm.RegisterDefinition(def).has_value());
    CHECK(s.pm.IsRegistered("com.test.mod"));
}

// 2. RegisterDefinition duplicate is no-op
TEST_CASE("MCM2 Persistence: register duplicate no-op", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    REQUIRE(s.pm.RegisterDefinition(def).has_value());
    REQUIRE(s.pm.RegisterDefinition(def).has_value());
    CHECK(s.pm.IsRegistered("com.test.mod"));
}

// 3. ApplyDefaults: toggle default applied
TEST_CASE("MCM2 Persistence: toggle default applied", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetBool("com.test.mod", "enabled") == true);
}

// 4. ApplyDefaults: slider default applied
TEST_CASE("MCM2 Persistence: slider default applied", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK_THAT(static_cast<double>(s.pm.GetFloat("com.test.mod", "strength")),
               WithinAbs(50.0, 0.01));
}

// 5. ApplyDefaults: dropdown default applied (index)
TEST_CASE("MCM2 Persistence: dropdown default index", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetInt("com.test.mod", "mode") == 1); // "Normal" is index 1
}

// 6. ApplyDefaults: keybind default applied
TEST_CASE("MCM2 Persistence: keybind default applied", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetInt("com.test.mod", "key") == 42);
}

// 7. ApplyDefaults: colour default applied
TEST_CASE("MCM2 Persistence: colour default applied", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetInt("com.test.mod", "color") == 0xFF0000);
}

// 8. ApplyDefaults: text default applied
TEST_CASE("MCM2 Persistence: text default applied", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetString("com.test.mod", "status") == "Ready");
}

// 9. ApplyDefaults: header and empty skipped
TEST_CASE("MCM2 Persistence: header and empty skipped", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    // Header and empty should not have persisted values
    auto ns = "mcm2.com.test.mod";
    CHECK(!s.api.Has(ns, "hdr"));
}

// 10. ApplyDefaults: existing value NOT overwritten
TEST_CASE("MCM2 Persistence: defaults don't overwrite existing", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.SetBool("com.test.mod", "enabled", false);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetBool("com.test.mod", "enabled") == false); // NOT overwritten to true
}

// 11. SetBool/GetBool round-trip
TEST_CASE("MCM2 Persistence: SetBool GetBool round-trip", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    REQUIRE(s.pm.SetBool("com.test.mod", "enabled", true).has_value());
    CHECK(s.pm.GetBool("com.test.mod", "enabled") == true);
}

// 12. SetFloat/GetFloat round-trip
TEST_CASE("MCM2 Persistence: SetFloat GetFloat round-trip", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    REQUIRE(s.pm.SetFloat("com.test.mod", "strength", 75.5f).has_value());
    CHECK_THAT(static_cast<double>(s.pm.GetFloat("com.test.mod", "strength")),
               WithinAbs(75.5, 0.01));
}

// 13. SetInt/GetInt round-trip
TEST_CASE("MCM2 Persistence: SetInt GetInt round-trip", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    REQUIRE(s.pm.SetInt("com.test.mod", "key", 99).has_value());
    CHECK(s.pm.GetInt("com.test.mod", "key") == 99);
}

// 14. SetString/GetString round-trip
TEST_CASE("MCM2 Persistence: SetString GetString round-trip", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    REQUIRE(s.pm.SetString("com.test.mod", "status", "Active").has_value());
    CHECK(s.pm.GetString("com.test.mod", "status") == "Active");
}

// 15. Get returns default for missing control
TEST_CASE("MCM2 Persistence: get missing control returns default", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    CHECK(s.pm.GetBool("com.test.mod", "missing", true) == true);
    CHECK(s.pm.GetInt("com.test.mod", "missing", -1) == -1);
    CHECK_THAT(static_cast<double>(s.pm.GetFloat("com.test.mod", "missing", 9.9f)),
               WithinAbs(9.9, 0.01));
    CHECK(s.pm.GetString("com.test.mod", "missing", "fallback") == "fallback");
}

// 16. Get returns default for unregistered MCM ID
TEST_CASE("MCM2 Persistence: get unregistered returns default", "[mcm2-persistence]") {
    auto s = MakeStack();
    CHECK(s.pm.GetBool("unknown", "key", true) == true);
    CHECK(s.pm.GetInt("unknown", "key", 42) == 42);
}

// 17. Set fails for unregistered MCM ID
TEST_CASE("MCM2 Persistence: set unregistered fails", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto result = s.pm.SetBool("unknown", "key", true);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

// 18. GetAllValues returns map of all control values
TEST_CASE("MCM2 Persistence: GetAllValues returns all", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);

    auto values = s.pm.GetAllValues(def);
    CHECK(values.size() >= 6); // 6 controls with persistence (not header/empty)
    CHECK(std::get<bool>(values["enabled"]) == true);
}

// 19. GetAllValues uses defaults for unset controls
TEST_CASE("MCM2 Persistence: GetAllValues uses defaults", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.RegisterDefinition(def);
    // Don't call ApplyDefaults — GetAllValues should still use defaults from def

    auto values = s.pm.GetAllValues(def);
    CHECK(std::get<bool>(values["enabled"]) == true); // default from def
}

// 20. BeginSession increments session counter
TEST_CASE("MCM2 Persistence: BeginSession increments", "[mcm2-persistence]") {
    auto s = MakeStack();
    CHECK(s.pm.CurrentSession() == 0);
    s.pm.BeginSession();
    CHECK(s.pm.CurrentSession() == 1);
    s.pm.BeginSession();
    CHECK(s.pm.CurrentSession() == 2);
}

// 21. TouchLastSeen updates session key
TEST_CASE("MCM2 Persistence: TouchLastSeen stores session", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();
    s.pm.BeginSession();
    s.pm.RegisterDefinition(def);

    auto ns = "mcm2.com.test.mod";
    auto lastSeen = s.api.GetInt(ns, std::string(MCM2PersistenceManager::kLastSeenKey));
    CHECK(lastSeen == 1);
}

// 22. PruneOrphanedEntries: MCM seen this session NOT pruned
TEST_CASE("MCM2 Persistence: recent MCM not pruned", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();

    for (int i = 0; i < 5; ++i) s.pm.BeginSession();
    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);

    auto pruned = s.pm.PruneOrphanedEntries();
    REQUIRE(pruned.has_value());
    CHECK(*pruned == 0);
    CHECK(s.pm.IsRegistered("com.test.mod"));
}

// 23. PruneOrphanedEntries: MCM not seen for 5 sessions IS pruned
TEST_CASE("MCM2 Persistence: old MCM pruned", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();

    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);

    // Advance 5 sessions without re-registering
    for (int i = 0; i < 5; ++i) s.pm.BeginSession();

    auto pruned = s.pm.PruneOrphanedEntries();
    REQUIRE(pruned.has_value());
    CHECK(*pruned == 1);
}

// 24. PruneOrphanedEntries: MCM not seen for 4 sessions NOT pruned
TEST_CASE("MCM2 Persistence: 4 sessions not enough to prune", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();

    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);

    // Advance only 4 sessions
    for (int i = 0; i < 4; ++i) s.pm.BeginSession();

    auto pruned = s.pm.PruneOrphanedEntries();
    REQUIRE(pruned.has_value());
    CHECK(*pruned == 0);
}

// 25. PruneOrphanedEntries: returns count of pruned entries
TEST_CASE("MCM2 Persistence: prune returns count", "[mcm2-persistence]") {
    auto s = MakeStack();

    auto def1 = MakeDef("mod1");
    auto def2 = MakeDef("mod2");

    s.pm.RegisterDefinition(def1);
    s.pm.RegisterDefinition(def2);

    for (int i = 0; i < 5; ++i) s.pm.BeginSession();

    auto pruned = s.pm.PruneOrphanedEntries();
    REQUIRE(pruned.has_value());
    CHECK(*pruned == 2);
}

// 26. Multiple MCMs with different IDs have isolated storage
TEST_CASE("MCM2 Persistence: isolated storage", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def1 = MakeDef("mod1");
    auto def2 = MakeDef("mod2");

    s.pm.RegisterDefinition(def1);
    s.pm.RegisterDefinition(def2);

    s.pm.SetBool("mod1", "enabled", true);
    s.pm.SetBool("mod2", "enabled", false);

    CHECK(s.pm.GetBool("mod1", "enabled") == true);
    CHECK(s.pm.GetBool("mod2", "enabled") == false);
}

// 27. Values survive PersistenceAPI serialize/deserialize round-trip
TEST_CASE("MCM2 Persistence: serialize/deserialize round-trip", "[mcm2-persistence]") {
    auto s = MakeStack();
    auto def = MakeDef();

    s.pm.RegisterDefinition(def);
    s.pm.SetBool("com.test.mod", "enabled", true);
    s.pm.SetFloat("com.test.mod", "strength", 75.0f);
    s.pm.SetInt("com.test.mod", "key", 99);
    s.pm.SetString("com.test.mod", "status", "Active");

    auto bytes = s.api.SerializeAll();
    REQUIRE(!bytes.empty());

    // Create fresh stack and deserialize
    auto s2 = MakeStack();
    REQUIRE(s2.api.DeserializeAll(bytes).has_value());

    CHECK(s2.pm.GetBool("com.test.mod", "enabled") == true);
    CHECK_THAT(static_cast<double>(s2.pm.GetFloat("com.test.mod", "strength")),
               WithinAbs(75.0, 0.01));
    CHECK(s2.pm.GetInt("com.test.mod", "key") == 99);
    CHECK(s2.pm.GetString("com.test.mod", "status") == "Active");
}

// 28. Dropdown default with missing option text falls back to index 0
TEST_CASE("MCM2 Persistence: dropdown missing default falls back to 0", "[mcm2-persistence]") {
    auto s = MakeStack();

    MCM2Definition def;
    def.id = "test";
    MCM2PageDef page;
    page.id = "p";
    MCM2SectionDef section;
    section.id = "s";
    MCM2ControlDef dd;
    dd.type = MCM2ControlType::Dropdown;
    dd.id = "choice";
    dd.properties = MCM2DropdownProps{{"A", "B", "C"}, "Missing"};
    section.controls.push_back(std::move(dd));
    page.sections.push_back(std::move(section));
    def.pages.push_back(std::move(page));

    s.pm.RegisterDefinition(def);
    s.pm.ApplyDefaults(def);
    CHECK(s.pm.GetInt("test", "choice") == 0); // falls back to index 0
}
