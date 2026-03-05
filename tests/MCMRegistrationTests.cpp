#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCMRegistrationEngine.h"
#include "ohui/mcm/MCMTypes.h"

using namespace ohui;
using namespace ohui::mcm;

// --- Helper: register a mod with one page, built and ready ---
static MCMRegistrationEngine MakeReadyEngine(const std::string& modName = "TestMod") {
    MCMRegistrationEngine engine;
    engine.RegisterMod(modName);
    engine.AddPage(modName, "General");
    engine.BeginPageBuild(modName, 0);
    engine.AddToggleOption(modName, "Toggle1", false);
    engine.AddSliderOption(modName, "Slider1", 50.0f, "{0}%");
    engine.AddColorOption(modName, "Color1", 0xFF0000);
    engine.AddKeyMapOption(modName, "Key1", 42);
    engine.EndPageBuild(modName);
    return engine;
}

// 1. RegisterMod succeeds, mod appears in registry
TEST_CASE("RegisterMod succeeds, mod appears in registry", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    auto result = engine.RegisterMod("TestMod");
    REQUIRE(result.has_value());
    CHECK(engine.HasMod("TestMod"));
    CHECK(engine.ModCount() == 1);
}

// 2. RegisterMod duplicate returns DuplicateRegistration
TEST_CASE("RegisterMod duplicate returns DuplicateRegistration", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    auto result = engine.RegisterMod("TestMod");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

// 3. UnregisterMod succeeds, mod removed
TEST_CASE("UnregisterMod succeeds, mod removed", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    auto result = engine.UnregisterMod("TestMod");
    REQUIRE(result.has_value());
    CHECK(!engine.HasMod("TestMod"));
    CHECK(engine.ModCount() == 0);
}

// 4. UnregisterMod unknown returns NotRegistered
TEST_CASE("UnregisterMod unknown returns NotRegistered", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    auto result = engine.UnregisterMod("Unknown");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

// 5. AddPage succeeds, returns page index 0
TEST_CASE("AddPage succeeds, returns page index 0", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    auto result = engine.AddPage("TestMod", "General");
    REQUIRE(result.has_value());
    CHECK(result.value() == 0);
}

// 6. AddPage second page returns index 1
TEST_CASE("AddPage second page returns index 1", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    auto result = engine.AddPage("TestMod", "Advanced");
    REQUIRE(result.has_value());
    CHECK(result.value() == 1);
}

// 7. AddPage to unregistered mod returns NotRegistered
TEST_CASE("AddPage to unregistered mod returns NotRegistered", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    auto result = engine.AddPage("Unknown", "Page");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

// 8. BeginPageBuild transitions state to Building
TEST_CASE("BeginPageBuild transitions state to Building", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    auto result = engine.BeginPageBuild("TestMod", 0);
    REQUIRE(result.has_value());
    auto* mod = engine.GetMod("TestMod");
    REQUIRE(mod != nullptr);
    CHECK(mod->state == MCMModState::Building);
}

// 9. BeginPageBuild on invalid page returns PageNotFound
TEST_CASE("BeginPageBuild on invalid page returns PageNotFound", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    auto result = engine.BeginPageBuild("TestMod", 5);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::PageNotFound);
}

// 10. BeginPageBuild while already Building returns InvalidState
TEST_CASE("BeginPageBuild while already Building returns InvalidState", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.AddPage("TestMod", "Advanced");
    engine.BeginPageBuild("TestMod", 0);
    auto result = engine.BeginPageBuild("TestMod", 1);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidState);
}

// 11. EndPageBuild transitions state to Ready
TEST_CASE("EndPageBuild transitions state to Ready", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.BeginPageBuild("TestMod", 0);
    auto result = engine.EndPageBuild("TestMod");
    REQUIRE(result.has_value());
    auto* mod = engine.GetMod("TestMod");
    REQUIRE(mod != nullptr);
    CHECK(mod->state == MCMModState::Ready);
}

// 12. EndPageBuild when not Building returns InvalidState
TEST_CASE("EndPageBuild when not Building returns InvalidState", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    auto result = engine.EndPageBuild("TestMod");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidState);
}

// 13. AddToggleOption returns encoded option ID (pageIndex * 256 + optionIndex)
TEST_CASE("AddToggleOption returns encoded option ID", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.BeginPageBuild("TestMod", 0);

    auto r1 = engine.AddToggleOption("TestMod", "Option1", true);
    REQUIRE(r1.has_value());
    CHECK(r1.value() == EncodeOptionId(0, 0));  // 0

    auto r2 = engine.AddToggleOption("TestMod", "Option2", false);
    REQUIRE(r2.has_value());
    CHECK(r2.value() == EncodeOptionId(0, 1));  // 1
}

// 14. AddToggleOption outside build returns InvalidState
TEST_CASE("AddToggleOption outside build returns InvalidState", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");

    auto result = engine.AddToggleOption("TestMod", "Opt", false);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidState);
}

// 15. All 8 option types register with correct type and values
TEST_CASE("All 8 option types register with correct type and values", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.BeginPageBuild("TestMod", 0);

    auto toggleId = engine.AddToggleOption("TestMod", "Toggle", true);
    auto sliderId = engine.AddSliderOption("TestMod", "Slider", 75.0f, "{0}%");
    auto menuId = engine.AddMenuOption("TestMod", "Menu", "Choice A");
    auto textId = engine.AddTextOption("TestMod", "Text", "Hello");
    auto colorId = engine.AddColorOption("TestMod", "Color", 0xFF00FF);
    auto keymapId = engine.AddKeyMapOption("TestMod", "KeyMap", 42);
    auto headerId = engine.AddHeaderOption("TestMod", "Header");
    auto emptyId = engine.AddEmptyOption("TestMod");

    engine.EndPageBuild("TestMod");

    auto* toggle = engine.GetOption("TestMod", toggleId.value());
    REQUIRE(toggle != nullptr);
    CHECK(toggle->type == MCMOptionType::Toggle);
    CHECK(toggle->boolValue == true);
    CHECK(toggle->name == "Toggle");

    auto* slider = engine.GetOption("TestMod", sliderId.value());
    REQUIRE(slider != nullptr);
    CHECK(slider->type == MCMOptionType::Slider);
    CHECK(slider->floatValue == 75.0f);
    CHECK(slider->formatString == "{0}%");

    auto* menu = engine.GetOption("TestMod", menuId.value());
    REQUIRE(menu != nullptr);
    CHECK(menu->type == MCMOptionType::Menu);
    CHECK(menu->stringValue == "Choice A");

    auto* text = engine.GetOption("TestMod", textId.value());
    REQUIRE(text != nullptr);
    CHECK(text->type == MCMOptionType::Text);
    CHECK(text->stringValue == "Hello");

    auto* color = engine.GetOption("TestMod", colorId.value());
    REQUIRE(color != nullptr);
    CHECK(color->type == MCMOptionType::Color);
    CHECK(color->intValue == 0xFF00FF);

    auto* keymap = engine.GetOption("TestMod", keymapId.value());
    REQUIRE(keymap != nullptr);
    CHECK(keymap->type == MCMOptionType::KeyMap);
    CHECK(keymap->intValue == 42);

    auto* header = engine.GetOption("TestMod", headerId.value());
    REQUIRE(header != nullptr);
    CHECK(header->type == MCMOptionType::Header);
    CHECK(header->name == "Header");

    auto* empty = engine.GetOption("TestMod", emptyId.value());
    REQUIRE(empty != nullptr);
    CHECK(empty->type == MCMOptionType::Empty);
}

// 16. Option ID encoding -- page 0 option 3 = 3, page 1 option 0 = 256, page 2 option 5 = 517
TEST_CASE("Option ID encoding matches SkyUI convention", "[mcm-registration]") {
    CHECK(EncodeOptionId(0, 3) == 3);
    CHECK(EncodeOptionId(1, 0) == 256);
    CHECK(EncodeOptionId(2, 5) == 517);

    CHECK(DecodePageIndex(3) == 0);
    CHECK(DecodeOptionIndex(3) == 3);

    CHECK(DecodePageIndex(256) == 1);
    CHECK(DecodeOptionIndex(256) == 0);

    CHECK(DecodePageIndex(517) == 2);
    CHECK(DecodeOptionIndex(517) == 5);
}

// 17. Max 128 options per page -- 129th returns SizeLimitExceeded
TEST_CASE("Max 128 options per page -- 129th returns SizeLimitExceeded", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.BeginPageBuild("TestMod", 0);

    for (int i = 0; i < kMaxOptionsPerPage; ++i) {
        auto r = engine.AddEmptyOption("TestMod");
        REQUIRE(r.has_value());
    }

    auto overflow = engine.AddEmptyOption("TestMod");
    REQUIRE(!overflow.has_value());
    CHECK(overflow.error().code == ErrorCode::SizeLimitExceeded);
}

// 18. SetToggleOptionValue updates stored value
TEST_CASE("SetToggleOptionValue updates stored value", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    // Toggle1 was added with value=false, id=0
    auto result = engine.SetToggleOptionValue("TestMod", 0, true);
    REQUIRE(result.has_value());

    auto* opt = engine.GetOption("TestMod", 0);
    REQUIRE(opt != nullptr);
    CHECK(opt->boolValue == true);
}

// 19. SetSliderOptionValue updates value and format string
TEST_CASE("SetSliderOptionValue updates value and format string", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    // Slider1 was added at index 1
    auto result = engine.SetSliderOptionValue("TestMod", 1, 99.0f, "{0} units");
    REQUIRE(result.has_value());

    auto* opt = engine.GetOption("TestMod", 1);
    REQUIRE(opt != nullptr);
    CHECK(opt->floatValue == 99.0f);
    CHECK(opt->formatString == "{0} units");
}

// 20. SetOptionFlags updates flags
TEST_CASE("SetOptionFlags updates flags", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    auto result = engine.SetOptionFlags("TestMod", 0, MCMOptionFlag::Disabled);
    REQUIRE(result.has_value());

    auto* opt = engine.GetOption("TestMod", 0);
    REQUIRE(opt != nullptr);
    CHECK(HasFlag(opt->flags, MCMOptionFlag::Disabled));
}

// 21. SetOptionValue with unknown option returns OptionNotFound
TEST_CASE("SetOptionValue with unknown option returns OptionNotFound", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    auto result = engine.SetToggleOptionValue("TestMod", 9999, true);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::OptionNotFound);
}

// 22. SetOptionValue during build returns InvalidState
TEST_CASE("SetOptionValue during build returns InvalidState", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("TestMod");
    engine.AddPage("TestMod", "General");
    engine.BeginPageBuild("TestMod", 0);
    auto optId = engine.AddToggleOption("TestMod", "Opt", false);
    REQUIRE(optId.has_value());

    auto result = engine.SetToggleOptionValue("TestMod", optId.value(), true);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::InvalidState);
}

// 23. Dialog params -- set and retrieve SliderDialogParams
TEST_CASE("Dialog params -- set and retrieve SliderDialogParams", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    SliderDialogParams params;
    params.minValue = 0.0f;
    params.maxValue = 100.0f;
    params.stepSize = 5.0f;
    params.defaultValue = 50.0f;
    params.currentValue = 75.0f;

    auto result = engine.SetSliderDialogParams("TestMod", params);
    REQUIRE(result.has_value());

    auto* retrieved = engine.GetSliderDialogParams("TestMod");
    REQUIRE(retrieved != nullptr);
    CHECK(retrieved->minValue == 0.0f);
    CHECK(retrieved->maxValue == 100.0f);
    CHECK(retrieved->stepSize == 5.0f);
    CHECK(retrieved->defaultValue == 50.0f);
    CHECK(retrieved->currentValue == 75.0f);
}

// 24. Dialog params -- set and retrieve MenuDialogParams
TEST_CASE("Dialog params -- set and retrieve MenuDialogParams", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    MenuDialogParams params;
    params.menuItems = {"Low", "Medium", "High"};
    params.defaultIndex = 1;
    params.currentIndex = 2;

    auto result = engine.SetMenuDialogParams("TestMod", params);
    REQUIRE(result.has_value());

    auto* retrieved = engine.GetMenuDialogParams("TestMod");
    REQUIRE(retrieved != nullptr);
    CHECK(retrieved->menuItems.size() == 3);
    CHECK(retrieved->menuItems[0] == "Low");
    CHECK(retrieved->defaultIndex == 1);
    CHECK(retrieved->currentIndex == 2);
}

// 25. SetInfoText and retrieve
TEST_CASE("SetInfoText and retrieve", "[mcm-registration]") {
    auto engine = MakeReadyEngine();
    auto result = engine.SetInfoText("TestMod", "This option does something cool");
    REQUIRE(result.has_value());

    auto* info = engine.GetInfoText("TestMod");
    REQUIRE(info != nullptr);
    CHECK(info->text == "This option does something cool");
}

// 26. GetModNames returns all registered mod names alphabetically
TEST_CASE("GetModNames returns all registered mod names alphabetically", "[mcm-registration]") {
    MCMRegistrationEngine engine;
    engine.RegisterMod("Zeta");
    engine.RegisterMod("Alpha");
    engine.RegisterMod("Mango");

    auto names = engine.GetModNames();
    REQUIRE(names.size() == 3);
    CHECK(names[0] == "Alpha");
    CHECK(names[1] == "Mango");
    CHECK(names[2] == "Zeta");
}
