#pragma once

#include <cstddef>
#include <memory>

namespace ohui::layout {

enum class FlexDirection { Row, RowReverse, Column, ColumnReverse };
enum class JustifyContent { FlexStart, Center, FlexEnd, SpaceBetween, SpaceAround, SpaceEvenly };
enum class AlignItems { FlexStart, Center, FlexEnd, Stretch, Baseline };
enum class AlignSelf { Auto, FlexStart, Center, FlexEnd, Stretch, Baseline };
enum class FlexWrap { NoWrap, Wrap, WrapReverse };
enum class Display { Flex, None };
enum class Overflow { Visible, Hidden, Scroll };
enum class PositionType { Relative, Absolute };
enum class Edge { Left, Top, Right, Bottom, All };

struct LayoutRect {
    float left{};
    float top{};
    float width{};
    float height{};
};

struct LayoutNodeImpl;

class LayoutNode {
public:
    LayoutNode();
    ~LayoutNode();
    LayoutNode(LayoutNode&&) noexcept;
    LayoutNode& operator=(LayoutNode&&) noexcept;

    LayoutNode(const LayoutNode&) = delete;
    LayoutNode& operator=(const LayoutNode&) = delete;

    // Children (non-owning — caller owns child lifetime)
    void InsertChild(LayoutNode& child, size_t index);
    void RemoveChild(LayoutNode& child);
    void RemoveAllChildren();
    size_t GetChildCount() const;

    // Style setters: flex container
    void SetFlexDirection(FlexDirection value);
    void SetJustifyContent(JustifyContent value);
    void SetAlignItems(AlignItems value);
    void SetAlignSelf(AlignSelf value);
    void SetFlexWrap(FlexWrap value);

    // Style setters: flex item
    void SetFlexGrow(float value);
    void SetFlexShrink(float value);
    void SetFlexBasis(float value);

    // Dimensions
    void SetWidth(float value);
    void SetHeight(float value);
    void SetMinWidth(float value);
    void SetMinHeight(float value);
    void SetMaxWidth(float value);
    void SetMaxHeight(float value);

    // Spacing
    void SetPadding(Edge edge, float value);
    void SetMargin(Edge edge, float value);
    void SetGap(float rowGap, float columnGap);

    // Other
    void SetAspectRatio(float value);
    void SetDisplay(Display value);
    void SetOverflow(Overflow value);
    void SetPositionType(PositionType value);
    void SetPosition(Edge edge, float value);

    // Result (valid after CalculateLayout)
    LayoutRect GetLayoutRect() const;

private:
    std::unique_ptr<LayoutNodeImpl> m_impl;
    friend void CalculateLayout(LayoutNode& root, float availableWidth, float availableHeight);
};

void CalculateLayout(LayoutNode& root, float availableWidth, float availableHeight);

}  // namespace ohui::layout
