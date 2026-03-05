#include "ohui/layout/YogaWrapper.h"

#include <yoga/Yoga.h>

namespace ohui::layout {

struct LayoutNodeImpl {
    YGNodeRef node = nullptr;

    LayoutNodeImpl() : node(YGNodeNew()) {}
    ~LayoutNodeImpl() {
        if (node) {
            YGNodeFree(node);
        }
    }

    LayoutNodeImpl(const LayoutNodeImpl&) = delete;
    LayoutNodeImpl& operator=(const LayoutNodeImpl&) = delete;
};

// --- Enum mapping helpers ---

static YGFlexDirection ToYG(FlexDirection d) {
    switch (d) {
        case FlexDirection::Row:           return YGFlexDirectionRow;
        case FlexDirection::RowReverse:    return YGFlexDirectionRowReverse;
        case FlexDirection::Column:        return YGFlexDirectionColumn;
        case FlexDirection::ColumnReverse: return YGFlexDirectionColumnReverse;
    }
    return YGFlexDirectionColumn;
}

static YGJustify ToYG(JustifyContent j) {
    switch (j) {
        case JustifyContent::FlexStart:    return YGJustifyFlexStart;
        case JustifyContent::Center:       return YGJustifyCenter;
        case JustifyContent::FlexEnd:      return YGJustifyFlexEnd;
        case JustifyContent::SpaceBetween: return YGJustifySpaceBetween;
        case JustifyContent::SpaceAround:  return YGJustifySpaceAround;
        case JustifyContent::SpaceEvenly:  return YGJustifySpaceEvenly;
    }
    return YGJustifyFlexStart;
}

static YGAlign ToYGAlign(AlignItems a) {
    switch (a) {
        case AlignItems::FlexStart: return YGAlignFlexStart;
        case AlignItems::Center:    return YGAlignCenter;
        case AlignItems::FlexEnd:   return YGAlignFlexEnd;
        case AlignItems::Stretch:   return YGAlignStretch;
        case AlignItems::Baseline:  return YGAlignBaseline;
    }
    return YGAlignStretch;
}

static YGAlign ToYGAlign(AlignSelf a) {
    switch (a) {
        case AlignSelf::Auto:      return YGAlignAuto;
        case AlignSelf::FlexStart: return YGAlignFlexStart;
        case AlignSelf::Center:    return YGAlignCenter;
        case AlignSelf::FlexEnd:   return YGAlignFlexEnd;
        case AlignSelf::Stretch:   return YGAlignStretch;
        case AlignSelf::Baseline:  return YGAlignBaseline;
    }
    return YGAlignAuto;
}

static YGWrap ToYG(FlexWrap w) {
    switch (w) {
        case FlexWrap::NoWrap:      return YGWrapNoWrap;
        case FlexWrap::Wrap:        return YGWrapWrap;
        case FlexWrap::WrapReverse: return YGWrapWrapReverse;
    }
    return YGWrapNoWrap;
}

static YGDisplay ToYG(Display d) {
    switch (d) {
        case Display::Flex: return YGDisplayFlex;
        case Display::None: return YGDisplayNone;
    }
    return YGDisplayFlex;
}

static YGOverflow ToYG(Overflow o) {
    switch (o) {
        case Overflow::Visible: return YGOverflowVisible;
        case Overflow::Hidden:  return YGOverflowHidden;
        case Overflow::Scroll:  return YGOverflowScroll;
    }
    return YGOverflowVisible;
}

static YGPositionType ToYG(PositionType p) {
    switch (p) {
        case PositionType::Relative: return YGPositionTypeRelative;
        case PositionType::Absolute: return YGPositionTypeAbsolute;
    }
    return YGPositionTypeRelative;
}

static YGDirection ToYG(Direction d) {
    switch (d) {
        case Direction::LTR:     return YGDirectionLTR;
        case Direction::RTL:     return YGDirectionRTL;
        case Direction::Inherit: return YGDirectionInherit;
    }
    return YGDirectionLTR;
}

static YGEdge ToYG(Edge e) {
    switch (e) {
        case Edge::Left:   return YGEdgeLeft;
        case Edge::Top:    return YGEdgeTop;
        case Edge::Right:  return YGEdgeRight;
        case Edge::Bottom: return YGEdgeBottom;
        case Edge::Start:  return YGEdgeStart;
        case Edge::End:    return YGEdgeEnd;
        case Edge::All:    return YGEdgeAll;
    }
    return YGEdgeAll;
}

// --- LayoutNode ---

LayoutNode::LayoutNode() : m_impl(std::make_unique<LayoutNodeImpl>()) {}
LayoutNode::~LayoutNode() = default;

LayoutNode::LayoutNode(LayoutNode&& other) noexcept = default;
LayoutNode& LayoutNode::operator=(LayoutNode&& other) noexcept = default;

void LayoutNode::InsertChild(LayoutNode& child, size_t index) {
    YGNodeInsertChild(m_impl->node, child.m_impl->node, static_cast<uint32_t>(index));
}

void LayoutNode::RemoveChild(LayoutNode& child) {
    YGNodeRemoveChild(m_impl->node, child.m_impl->node);
}

void LayoutNode::RemoveAllChildren() {
    YGNodeRemoveAllChildren(m_impl->node);
}

size_t LayoutNode::GetChildCount() const {
    return YGNodeGetChildCount(m_impl->node);
}

void LayoutNode::SetFlexDirection(FlexDirection value) {
    YGNodeStyleSetFlexDirection(m_impl->node, ToYG(value));
}

void LayoutNode::SetJustifyContent(JustifyContent value) {
    YGNodeStyleSetJustifyContent(m_impl->node, ToYG(value));
}

void LayoutNode::SetAlignItems(AlignItems value) {
    YGNodeStyleSetAlignItems(m_impl->node, ToYGAlign(value));
}

void LayoutNode::SetAlignSelf(AlignSelf value) {
    YGNodeStyleSetAlignSelf(m_impl->node, ToYGAlign(value));
}

void LayoutNode::SetFlexWrap(FlexWrap value) {
    YGNodeStyleSetFlexWrap(m_impl->node, ToYG(value));
}

void LayoutNode::SetFlexGrow(float value) {
    YGNodeStyleSetFlexGrow(m_impl->node, value);
}

void LayoutNode::SetFlexShrink(float value) {
    YGNodeStyleSetFlexShrink(m_impl->node, value);
}

void LayoutNode::SetFlexBasis(float value) {
    YGNodeStyleSetFlexBasis(m_impl->node, value);
}

void LayoutNode::SetWidth(float value) {
    YGNodeStyleSetWidth(m_impl->node, value);
}

void LayoutNode::SetHeight(float value) {
    YGNodeStyleSetHeight(m_impl->node, value);
}

void LayoutNode::SetMinWidth(float value) {
    YGNodeStyleSetMinWidth(m_impl->node, value);
}

void LayoutNode::SetMinHeight(float value) {
    YGNodeStyleSetMinHeight(m_impl->node, value);
}

void LayoutNode::SetMaxWidth(float value) {
    YGNodeStyleSetMaxWidth(m_impl->node, value);
}

void LayoutNode::SetMaxHeight(float value) {
    YGNodeStyleSetMaxHeight(m_impl->node, value);
}

void LayoutNode::SetPadding(Edge edge, float value) {
    YGNodeStyleSetPadding(m_impl->node, ToYG(edge), value);
}

void LayoutNode::SetMargin(Edge edge, float value) {
    YGNodeStyleSetMargin(m_impl->node, ToYG(edge), value);
}

void LayoutNode::SetGap(float rowGap, float columnGap) {
    YGNodeStyleSetGap(m_impl->node, YGGutterRow, rowGap);
    YGNodeStyleSetGap(m_impl->node, YGGutterColumn, columnGap);
}

void LayoutNode::SetAspectRatio(float value) {
    YGNodeStyleSetAspectRatio(m_impl->node, value);
}

void LayoutNode::SetDisplay(Display value) {
    YGNodeStyleSetDisplay(m_impl->node, ToYG(value));
}

void LayoutNode::SetOverflow(Overflow value) {
    YGNodeStyleSetOverflow(m_impl->node, ToYG(value));
}

void LayoutNode::SetPositionType(PositionType value) {
    YGNodeStyleSetPositionType(m_impl->node, ToYG(value));
}

void LayoutNode::SetPosition(Edge edge, float value) {
    YGNodeStyleSetPosition(m_impl->node, ToYG(edge), value);
}

void LayoutNode::SetDirection(Direction dir) {
    YGNodeStyleSetDirection(m_impl->node, ToYG(dir));
}

LayoutRect LayoutNode::GetLayoutRect() const {
    return {
        YGNodeLayoutGetLeft(m_impl->node),
        YGNodeLayoutGetTop(m_impl->node),
        YGNodeLayoutGetWidth(m_impl->node),
        YGNodeLayoutGetHeight(m_impl->node),
    };
}

void CalculateLayout(LayoutNode& root, float availableWidth, float availableHeight) {
    YGNodeCalculateLayout(root.m_impl->node, availableWidth, availableHeight, YGDirectionLTR);
}

void CalculateLayout(LayoutNode& root, float availableWidth, float availableHeight,
                     Direction direction) {
    YGNodeCalculateLayout(root.m_impl->node, availableWidth, availableHeight, ToYG(direction));
}

}  // namespace ohui::layout
