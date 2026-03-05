#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCMCallbackEngine.h"
#include "ohui/mcm/MCMRegistrationEngine.h"

using namespace ohui;
using namespace ohui::mcm;

// Helper: set up a registration engine with one mod, one page, multiple option types
static MCMRegistrationEngine MakeReadyReg() {
    MCMRegistrationEngine reg;
    reg.RegisterMod("TestMod");
    reg.AddPage("TestMod", "General");
    reg.BeginPageBuild("TestMod", 0);
    reg.AddToggleOption("TestMod", "Toggle", false);       // id=0
    reg.AddSliderOption("TestMod", "Slider", 50.0f);       // id=1
    reg.AddMenuOption("TestMod", "Menu", "ChoiceA");        // id=2
    reg.AddColorOption("TestMod", "Color", 0xFF0000);       // id=3
    reg.AddKeyMapOption("TestMod", "KeyMap", 42);           // id=4
    reg.EndPageBuild("TestMod");
    return reg;
}

// 1. ResolveOptionSelect produces correct descriptor type and optionId
TEST_CASE("ResolveOptionSelect produces correct descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveOptionSelect("TestMod", 0);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionSelect);
    CHECK(result.value().optionId == 0);
    CHECK(result.value().modName == "TestMod");
}

// 2. ResolveOptionDefault produces correct descriptor
TEST_CASE("ResolveOptionDefault produces correct descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveOptionDefault("TestMod", 0);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionDefault);
    CHECK(result.value().optionId == 0);
}

// 3. ResolveOptionHighlight produces correct descriptor
TEST_CASE("ResolveOptionHighlight produces correct descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveOptionHighlight("TestMod", 1);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionHighlight);
    CHECK(result.value().optionId == 1);
}

// 4. ResolveSliderOpen produces OnOptionSliderOpen descriptor
TEST_CASE("ResolveSliderOpen produces OnOptionSliderOpen descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveSliderOpen("TestMod", 1);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionSliderOpen);
}

// 5. ResolveSliderAccept produces descriptor with float payload
TEST_CASE("ResolveSliderAccept produces descriptor with float payload", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveSliderAccept("TestMod", 1, 75.0f);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionSliderAccept);
    CHECK(result.value().floatValue == 75.0f);
}

// 6. ResolveMenuOpen produces descriptor
TEST_CASE("ResolveMenuOpen produces descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveMenuOpen("TestMod", 2);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionMenuOpen);
}

// 7. ResolveMenuAccept produces descriptor with index payload
TEST_CASE("ResolveMenuAccept produces descriptor with index payload", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveMenuAccept("TestMod", 2, 3);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionMenuAccept);
    CHECK(result.value().intValue == 3);
}

// 8. ResolveColorOpen produces descriptor
TEST_CASE("ResolveColorOpen produces descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveColorOpen("TestMod", 3);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionColorOpen);
}

// 9. ResolveColorAccept produces descriptor with color payload
TEST_CASE("ResolveColorAccept produces descriptor with color payload", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveColorAccept("TestMod", 3, 0x00FF00);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionColorAccept);
    CHECK(result.value().intValue == 0x00FF00);
}

// 10. ResolveKeyMapChange produces descriptor with keyCode payload
TEST_CASE("ResolveKeyMapChange produces descriptor with keyCode payload", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveKeyMapChange("TestMod", 4, 57);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnOptionKeyMapChange);
    CHECK(result.value().intValue == 57);
}

// 11. ResolveConfigInit produces lifecycle descriptor
TEST_CASE("ResolveConfigInit produces lifecycle descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveConfigInit("TestMod");
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnConfigInit);
    CHECK(result.value().modName == "TestMod");
}

// 12. ResolveConfigOpen produces lifecycle descriptor
TEST_CASE("ResolveConfigOpen produces lifecycle descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveConfigOpen("TestMod");
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnConfigOpen);
}

// 13. ResolveConfigClose produces lifecycle descriptor
TEST_CASE("ResolveConfigClose produces lifecycle descriptor", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveConfigClose("TestMod");
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnConfigClose);
}

// 14. ResolvePageReset produces descriptor with pageIndex
TEST_CASE("ResolvePageReset produces descriptor with pageIndex", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolvePageReset("TestMod", 0);
    REQUIRE(result.has_value());
    CHECK(result.value().type == MCMCallbackType::OnPageReset);
    CHECK(result.value().pageIndex == 0);
}

// 15. Resolve on unknown mod returns NotRegistered
TEST_CASE("Resolve on unknown mod returns NotRegistered", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto r1 = cb.ResolveOptionSelect("Unknown", 0);
    REQUIRE(!r1.has_value());
    CHECK(r1.error().code == ErrorCode::NotRegistered);

    auto r2 = cb.ResolveConfigInit("Unknown");
    REQUIRE(!r2.has_value());
    CHECK(r2.error().code == ErrorCode::NotRegistered);
}

// 16. Resolve on unknown option returns OptionNotFound
TEST_CASE("Resolve on unknown option returns OptionNotFound", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolveOptionSelect("TestMod", 9999);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::OptionNotFound);
}

// 17. ResolvePageReset on unknown page returns PageNotFound
TEST_CASE("ResolvePageReset on unknown page returns PageNotFound", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ResolvePageReset("TestMod", 99);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::PageNotFound);
}

// 18. ApplyToggleChange updates value and records history
TEST_CASE("ApplyToggleChange updates value and records history", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ApplyToggleChange("TestMod", 0, true);
    REQUIRE(result.has_value());
    CHECK(result.value().oldBool == false);
    CHECK(result.value().newBool == true);

    auto* opt = reg.GetOption("TestMod", 0);
    REQUIRE(opt != nullptr);
    CHECK(opt->boolValue == true);
    CHECK(cb.GetChangeHistory().size() == 1);
}

// 19. ApplySliderChange updates value and records history
TEST_CASE("ApplySliderChange updates value and records history", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ApplySliderChange("TestMod", 1, 99.0f);
    REQUIRE(result.has_value());
    CHECK(result.value().oldFloat == 50.0f);
    CHECK(result.value().newFloat == 99.0f);

    auto* opt = reg.GetOption("TestMod", 1);
    REQUIRE(opt != nullptr);
    CHECK(opt->floatValue == 99.0f);
}

// 20. ApplyColorChange updates value and records history
TEST_CASE("ApplyColorChange updates value and records history", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ApplyColorChange("TestMod", 3, 0x00FF00);
    REQUIRE(result.has_value());
    CHECK(result.value().oldInt == 0xFF0000);
    CHECK(result.value().newInt == 0x00FF00);

    auto* opt = reg.GetOption("TestMod", 3);
    REQUIRE(opt != nullptr);
    CHECK(opt->intValue == 0x00FF00);
}

// 21. ApplyKeyMapChange updates value and records history
TEST_CASE("ApplyKeyMapChange updates value and records history", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);
    auto result = cb.ApplyKeyMapChange("TestMod", 4, 57);
    REQUIRE(result.has_value());
    CHECK(result.value().oldInt == 42);
    CHECK(result.value().newInt == 57);

    auto* opt = reg.GetOption("TestMod", 4);
    REQUIRE(opt != nullptr);
    CHECK(opt->intValue == 57);
}

// 22. Change history records before/after values correctly
TEST_CASE("Change history records before/after values correctly", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);

    cb.ApplyToggleChange("TestMod", 0, true);
    cb.ApplySliderChange("TestMod", 1, 80.0f);

    const auto& history = cb.GetChangeHistory();
    REQUIRE(history.size() == 2);

    CHECK(history[0].optionType == MCMOptionType::Toggle);
    CHECK(history[0].oldBool == false);
    CHECK(history[0].newBool == true);

    CHECK(history[1].optionType == MCMOptionType::Slider);
    CHECK(history[1].oldFloat == 50.0f);
    CHECK(history[1].newFloat == 80.0f);
}

// 23. ClearChangeHistory empties the log
TEST_CASE("ClearChangeHistory empties the log", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);

    cb.ApplyToggleChange("TestMod", 0, true);
    REQUIRE(cb.GetChangeHistory().size() == 1);

    cb.ClearChangeHistory();
    CHECK(cb.GetChangeHistory().empty());
}

// 24. Multiple changes accumulate in order
TEST_CASE("Multiple changes accumulate in order", "[mcm-callback]") {
    auto reg = MakeReadyReg();
    MCMCallbackEngine cb(reg);

    cb.ApplyToggleChange("TestMod", 0, true);
    cb.ApplySliderChange("TestMod", 1, 60.0f);
    cb.ApplyColorChange("TestMod", 3, 0x0000FF);
    cb.ApplyKeyMapChange("TestMod", 4, 100);

    const auto& history = cb.GetChangeHistory();
    REQUIRE(history.size() == 4);
    CHECK(history[0].optionId == 0);
    CHECK(history[1].optionId == 1);
    CHECK(history[2].optionId == 3);
    CHECK(history[3].optionId == 4);
}
