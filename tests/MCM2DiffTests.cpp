#include <catch2/catch_test_macros.hpp>

#include "ohui/mcm/MCM2DefinitionDiff.h"

using namespace ohui::mcm;

// Helper to build minimal defs
static MCM2Definition MakeBaseDef() {
    MCM2Definition def;
    def.id = "com.test.mod";
    def.displayName = "Test Mod";
    def.version = {1, 0, 0};

    MCM2PageDef page;
    page.id = "general";
    MCM2SectionDef section;
    section.id = "main";

    MCM2ControlDef toggle;
    toggle.type = MCM2ControlType::Toggle;
    toggle.id = "enabled";
    toggle.label = "Enabled";
    toggle.description = "Enable feature";
    toggle.properties = MCM2ToggleProps{true};
    section.controls.push_back(std::move(toggle));

    MCM2ControlDef slider;
    slider.type = MCM2ControlType::Slider;
    slider.id = "strength";
    slider.label = "Strength";
    slider.properties = MCM2SliderProps{0.0f, 100.0f, 1.0f, 50.0f, ""};
    section.controls.push_back(std::move(slider));

    page.sections.push_back(std::move(section));
    def.pages.push_back(std::move(page));
    return def;
}

// 1. Identical definitions: no changes
TEST_CASE("Diff: identical defs have no changes", "[mcm2-diff]") {
    auto def = MakeBaseDef();
    auto delta = ComputeDiff(def, def);
    CHECK(!delta.HasAnyChanges());
}

// 2. Added page detected
TEST_CASE("Diff: added page detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    MCM2PageDef newPage;
    newPage.id = "advanced";
    newDef.pages.push_back(std::move(newPage));

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.HasAnyChanges());
    bool found = false;
    for (const auto& c : delta.pageChanges) {
        if (c.type == DiffChangeType::Added && c.pageId == "advanced") found = true;
    }
    CHECK(found);
}

// 3. Removed page detected
TEST_CASE("Diff: removed page detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    MCM2PageDef extraPage;
    extraPage.id = "extra";
    oldDef.pages.push_back(std::move(extraPage));
    auto newDef = MakeBaseDef();

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.pageChanges) {
        if (c.type == DiffChangeType::Removed && c.pageId == "extra") found = true;
    }
    CHECK(found);
}

// 4. Added section detected
TEST_CASE("Diff: added section detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    MCM2SectionDef newSection;
    newSection.id = "extra";
    newDef.pages[0].sections.push_back(std::move(newSection));

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.sectionChanges) {
        if (c.type == DiffChangeType::Added && c.sectionId == "extra") found = true;
    }
    CHECK(found);
}

// 5. Removed section detected
TEST_CASE("Diff: removed section detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    MCM2SectionDef extraSec;
    extraSec.id = "extra";
    oldDef.pages[0].sections.push_back(std::move(extraSec));
    auto newDef = MakeBaseDef();

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.sectionChanges) {
        if (c.type == DiffChangeType::Removed && c.sectionId == "extra") found = true;
    }
    CHECK(found);
}

// 6. Added control detected
TEST_CASE("Diff: added control detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    MCM2ControlDef newCtrl;
    newCtrl.type = MCM2ControlType::Text;
    newCtrl.id = "newText";
    newCtrl.properties = MCM2TextProps{};
    newDef.pages[0].sections[0].controls.push_back(std::move(newCtrl));

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Added && c.controlId == "newText") found = true;
    }
    CHECK(found);
}

// 7. Removed control detected
TEST_CASE("Diff: removed control detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    // Remove the slider from new
    newDef.pages[0].sections[0].controls.erase(
        newDef.pages[0].sections[0].controls.begin() + 1);

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Removed && c.controlId == "strength") found = true;
    }
    CHECK(found);
}

// 8. Modified control label detected
TEST_CASE("Diff: modified label detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.pages[0].sections[0].controls[0].label = "Enable Feature V2";

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Modified && c.controlId == "enabled" &&
            c.fieldName == "label") found = true;
    }
    CHECK(found);
}

// 9. Modified control description detected
TEST_CASE("Diff: modified description detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.pages[0].sections[0].controls[0].description = "New description";

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Modified && c.fieldName == "description") found = true;
    }
    CHECK(found);
}

// 10. Modified slider min/max detected
TEST_CASE("Diff: modified slider properties", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    auto& sliderProps = std::get<MCM2SliderProps>(
        newDef.pages[0].sections[0].controls[1].properties);
    sliderProps.maxValue = 200.0f;

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Modified && c.controlId == "strength" &&
            c.fieldName == "properties") found = true;
    }
    CHECK(found);
}

// 11. Modified dropdown options detected
TEST_CASE("Diff: modified dropdown options", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    MCM2ControlDef dd;
    dd.type = MCM2ControlType::Dropdown;
    dd.id = "mode";
    dd.properties = MCM2DropdownProps{{"A", "B"}, "A"};
    oldDef.pages[0].sections[0].controls.push_back(dd);

    auto newDef = oldDef;
    auto& newProps = std::get<MCM2DropdownProps>(
        newDef.pages[0].sections[0].controls[2].properties);
    newProps.options = {"A", "B", "C"};

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Modified && c.controlId == "mode") found = true;
    }
    CHECK(found);
}

// 12. Modified control condition detected
TEST_CASE("Diff: modified condition detected", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.pages[0].sections[0].controls[1].conditionExpr = "enabled == true";

    auto delta = ComputeDiff(oldDef, newDef);
    bool found = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Modified && c.fieldName == "condition") found = true;
    }
    CHECK(found);
}

// 13. Reordered controls (same IDs): not destructive
TEST_CASE("Diff: reordered controls not destructive", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    // Swap the two controls
    std::swap(newDef.pages[0].sections[0].controls[0],
              newDef.pages[0].sections[0].controls[1]);

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(!delta.HasDestructiveChanges());
}

// 14. Reordered pages: not destructive
TEST_CASE("Diff: reordered pages not destructive", "[mcm2-diff]") {
    auto def = MakeBaseDef();
    MCM2PageDef page2;
    page2.id = "advanced";
    def.pages.push_back(page2);

    auto newDef = def;
    std::swap(newDef.pages[0], newDef.pages[1]);

    auto delta = ComputeDiff(def, newDef);
    CHECK(!delta.HasDestructiveChanges());
}

// 15. Changed control ID: flagged as destructive
TEST_CASE("Diff: changed control ID is destructive", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.pages[0].sections[0].controls[0].id = "isEnabled";

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.HasDestructiveChanges());
    bool foundDestructive = false;
    for (const auto& c : delta.controlChanges) {
        if (c.type == DiffChangeType::Destructive) foundDestructive = true;
    }
    CHECK(foundDestructive);
}

// 16. Changed MCM ID: flagged as destructive
TEST_CASE("Diff: changed MCM ID is destructive", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.id = "com.test.newmod";

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.mcmIdChanged);
    CHECK(delta.HasDestructiveChanges());
}

// 17. Changed displayName: detected, not destructive
TEST_CASE("Diff: changed displayName not destructive", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.displayName = "New Name";

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.displayNameChanged);
    CHECK(!delta.HasDestructiveChanges());
}

// 18. Changed version: detected, not destructive
TEST_CASE("Diff: changed version not destructive", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();
    newDef.version = {2, 0, 0};

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.versionChanged);
    CHECK(!delta.HasDestructiveChanges());
}

// 19. HasDestructiveChanges correct
TEST_CASE("Diff: HasDestructiveChanges correct", "[mcm2-diff]") {
    MCM2DefinitionDelta delta;
    CHECK(!delta.HasDestructiveChanges());
    delta.mcmIdChanged = true;
    CHECK(delta.HasDestructiveChanges());
}

// 20. HasAnyChanges false for identical
TEST_CASE("Diff: HasAnyChanges false for identical", "[mcm2-diff]") {
    MCM2DefinitionDelta delta;
    CHECK(!delta.HasAnyChanges());
}

// 21. DestructiveChangeCount correct
TEST_CASE("Diff: DestructiveChangeCount correct", "[mcm2-diff]") {
    MCM2DefinitionDelta delta;
    delta.mcmIdChanged = true;
    delta.controlChanges.push_back({DiffChangeType::Destructive, "", "", "a", "", "", "", ""});
    delta.controlChanges.push_back({DiffChangeType::Modified, "", "", "b", "", "", "", ""});
    CHECK(delta.DestructiveChangeCount() == 2); // mcmId + 1 destructive control
}

// 22. Complex delta: mixed adds, removes, modifies, destructive
TEST_CASE("Diff: complex mixed delta", "[mcm2-diff]") {
    auto oldDef = MakeBaseDef();
    auto newDef = MakeBaseDef();

    // Modify label
    newDef.pages[0].sections[0].controls[0].label = "Changed Label";

    // Add a new control
    MCM2ControlDef newCtrl;
    newCtrl.type = MCM2ControlType::Text;
    newCtrl.id = "added";
    newCtrl.properties = MCM2TextProps{};
    newDef.pages[0].sections[0].controls.push_back(std::move(newCtrl));

    // Add a new page
    MCM2PageDef newPage;
    newPage.id = "newPage";
    newDef.pages.push_back(std::move(newPage));

    auto delta = ComputeDiff(oldDef, newDef);
    CHECK(delta.HasAnyChanges());
    CHECK(!delta.controlChanges.empty());
    CHECK(!delta.pageChanges.empty());
}
