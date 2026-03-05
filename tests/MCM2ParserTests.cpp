#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/mcm/MCM2DefinitionParser.h"

using namespace ohui;
using namespace ohui::mcm;
using Catch::Matchers::WithinAbs;

static MCM2ParseResult ParseOk(std::string_view input, std::string_view filename = "test.mcm") {
    MCM2DefinitionParser parser;
    auto result = parser.Parse(input, filename);
    REQUIRE(result.has_value());
    return std::move(*result);
}

static MCM2ParseResult ParseAny(std::string_view input, std::string_view filename = "test.mcm") {
    MCM2DefinitionParser parser;
    auto result = parser.Parse(input, filename);
    REQUIRE(result.has_value());
    return std::move(*result);
}

// 1. Empty input returns error
TEST_CASE("Empty input returns error", "[mcm2-parser]") {
    MCM2DefinitionParser parser;
    auto result = parser.Parse("", "test.mcm");
    REQUIRE(result.has_value());
    CHECK(result->hasErrors);
    CHECK(!result->diagnostics.empty());
}

// 2. Minimal valid definition
TEST_CASE("Minimal valid definition", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "com.test.mod" {
            displayName: "Test Mod"
            version: "1.0.0"
        }
    )");
    CHECK(!r.hasErrors);
    CHECK(r.definition.id == "com.test.mod");
    CHECK(r.definition.displayName == "Test Mod");
    CHECK(r.definition.pages.empty());
}

// 3. Version parsing: "1.0.0" → {1, 0, 0}
TEST_CASE("Version parsing major.minor.patch", "[mcm2-parser]") {
    auto r = ParseOk(R"(mcm "test" { version: "1.0.0" })");
    CHECK(r.definition.version.major == 1);
    CHECK(r.definition.version.minor == 0);
    CHECK(r.definition.version.patch == 0);
}

// 4. Version parsing: "2.3" → {2, 3, 0}
TEST_CASE("Version parsing major.minor", "[mcm2-parser]") {
    auto r = ParseOk(R"(mcm "test" { version: "2.3" })");
    CHECK(r.definition.version.major == 2);
    CHECK(r.definition.version.minor == 3);
    CHECK(r.definition.version.patch == 0);
}

// 5. Single page with no sections
TEST_CASE("Single page with no sections", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "general" {
                displayName: "General"
            }
        }
    )");
    CHECK(!r.hasErrors);
    REQUIRE(r.definition.pages.size() == 1);
    CHECK(r.definition.pages[0].id == "general");
    CHECK(r.definition.pages[0].displayName == "General");
    CHECK(r.definition.pages[0].sections.empty());
}

// 6. Single page with single section with no controls
TEST_CASE("Single page with single section no controls", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p1" {
                section "s1" {
                    displayName: "Section One"
                }
            }
        }
    )");
    REQUIRE(r.definition.pages.size() == 1);
    REQUIRE(r.definition.pages[0].sections.size() == 1);
    CHECK(r.definition.pages[0].sections[0].id == "s1");
    CHECK(r.definition.pages[0].sections[0].displayName == "Section One");
    CHECK(r.definition.pages[0].sections[0].controls.empty());
}

// 7. Toggle control — all fields populated
TEST_CASE("Toggle control all fields", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle "enableFeature" {
                        label: "Enable Feature"
                        description: "Enables the main feature."
                        default: true
                        onChange: "OnFeatureToggled"
                        condition: "someOther == true"
                    }
                }
            }
        }
    )");
    REQUIRE(r.definition.pages[0].sections[0].controls.size() == 1);
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Toggle);
    CHECK(c.id == "enableFeature");
    CHECK(c.label == "Enable Feature");
    CHECK(c.description == "Enables the main feature.");
    CHECK(c.onChange == "OnFeatureToggled");
    CHECK(c.conditionExpr == "someOther == true");
    auto& props = std::get<MCM2ToggleProps>(c.properties);
    CHECK(props.defaultValue == true);
}

// 8. Toggle control — minimal
TEST_CASE("Toggle control minimal", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle "feat" {
                        default: false
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Toggle);
    CHECK(c.id == "feat");
    CHECK(c.label.empty());
    auto& props = std::get<MCM2ToggleProps>(c.properties);
    CHECK(props.defaultValue == false);
}

// 9. Slider control — all fields
TEST_CASE("Slider control all fields", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    slider "strength" {
                        label: "Strength"
                        description: "How strong."
                        min: 0.0
                        max: 100.0
                        step: 1.0
                        default: 50.0
                        format: "{0}%"
                        onChange: "OnStrength"
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Slider);
    auto& props = std::get<MCM2SliderProps>(c.properties);
    CHECK_THAT(static_cast<double>(props.minValue), WithinAbs(0.0, 0.01));
    CHECK_THAT(static_cast<double>(props.maxValue), WithinAbs(100.0, 0.01));
    CHECK_THAT(static_cast<double>(props.stepSize), WithinAbs(1.0, 0.01));
    CHECK_THAT(static_cast<double>(props.defaultValue), WithinAbs(50.0, 0.01));
    CHECK(props.formatString == "{0}%");
    CHECK(c.onChange == "OnStrength");
}

// 10. Dropdown control — options list
TEST_CASE("Dropdown control options list", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    dropdown "mode" {
                        label: "Mode"
                        options: ["Subtle", "Normal", "Aggressive"]
                        default: "Normal"
                        onChange: "OnMode"
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Dropdown);
    auto& props = std::get<MCM2DropdownProps>(c.properties);
    REQUIRE(props.options.size() == 3);
    CHECK(props.options[0] == "Subtle");
    CHECK(props.options[1] == "Normal");
    CHECK(props.options[2] == "Aggressive");
    CHECK(props.defaultValue == "Normal");
}

// 11. KeyBind control — default keycode
TEST_CASE("KeyBind control default keycode", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    keybind "activationKey" {
                        label: "Activation Key"
                        default: 42
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::KeyBind);
    auto& props = std::get<MCM2KeyBindProps>(c.properties);
    CHECK(props.defaultValue == 42);
}

// 12. Colour control — hex default
TEST_CASE("Colour control hex default", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    colour "highlight" {
                        label: "Highlight"
                        default: 0xFF0000
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Colour);
    auto& props = std::get<MCM2ColourProps>(c.properties);
    CHECK(props.defaultValue == 0xFF0000);
}

// 13. Text control — string default
TEST_CASE("Text control string default", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    text "status" {
                        label: "Status"
                        default: "Ready"
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Text);
    auto& props = std::get<MCM2TextProps>(c.properties);
    CHECK(props.defaultValue == "Ready");
}

// 14. Header control — label only
TEST_CASE("Header control label only", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    header "miscHeader" {
                        label: "Miscellaneous"
                    }
                }
            }
        }
    )");
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Header);
    CHECK(c.label == "Miscellaneous");
    CHECK(std::holds_alternative<MCM2HeaderProps>(c.properties));
}

// 15. Empty control — empty block
TEST_CASE("Empty control empty block", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    empty {}
                }
            }
        }
    )");
    REQUIRE(r.definition.pages[0].sections[0].controls.size() == 1);
    auto& c = r.definition.pages[0].sections[0].controls[0];
    CHECK(c.type == MCM2ControlType::Empty);
    CHECK(std::holds_alternative<MCM2EmptyProps>(c.properties));
}

// 16. Control with condition expression
TEST_CASE("Control with condition expression stored as raw string", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    slider "str" {
                        condition: "enableFeature == true"
                        default: 0.0
                    }
                }
            }
        }
    )");
    CHECK(r.definition.pages[0].sections[0].controls[0].conditionExpr == "enableFeature == true");
}

// 17. Control with onChange callback name
TEST_CASE("Control with onChange callback", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle "t" {
                        onChange: "MyCallback"
                        default: false
                    }
                }
            }
        }
    )");
    CHECK(r.definition.pages[0].sections[0].controls[0].onChange == "MyCallback");
}

// 18. Multiple pages with multiple sections
TEST_CASE("Multiple pages with multiple sections", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "general" {
                displayName: "General"
                section "s1" { displayName: "S1" }
                section "s2" { displayName: "S2" }
            }
            page "advanced" {
                displayName: "Advanced"
                section "s3" { displayName: "S3" }
            }
        }
    )");
    REQUIRE(r.definition.pages.size() == 2);
    CHECK(r.definition.pages[0].sections.size() == 2);
    CHECK(r.definition.pages[1].sections.size() == 1);
    CHECK(r.definition.pages[0].id == "general");
    CHECK(r.definition.pages[1].id == "advanced");
}

// 19. Multiple controls in one section
TEST_CASE("Multiple controls in one section", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle "a" { default: true }
                    toggle "b" { default: false }
                    slider "c" { default: 5.0 }
                }
            }
        }
    )");
    REQUIRE(r.definition.pages[0].sections[0].controls.size() == 3);
    CHECK(r.definition.pages[0].sections[0].controls[0].id == "a");
    CHECK(r.definition.pages[0].sections[0].controls[1].id == "b");
    CHECK(r.definition.pages[0].sections[0].controls[2].id == "c");
}

// 20. Section with collapsed: true
TEST_CASE("Section collapsed true", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    collapsed: true
                }
            }
        }
    )");
    CHECK(r.definition.pages[0].sections[0].collapsed == true);
}

// 21. Unknown fields ignored
TEST_CASE("Unknown fields ignored", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            futureField: "value"
            page "p" {
                section "s" {
                    toggle "t" {
                        label: "Test"
                        default: true
                        unknownProp: "ignored"
                    }
                }
            }
        }
    )");
    CHECK(!r.hasErrors);
    CHECK(r.definition.pages[0].sections[0].controls[0].label == "Test");
}

// 22. Unknown control type — error diagnostic, parse continues
TEST_CASE("Unknown control type error and recovery", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    unknownWidget "bad" {
                        label: "Bad"
                    }
                    toggle "good" {
                        default: true
                    }
                }
            }
        }
    )");
    CHECK(r.hasErrors);
    // The valid toggle should still be parsed
    bool foundToggle = false;
    for (auto& c : r.definition.pages[0].sections[0].controls) {
        if (c.id == "good" && c.type == MCM2ControlType::Toggle) foundToggle = true;
    }
    CHECK(foundToggle);
}

// 23. Syntax error: missing opening brace
TEST_CASE("Syntax error missing opening brace", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test"
            displayName: "Test"
        }
    )");
    CHECK(r.hasErrors);
    bool hasFileInfo = false;
    for (auto& d : r.diagnostics) {
        if (d.location.line > 0 && d.location.column > 0) hasFileInfo = true;
    }
    CHECK(hasFileInfo);
}

// 24. Syntax error: missing closing brace
TEST_CASE("Syntax error missing closing brace", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            displayName: "Test"
    )");
    CHECK(r.hasErrors);
}

// 25. Syntax error: missing MCM ID string
TEST_CASE("Syntax error missing MCM ID", "[mcm2-parser]") {
    auto r = ParseAny(R"(mcm { displayName: "Test" })");
    CHECK(r.hasErrors);
}

// 26. Missing control ID — error, recovery continues
TEST_CASE("Missing control ID error recovery", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle {
                        default: true
                    }
                    toggle "valid" {
                        default: false
                    }
                }
            }
        }
    )");
    // Should have a diagnostic about missing ID
    CHECK(r.hasErrors);
    // Second toggle should still be parsed
    bool foundValid = false;
    for (auto& c : r.definition.pages[0].sections[0].controls) {
        if (c.id == "valid") foundValid = true;
    }
    CHECK(foundValid);
}

// 27. Error recovery: bad control followed by valid control
TEST_CASE("Error recovery bad then valid control", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    badtype "x" {
                        label: "Bad"
                    }
                    toggle "good" {
                        default: true
                    }
                }
            }
        }
    )");
    CHECK(r.hasErrors);
    bool found = false;
    for (auto& c : r.definition.pages[0].sections[0].controls) {
        if (c.id == "good") found = true;
    }
    CHECK(found);
}

// 28. Error recovery: bad page followed by valid page
TEST_CASE("Error recovery bad page then valid page", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            page {
                displayName: "Bad"
            }
            page "good" {
                displayName: "Good"
            }
        }
    )");
    CHECK(r.hasErrors);
    bool foundGood = false;
    for (auto& p : r.definition.pages) {
        if (p.id == "good") foundGood = true;
    }
    CHECK(foundGood);
}

// 29. Round-trip: parse → serialize → reparse
TEST_CASE("Round-trip parse serialize reparse", "[mcm2-parser]") {
    const char* input = R"(
        mcm "com.test.roundtrip" {
            displayName: "Round Trip"
            version: "1.2.3"
            page "general" {
                displayName: "General"
                section "main" {
                    displayName: "Main"
                    toggle "enabled" {
                        label: "Enabled"
                        default: true
                    }
                    slider "amount" {
                        label: "Amount"
                        min: 0.0
                        max: 100.0
                        step: 5.0
                        default: 50.0
                    }
                }
            }
        }
    )";

    auto r1 = ParseOk(input);
    CHECK(!r1.hasErrors);

    auto serialized = MCM2DefinitionParser::Serialize(r1.definition);
    auto r2 = ParseOk(serialized);
    CHECK(!r2.hasErrors);

    CHECK(r2.definition.id == r1.definition.id);
    CHECK(r2.definition.displayName == r1.definition.displayName);
    CHECK(r2.definition.version.major == r1.definition.version.major);
    CHECK(r2.definition.version.minor == r1.definition.version.minor);
    CHECK(r2.definition.version.patch == r1.definition.version.patch);
    REQUIRE(r2.definition.pages.size() == r1.definition.pages.size());
    REQUIRE(r2.definition.pages[0].sections.size() == r1.definition.pages[0].sections.size());
    REQUIRE(r2.definition.pages[0].sections[0].controls.size() ==
            r1.definition.pages[0].sections[0].controls.size());

    auto& c1 = r1.definition.pages[0].sections[0].controls[0];
    auto& c2 = r2.definition.pages[0].sections[0].controls[0];
    CHECK(c1.id == c2.id);
    CHECK(c1.label == c2.label);
    CHECK(std::get<MCM2ToggleProps>(c1.properties).defaultValue ==
          std::get<MCM2ToggleProps>(c2.properties).defaultValue);
}

// 30. Round-trip: complex definition with all 8 control types
TEST_CASE("Round-trip all 8 control types", "[mcm2-parser]") {
    const char* input = R"(
        mcm "com.test.alltypes" {
            displayName: "All Types"
            version: "1.0.0"
            page "p" {
                displayName: "Page"
                section "s" {
                    displayName: "Section"
                    toggle "t" { label: "Toggle" default: true }
                    slider "sl" { label: "Slider" min: 0.0 max: 10.0 step: 0.5 default: 5.0 format: "{0}" }
                    dropdown "dd" { label: "Drop" options: ["A", "B"] default: "A" }
                    keybind "kb" { label: "Key" default: 42 }
                    colour "col" { label: "Color" default: 0xFF00FF }
                    text "txt" { label: "Text" default: "hello" }
                    header "hdr" { label: "Header" }
                    empty {}
                }
            }
        }
    )";

    auto r1 = ParseOk(input);
    CHECK(!r1.hasErrors);
    REQUIRE(r1.definition.pages[0].sections[0].controls.size() == 8);

    auto serialized = MCM2DefinitionParser::Serialize(r1.definition);
    auto r2 = ParseOk(serialized);
    CHECK(!r2.hasErrors);
    REQUIRE(r2.definition.pages[0].sections[0].controls.size() == 8);

    // Verify each control type survived
    CHECK(r2.definition.pages[0].sections[0].controls[0].type == MCM2ControlType::Toggle);
    CHECK(r2.definition.pages[0].sections[0].controls[1].type == MCM2ControlType::Slider);
    CHECK(r2.definition.pages[0].sections[0].controls[2].type == MCM2ControlType::Dropdown);
    CHECK(r2.definition.pages[0].sections[0].controls[3].type == MCM2ControlType::KeyBind);
    CHECK(r2.definition.pages[0].sections[0].controls[4].type == MCM2ControlType::Colour);
    CHECK(r2.definition.pages[0].sections[0].controls[5].type == MCM2ControlType::Text);
    CHECK(r2.definition.pages[0].sections[0].controls[6].type == MCM2ControlType::Header);
    CHECK(r2.definition.pages[0].sections[0].controls[7].type == MCM2ControlType::Empty);
}

// 31. SourceLocation on MCM definition
TEST_CASE("SourceLocation on MCM definition", "[mcm2-parser]") {
    auto r = ParseOk(R"(mcm "test" { version: "1.0.0" })", "myfile.mcm");
    CHECK(r.definition.location.file == "myfile.mcm");
    CHECK(r.definition.location.line == 1);
}

// 32. SourceLocation on each control
TEST_CASE("SourceLocation on controls", "[mcm2-parser]") {
    auto r = ParseOk(R"(mcm "test" {
    page "p" {
        section "s" {
            toggle "a" { default: true }
            toggle "b" { default: false }
        }
    }
})");
    REQUIRE(r.definition.pages[0].sections[0].controls.size() == 2);
    auto& c1 = r.definition.pages[0].sections[0].controls[0];
    auto& c2 = r.definition.pages[0].sections[0].controls[1];
    CHECK(c1.location.line < c2.location.line);
}

// 33. Inline list with quoted strings
TEST_CASE("Inline list with quoted strings", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    dropdown "d" {
                        options: ["Option A", "Option B"]
                        default: "Option A"
                    }
                }
            }
        }
    )");
    auto& props = std::get<MCM2DropdownProps>(
        r.definition.pages[0].sections[0].controls[0].properties);
    REQUIRE(props.options.size() == 2);
    CHECK(props.options[0] == "Option A");
    CHECK(props.options[1] == "Option B");
}

// 34. Duplicate control IDs within a section: warning diagnostic
TEST_CASE("Duplicate control IDs warning", "[mcm2-parser]") {
    auto r = ParseAny(R"(
        mcm "test" {
            page "p" {
                section "s" {
                    toggle "dup" { default: true }
                    toggle "dup" { default: false }
                }
            }
        }
    )");
    // Should have a warning about duplicate but not necessarily an error
    bool foundDupDiag = false;
    for (auto& d : r.diagnostics) {
        if (d.code == ErrorCode::DuplicateRegistration) foundDupDiag = true;
    }
    CHECK(foundDupDiag);
}

// 35. Comments stripped correctly
TEST_CASE("Comments stripped correctly", "[mcm2-parser]") {
    auto r = ParseOk(R"(
        // This is a line comment
        mcm "test" {
            /* Block comment */
            displayName: "Test"
            version: "1.0.0"
            page "p" {
                // Another comment
                section "s" {
                    toggle "t" {
                        /* multi
                           line
                           comment */
                        default: true
                    }
                }
            }
        }
    )");
    CHECK(!r.hasErrors);
    CHECK(r.definition.id == "test");
    CHECK(r.definition.pages[0].sections[0].controls[0].id == "t");
}
