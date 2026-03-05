#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ohui::dsl {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
    bool operator==(const Color&) const = default;
};

enum class TextAlign { Left, Center, Right };
enum class DrawCallType { Rect, Text, Icon, Image, Line };

struct DrawRect {
    float x{}, y{}, width{}, height{};
    Color fillColor;
    Color borderColor;
    float borderWidth{0};
    float borderRadius{0};
    float opacity{1.0f};
};

struct DrawText {
    float x{}, y{}, width{}, height{};
    std::string text;
    std::string fontFamily;
    float fontSize{14.0f};
    Color color;
    TextAlign align{TextAlign::Left};
    float opacity{1.0f};
};

struct DrawIcon {
    float x{}, y{}, width{}, height{};
    std::string source;
    Color tint{255, 255, 255, 255};
    float opacity{1.0f};
};

struct DrawImage {
    float x{}, y{}, width{}, height{};
    std::string source;
    float opacity{1.0f};
    float sliceLeft{0}, sliceTop{0}, sliceRight{0}, sliceBottom{0};
};

struct DrawLine {
    float x1{}, y1{}, x2{}, y2{};
    Color color;
    float thickness{1.0f};
    float opacity{1.0f};
};

struct ClipRect {
    float x{}, y{}, width{}, height{};
};

struct DrawCall {
    DrawCallType type;
    std::variant<DrawRect, DrawText, DrawIcon, DrawImage, DrawLine> data;
    int32_t zIndex{0};
    std::optional<ClipRect> clip;
};

struct DrawCallList {
    std::vector<DrawCall> calls;
};

}  // namespace ohui::dsl
