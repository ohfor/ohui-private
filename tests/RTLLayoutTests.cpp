#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/layout/YogaWrapper.h"

using namespace ohui::layout;
using Catch::Matchers::WithinAbs;

TEST_CASE("Default CalculateLayout still works (LTR backward compat)", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Row);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    root.InsertChild(child, 0);

    CalculateLayout(root, 400, 300);

    auto rect = child.GetLayoutRect();
    CHECK_THAT(rect.left, WithinAbs(0.0f, 0.1f));
    CHECK_THAT(rect.width, WithinAbs(100.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("RTL direction: row children layout right-to-left", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Row);

    LayoutNode child1;
    child1.SetWidth(100);
    child1.SetHeight(50);

    LayoutNode child2;
    child2.SetWidth(100);
    child2.SetHeight(50);

    root.InsertChild(child1, 0);
    root.InsertChild(child2, 1);

    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect1 = child1.GetLayoutRect();
    auto rect2 = child2.GetLayoutRect();
    // In RTL, first child should be on the right side
    CHECK(rect1.left > rect2.left);

    root.RemoveAllChildren();
}

TEST_CASE("RTL direction: Edge::Start maps to right", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetMargin(Edge::Start, 20);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // In RTL, Start = right side. Child pushed 20px from right.
    CHECK_THAT(rect.left, WithinAbs(280.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("RTL direction: Edge::End maps to left", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetMargin(Edge::End, 30);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // In RTL, End = left side. Child has 30px margin on left.
    CHECK_THAT(rect.left, WithinAbs(300.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("LTR direction: Edge::Start maps to left", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetMargin(Edge::Start, 20);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::LTR);

    auto rect = child.GetLayoutRect();
    CHECK_THAT(rect.left, WithinAbs(20.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("LTR direction: Edge::End maps to right", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetMargin(Edge::End, 30);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::LTR);

    auto rect = child.GetLayoutRect();
    // In LTR, End = right. Margin on right doesn't affect left position.
    CHECK_THAT(rect.left, WithinAbs(0.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("SetDirection on node propagates to children (Inherit)", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Row);
    root.SetDirection(Direction::RTL);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetDirection(Direction::Inherit);

    LayoutNode grandchild;
    grandchild.SetWidth(50);
    grandchild.SetHeight(25);
    grandchild.SetMargin(Edge::Start, 10);

    child.InsertChild(grandchild, 0);
    root.InsertChild(child, 0);

    CalculateLayout(root, 400, 300, Direction::RTL);

    // Grandchild inherits RTL, so Start margin = right side
    auto gcRect = grandchild.GetLayoutRect();
    // Grandchild is inside child (width 100), with 10px start(right) margin
    CHECK_THAT(gcRect.left, WithinAbs(40.0f, 0.1f));

    child.RemoveAllChildren();
    root.RemoveAllChildren();
}

TEST_CASE("Mixed LTR/RTL: parent RTL, child overrides LTR", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Column);
    root.SetDirection(Direction::RTL);

    LayoutNode child;
    child.SetWidth(200);
    child.SetHeight(50);
    child.SetDirection(Direction::LTR);

    LayoutNode grandchild;
    grandchild.SetWidth(50);
    grandchild.SetHeight(25);
    grandchild.SetMargin(Edge::Start, 10);

    child.InsertChild(grandchild, 0);
    root.InsertChild(child, 0);

    CalculateLayout(root, 400, 300, Direction::RTL);

    // Child overrides to LTR, so Start = left for grandchild
    auto gcRect = grandchild.GetLayoutRect();
    CHECK_THAT(gcRect.left, WithinAbs(10.0f, 0.1f));

    child.RemoveAllChildren();
    root.RemoveAllChildren();
}

TEST_CASE("Padding with Edge::Start in RTL applies to right side", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetPadding(Edge::Start, 50);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // In RTL, Start padding = right padding, child aligns to the right.
    // With 50px right padding, child at right = 400 - 50 - 100 = 250
    CHECK_THAT(rect.left, WithinAbs(250.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("Margin with Edge::End in RTL applies to left side", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    child.SetMargin(Edge::End, 40);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // In RTL, End = left. Child has 40px margin on left.
    CHECK_THAT(rect.left, WithinAbs(300.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("Direction::Inherit uses parent direction", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Row);
    root.SetDirection(Direction::RTL);

    LayoutNode child;
    child.SetWidth(100);
    child.SetHeight(50);
    // Default direction is Inherit for Yoga nodes

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // RTL: first child on right
    CHECK_THAT(rect.left, WithinAbs(300.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("Two-column layout mirrors correctly in RTL", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);
    root.SetFlexDirection(FlexDirection::Row);

    LayoutNode left;
    left.SetWidth(150);
    left.SetHeight(300);

    LayoutNode right;
    right.SetWidth(250);
    right.SetHeight(300);

    root.InsertChild(left, 0);
    root.InsertChild(right, 1);

    // LTR: left at 0, right at 150
    CalculateLayout(root, 400, 300, Direction::LTR);
    CHECK_THAT(left.GetLayoutRect().left, WithinAbs(0.0f, 0.1f));
    CHECK_THAT(right.GetLayoutRect().left, WithinAbs(150.0f, 0.1f));

    // RTL: left at 250, right at 0
    CalculateLayout(root, 400, 300, Direction::RTL);
    CHECK_THAT(left.GetLayoutRect().left, WithinAbs(250.0f, 0.1f));
    CHECK_THAT(right.GetLayoutRect().left, WithinAbs(0.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("Absolute positioned element respects Edge::Start in RTL", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(300);

    LayoutNode child;
    child.SetPositionType(PositionType::Absolute);
    child.SetWidth(80);
    child.SetHeight(40);
    child.SetPosition(Edge::Start, 10);
    child.SetPosition(Edge::Top, 20);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 300, Direction::RTL);

    auto rect = child.GetLayoutRect();
    // In RTL, Start = right, so position from right edge: left = 400 - 10 - 80 = 310
    CHECK_THAT(rect.left, WithinAbs(310.0f, 0.1f));
    CHECK_THAT(rect.top, WithinAbs(20.0f, 0.1f));

    root.RemoveAllChildren();
}

TEST_CASE("FlexDirection::Row in RTL reverses", "[rtl]") {
    LayoutNode root;
    root.SetWidth(300);
    root.SetHeight(100);
    root.SetFlexDirection(FlexDirection::Row);

    LayoutNode a, b, c;
    a.SetWidth(50); a.SetHeight(50);
    b.SetWidth(50); b.SetHeight(50);
    c.SetWidth(50); c.SetHeight(50);

    root.InsertChild(a, 0);
    root.InsertChild(b, 1);
    root.InsertChild(c, 2);

    CalculateLayout(root, 300, 100, Direction::RTL);

    // RTL row: a at right, b in middle, c at left
    CHECK(a.GetLayoutRect().left > b.GetLayoutRect().left);
    CHECK(b.GetLayoutRect().left > c.GetLayoutRect().left);

    root.RemoveAllChildren();
}

TEST_CASE("Gap spacing unaffected by direction (symmetric)", "[rtl]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(100);
    root.SetFlexDirection(FlexDirection::Row);
    root.SetGap(0, 20);

    LayoutNode a, b;
    a.SetWidth(100); a.SetHeight(50);
    b.SetWidth(100); b.SetHeight(50);

    root.InsertChild(a, 0);
    root.InsertChild(b, 1);

    // LTR gap
    CalculateLayout(root, 400, 100, Direction::LTR);
    float ltrGap = b.GetLayoutRect().left - (a.GetLayoutRect().left + a.GetLayoutRect().width);

    // RTL gap
    CalculateLayout(root, 400, 100, Direction::RTL);
    float rtlGap = a.GetLayoutRect().left - (b.GetLayoutRect().left + b.GetLayoutRect().width);

    CHECK_THAT(ltrGap, WithinAbs(20.0f, 0.1f));
    CHECK_THAT(rtlGap, WithinAbs(20.0f, 0.1f));

    root.RemoveAllChildren();
}
