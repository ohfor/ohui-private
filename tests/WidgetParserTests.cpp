#include <catch2/catch_test_macros.hpp>

#include "ohui/dsl/WidgetParser.h"

using namespace ohui;
using namespace ohui::dsl;

TEST_CASE("Empty input produces empty AST, no errors", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse("");
    REQUIRE(result.has_value());
    CHECK(result->ast.widgets.empty());
    CHECK(result->ast.skins.empty());
    CHECK(result->ast.instances.empty());
    CHECK(result->diagnostics.empty());
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Minimal widget definition", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse("widget Foo {}");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 1);
    CHECK(result->ast.widgets[0].name == "Foo");
    CHECK(result->ast.widgets[0].propertyDecls.empty());
    CHECK(result->ast.widgets[0].rootComponent == nullptr);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Widget with property declarations -- all 5 types", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget Bar {
            property current: float;
            property count: int;
            property visible: bool;
            property label: string;
            property tint: color;
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 1);
    auto& w = result->ast.widgets[0];
    REQUIRE(w.propertyDecls.size() == 5);
    CHECK(w.propertyDecls[0].name == "current");
    CHECK(w.propertyDecls[0].type == PropertyType::Float);
    CHECK(w.propertyDecls[1].name == "count");
    CHECK(w.propertyDecls[1].type == PropertyType::Int);
    CHECK(w.propertyDecls[2].name == "visible");
    CHECK(w.propertyDecls[2].type == PropertyType::Bool);
    CHECK(w.propertyDecls[3].name == "label");
    CHECK(w.propertyDecls[3].type == PropertyType::String);
    CHECK(w.propertyDecls[4].name == "tint");
    CHECK(w.propertyDecls[4].type == PropertyType::Color);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Widget with requires clause", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget VersionedWidget {
            requires ohui >= 1.3;
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 1);
    auto& w = result->ast.widgets[0];
    REQUIRE(w.requiresVersion.has_value());
    CHECK(w.requiresVersion->major == 1);
    CHECK(w.requiresVersion->minor == 3);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("HealthBar full example", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget HealthBar {
            requires ohui >= 1.0;
            property current: float;
            property max: float;

            Panel {
                class: "health-bar-container";

                ValueBar {
                    value: bind(player.health);
                    maxValue: bind(player.healthMax);
                    fillColor: token(--health-color);
                }

                Label {
                    text: bind(player.healthText);
                    fontSize: 14;
                }
            }
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 1);
    auto& w = result->ast.widgets[0];
    CHECK(w.name == "HealthBar");
    REQUIRE(w.requiresVersion.has_value());
    CHECK(w.requiresVersion->major == 1);
    CHECK(w.requiresVersion->minor == 0);
    REQUIRE(w.propertyDecls.size() == 2);
    REQUIRE(w.rootComponent != nullptr);
    CHECK(w.rootComponent->typeName == "Panel");
    REQUIRE(w.rootComponent->children.size() == 2);
    CHECK(w.rootComponent->children[0]->typeName == "ValueBar");
    CHECK(w.rootComponent->children[1]->typeName == "Label");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Component tree depth -- Panel containing ValueBar and Label", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                ValueBar {
                    value: 50;
                }
                Label {
                    text: "Hello";
                }
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& root = result->ast.widgets[0].rootComponent;
    REQUIRE(root != nullptr);
    CHECK(root->typeName == "Panel");
    REQUIRE(root->children.size() == 2);
    CHECK(root->children[0]->typeName == "ValueBar");
    CHECK(root->children[1]->typeName == "Label");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Component with class property", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                class: "health-bar-container";
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& root = result->ast.widgets[0].rootComponent;
    REQUIRE(root != nullptr);
    REQUIRE(root->properties.size() == 1);
    CHECK(root->properties[0].name == "class");
    CHECK(root->properties[0].value.kind == ExprKind::Literal);
    CHECK(root->properties[0].value.value == "health-bar-container");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("bind() expression", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Label {
                text: bind(player.name);
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& label = result->ast.widgets[0].rootComponent;
    REQUIRE(label != nullptr);
    REQUIRE(label->properties.size() == 1);
    CHECK(label->properties[0].value.kind == ExprKind::Bind);
    CHECK(label->properties[0].value.value == "player.name");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("token() expression", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                fillColor: token(--health-color);
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& panel = result->ast.widgets[0].rootComponent;
    REQUIRE(panel != nullptr);
    REQUIRE(panel->properties.size() == 1);
    CHECK(panel->properties[0].value.kind == ExprKind::Token);
    CHECK(panel->properties[0].value.value == "--health-color");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("var() expression with fallback", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                color: var(--primary, red);
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& panel = result->ast.widgets[0].rootComponent;
    REQUIRE(panel != nullptr);
    REQUIRE(panel->properties.size() == 1);
    CHECK(panel->properties[0].value.kind == ExprKind::Var);
    CHECK(panel->properties[0].value.value == "--primary");
    CHECK(panel->properties[0].value.fallback == "red");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Literal string expression", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Label {
                text: "Hello World";
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& label = result->ast.widgets[0].rootComponent;
    REQUIRE(label != nullptr);
    REQUIRE(label->properties.size() == 1);
    CHECK(label->properties[0].value.kind == ExprKind::Literal);
    CHECK(label->properties[0].value.value == "Hello World");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Literal number with unit", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                duration: 150ms;
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& panel = result->ast.widgets[0].rootComponent;
    REQUIRE(panel != nullptr);
    REQUIRE(panel->properties.size() == 1);
    CHECK(panel->properties[0].value.kind == ExprKind::Literal);
    CHECK(panel->properties[0].value.value == "150ms");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Animate block", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            Panel {
                animate opacity {
                    duration: 200ms;
                    easing: ease-out;
                }
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& panel = result->ast.widgets[0].rootComponent;
    REQUIRE(panel != nullptr);
    REQUIRE(panel->animations.size() == 1);
    CHECK(panel->animations[0].property == "opacity");
    CHECK(panel->animations[0].durationMs == 200.0f);
    CHECK(panel->animations[0].easing == EasingFunction::EaseOut);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Skin with tokens only", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        skin HealthBar "dark-theme" {
            tokens {
                --health-color: #FF0000;
                --bg-color: #222222;
            }
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.skins.size() == 1);
    auto& s = result->ast.skins[0];
    CHECK(s.widgetName == "HealthBar");
    CHECK(s.skinName == "dark-theme");
    REQUIRE(s.tokens.size() == 2);
    CHECK(s.tokens[0].name == "--health-color");
    CHECK(s.tokens[1].name == "--bg-color");
    CHECK(s.templateRoot == nullptr);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Skin with tokens and template", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        skin HealthBar "custom" {
            tokens {
                --color: blue;
            }
            template {
                Panel {
                    Label {
                        text: "Custom";
                    }
                }
            }
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.skins.size() == 1);
    auto& s = result->ast.skins[0];
    REQUIRE(s.tokens.size() == 1);
    REQUIRE(s.templateRoot != nullptr);
    CHECK(s.templateRoot->typeName == "Panel");
    REQUIRE(s.templateRoot->children.size() == 1);
    CHECK(s.templateRoot->children[0]->typeName == "Label");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Skin requires clause", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        skin HealthBar "versioned" {
            requires ohui >= 2.0;
            tokens {
                --color: red;
            }
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.skins.size() == 1);
    REQUIRE(result->ast.skins[0].requiresVersion.has_value());
    CHECK(result->ast.skins[0].requiresVersion->major == 2);
    CHECK(result->ast.skins[0].requiresVersion->minor == 0);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Widget instance", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        HealthBar {
            current: bind(player.health);
            max: bind(player.healthMax);
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.instances.size() == 1);
    auto& inst = result->ast.instances[0];
    CHECK(inst.widgetTypeName == "HealthBar");
    REQUIRE(inst.properties.size() == 2);
    CHECK(inst.properties[0].name == "current");
    CHECK(inst.properties[0].value.kind == ExprKind::Bind);
    CHECK(inst.properties[1].name == "max");
    CHECK(inst.properties[1].value.kind == ExprKind::Bind);
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Update declaration", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        HealthBar {
            current: bind(player.health);
            update: reactive(player.health, player.healthMax);
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.instances.size() == 1);
    auto& inst = result->ast.instances[0];
    REQUIRE(inst.updateDecl.has_value());
    CHECK(inst.updateDecl->mode == "reactive");
    REQUIRE(inst.updateDecl->watchBindings.size() == 2);
    CHECK(inst.updateDecl->watchBindings[0] == "player.health");
    CHECK(inst.updateDecl->watchBindings[1] == "player.healthMax");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Syntax error reports correct line/column", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse("widget Foo {\n  @invalid\n}", "test.widget");
    REQUIRE(result.has_value());
    CHECK(result->hasErrors);
    REQUIRE_FALSE(result->diagnostics.empty());
    CHECK(result->diagnostics[0].location.file == "test.widget");
    CHECK(result->diagnostics[0].location.line == 2);
}

TEST_CASE("Error recovery -- malformed widget + valid widget", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget Bad {
            @invalid stuff here
        }
        widget Good {
            Panel {
                color: red;
            }
        }
    )");
    REQUIRE(result.has_value());
    CHECK(result->hasErrors);
    // The second valid widget should still be parsed
    bool foundGood = false;
    for (auto& w : result->ast.widgets) {
        if (w.name == "Good") foundGood = true;
    }
    CHECK(foundGood);
}

TEST_CASE("Comments within widget definitions", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        // This is a line comment
        widget Foo {
            /* block comment */
            property x: float;

            Panel {
                // property inside component
                color: red;
                /* another block comment */
            }
        }
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 1);
    CHECK(result->ast.widgets[0].name == "Foo");
    REQUIRE(result->ast.widgets[0].propertyDecls.size() == 1);
    CHECK(result->ast.widgets[0].propertyDecls[0].name == "x");
    REQUIRE(result->ast.widgets[0].rootComponent != nullptr);
    CHECK(result->ast.widgets[0].rootComponent->typeName == "Panel");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Multiple widgets in one file", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget Alpha {}
        widget Beta {}
        widget Gamma {}
    )");
    REQUIRE(result.has_value());
    REQUIRE(result->ast.widgets.size() == 3);
    CHECK(result->ast.widgets[0].name == "Alpha");
    CHECK(result->ast.widgets[1].name == "Beta");
    CHECK(result->ast.widgets[2].name == "Gamma");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("Nested components -- three levels deep", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget Deep {
            Panel {
                Panel {
                    Label {
                        text: "deep";
                    }
                }
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& root = result->ast.widgets[0].rootComponent;
    REQUIRE(root != nullptr);
    CHECK(root->typeName == "Panel");
    REQUIRE(root->children.size() == 1);
    CHECK(root->children[0]->typeName == "Panel");
    REQUIRE(root->children[0]->children.size() == 1);
    CHECK(root->children[0]->children[0]->typeName == "Label");
    REQUIRE(root->children[0]->children[0]->properties.size() == 1);
    CHECK(root->children[0]->children[0]->properties[0].value.value == "deep");
    CHECK_FALSE(result->hasErrors);
}

TEST_CASE("PropertyRef disambiguation", "[dsl-parser]") {
    WidgetParser parser;
    auto result = parser.Parse(R"(
        widget TestWidget {
            property current: float;
            property max: float;

            ValueBar {
                value: current;
                label: "static";
                maxValue: max;
                unknown: someLiteral;
            }
        }
    )");
    REQUIRE(result.has_value());
    auto& vbar = result->ast.widgets[0].rootComponent;
    REQUIRE(vbar != nullptr);
    REQUIRE(vbar->properties.size() == 4);

    CHECK(vbar->properties[0].name == "value");
    CHECK(vbar->properties[0].value.kind == ExprKind::PropertyRef);
    CHECK(vbar->properties[0].value.value == "current");

    CHECK(vbar->properties[1].name == "label");
    CHECK(vbar->properties[1].value.kind == ExprKind::Literal);

    CHECK(vbar->properties[2].name == "maxValue");
    CHECK(vbar->properties[2].value.kind == ExprKind::PropertyRef);
    CHECK(vbar->properties[2].value.value == "max");

    CHECK(vbar->properties[3].name == "unknown");
    CHECK(vbar->properties[3].value.kind == ExprKind::Literal);
    CHECK(vbar->properties[3].value.value == "someLiteral");
    CHECK_FALSE(result->hasErrors);
}
