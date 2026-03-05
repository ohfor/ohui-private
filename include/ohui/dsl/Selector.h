#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ohui::dsl {

enum class SelectorType { Type, Class, Id, PseudoClass, Universal };
enum class PseudoClass { None, Hover, Focus, Active, Disabled };

struct SelectorPart {
    SelectorType type{SelectorType::Type};
    std::string value;
    PseudoClass pseudoClass{PseudoClass::None};
};

struct Selector {
    // Descendant chain: segments[0]=ancestor ... segments[N]=target
    // Each segment is a vector of compound parts (e.g. Button.primary:hover)
    std::vector<std::vector<SelectorPart>> segments;

    struct Specificity {
        uint16_t ids{0};
        uint16_t classes{0};
        uint16_t types{0};

        bool operator>(const Specificity& other) const {
            if (ids != other.ids) return ids > other.ids;
            if (classes != other.classes) return classes > other.classes;
            return types > other.types;
        }

        bool operator==(const Specificity& other) const = default;
    };

    Specificity ComputeSpecificity() const {
        Specificity spec;
        for (const auto& segment : segments) {
            for (const auto& part : segment) {
                switch (part.type) {
                    case SelectorType::Id:
                        ++spec.ids;
                        break;
                    case SelectorType::Class:
                    case SelectorType::PseudoClass:
                        ++spec.classes;
                        break;
                    case SelectorType::Type:
                        ++spec.types;
                        break;
                    case SelectorType::Universal:
                        break;
                }
                if (part.pseudoClass != PseudoClass::None && part.type != SelectorType::PseudoClass) {
                    ++spec.classes;
                }
            }
        }
        return spec;
    }
};

}  // namespace ohui::dsl
