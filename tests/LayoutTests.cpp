#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/layout/YogaWrapper.h"

using namespace ohui::layout;
using Catch::Matchers::WithinAbs;

static constexpr double kEps = 0.5;

TEST_CASE("Default node creates valid", "[layout]") {
    LayoutNode node;
    CalculateLayout(node, 100, 100);
    auto rect = node.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect.left), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(rect.top), WithinAbs(0.0, kEps));
}

TEST_CASE("Column stack distributes height", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Column);
    root.SetWidth(300);
    root.SetHeight(600);

    LayoutNode c1, c2, c3;
    c1.SetHeight(50);
    c2.SetHeight(50);
    c3.SetHeight(50);

    root.InsertChild(c1, 0);
    root.InsertChild(c2, 1);
    root.InsertChild(c3, 2);
    CalculateLayout(root, 300, 600);

    auto r1 = c1.GetLayoutRect();
    auto r2 = c2.GetLayoutRect();
    auto r3 = c3.GetLayoutRect();

    CHECK_THAT(static_cast<double>(r1.top), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(r1.height), WithinAbs(50.0, kEps));
    CHECK_THAT(static_cast<double>(r2.top), WithinAbs(50.0, kEps));
    CHECK_THAT(static_cast<double>(r2.height), WithinAbs(50.0, kEps));
    CHECK_THAT(static_cast<double>(r3.top), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(r3.height), WithinAbs(50.0, kEps));
}

TEST_CASE("Row stack distributes width", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Row);
    root.SetWidth(300);
    root.SetHeight(100);

    LayoutNode c1, c2;
    c1.SetWidth(100);
    c2.SetWidth(100);

    root.InsertChild(c1, 0);
    root.InsertChild(c2, 1);
    CalculateLayout(root, 300, 100);

    auto r1 = c1.GetLayoutRect();
    auto r2 = c2.GetLayoutRect();

    CHECK_THAT(static_cast<double>(r1.left), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(r1.width), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(r2.left), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(r2.width), WithinAbs(100.0, kEps));
}

TEST_CASE("Nested flex layout", "[layout]") {
    // root (column) → 2 row containers → 2 children each
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Column);
    root.SetWidth(200);
    root.SetHeight(200);

    LayoutNode row1, row2;
    row1.SetFlexDirection(FlexDirection::Row);
    row1.SetHeight(100);
    row2.SetFlexDirection(FlexDirection::Row);
    row2.SetHeight(100);

    LayoutNode a1, a2, b1, b2;
    a1.SetWidth(100);
    a2.SetWidth(100);
    b1.SetWidth(100);
    b2.SetWidth(100);

    row1.InsertChild(a1, 0);
    row1.InsertChild(a2, 1);
    row2.InsertChild(b1, 0);
    row2.InsertChild(b2, 1);

    root.InsertChild(row1, 0);
    root.InsertChild(row2, 1);
    CalculateLayout(root, 200, 200);

    auto ra1 = a1.GetLayoutRect();
    auto ra2 = a2.GetLayoutRect();
    auto rb1 = b1.GetLayoutRect();

    CHECK_THAT(static_cast<double>(ra1.left), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(ra2.left), WithinAbs(100.0, kEps));

    auto rRow2 = row2.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rRow2.top), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(rb1.left), WithinAbs(0.0, kEps));
}

TEST_CASE("FlexGrow distributes remaining space", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Row);
    root.SetWidth(300);
    root.SetHeight(100);

    LayoutNode c1, c2;
    c1.SetFlexGrow(1);
    c2.SetFlexGrow(2);

    root.InsertChild(c1, 0);
    root.InsertChild(c2, 1);
    CalculateLayout(root, 300, 100);

    auto r1 = c1.GetLayoutRect();
    auto r2 = c2.GetLayoutRect();

    CHECK_THAT(static_cast<double>(r1.width), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(r2.width), WithinAbs(200.0, kEps));
}

TEST_CASE("Min/max constraints respected", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Column);
    root.SetWidth(400);
    root.SetHeight(400);

    LayoutNode child;
    child.SetFlexGrow(1);
    child.SetMaxHeight(100);
    child.SetMinWidth(50);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 400);

    auto rect = child.GetLayoutRect();
    CHECK(rect.height <= 100.5f);
    CHECK(rect.width >= 49.5f);
}

TEST_CASE("Padding reduces content area", "[layout]") {
    LayoutNode root;
    root.SetWidth(200);
    root.SetHeight(200);
    root.SetPadding(Edge::All, 20);
    root.SetFlexDirection(FlexDirection::Column);

    LayoutNode child;
    child.SetFlexGrow(1);

    root.InsertChild(child, 0);
    CalculateLayout(root, 200, 200);

    auto rect = child.GetLayoutRect();
    // Content area reduced by padding on both sides
    CHECK_THAT(static_cast<double>(rect.width), WithinAbs(160.0, kEps));
    CHECK_THAT(static_cast<double>(rect.height), WithinAbs(160.0, kEps));
    CHECK_THAT(static_cast<double>(rect.left), WithinAbs(20.0, kEps));
    CHECK_THAT(static_cast<double>(rect.top), WithinAbs(20.0, kEps));
}

TEST_CASE("Margin creates spacing", "[layout]") {
    LayoutNode root;
    root.SetWidth(200);
    root.SetHeight(200);
    root.SetFlexDirection(FlexDirection::Column);

    LayoutNode child;
    child.SetHeight(50);
    child.SetMargin(Edge::Top, 30);

    root.InsertChild(child, 0);
    CalculateLayout(root, 200, 200);

    auto rect = child.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect.top), WithinAbs(30.0, kEps));
}

TEST_CASE("Gap between children", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Column);
    root.SetWidth(200);
    root.SetHeight(400);
    root.SetGap(10, 0);

    LayoutNode c1, c2, c3;
    c1.SetHeight(50);
    c2.SetHeight(50);
    c3.SetHeight(50);

    root.InsertChild(c1, 0);
    root.InsertChild(c2, 1);
    root.InsertChild(c3, 2);
    CalculateLayout(root, 200, 400);

    auto r1 = c1.GetLayoutRect();
    auto r2 = c2.GetLayoutRect();
    auto r3 = c3.GetLayoutRect();

    CHECK_THAT(static_cast<double>(r1.top), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(r2.top), WithinAbs(60.0, kEps));  // 50 + 10 gap
    CHECK_THAT(static_cast<double>(r3.top), WithinAbs(120.0, kEps)); // 50 + 10 + 50 + 10
}

TEST_CASE("AspectRatio constrains dimensions", "[layout]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(400);
    root.SetFlexDirection(FlexDirection::Column);

    LayoutNode child;
    child.SetWidth(200);
    child.SetAspectRatio(2.0f); // width/height = 2, so height = 100

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 400);

    auto rect = child.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect.width), WithinAbs(200.0, kEps));
    CHECK_THAT(static_cast<double>(rect.height), WithinAbs(100.0, kEps));
}

TEST_CASE("Display::None excludes from layout", "[layout]") {
    LayoutNode root;
    root.SetFlexDirection(FlexDirection::Column);
    root.SetWidth(200);
    root.SetHeight(400);

    LayoutNode c1, c2, c3;
    c1.SetHeight(50);
    c2.SetHeight(50);
    c2.SetDisplay(Display::None);
    c3.SetHeight(50);

    root.InsertChild(c1, 0);
    root.InsertChild(c2, 1);
    root.InsertChild(c3, 2);
    CalculateLayout(root, 200, 400);

    auto r1 = c1.GetLayoutRect();
    auto r3 = c3.GetLayoutRect();

    // c3 should be right after c1 since c2 is hidden
    CHECK_THAT(static_cast<double>(r1.top), WithinAbs(0.0, kEps));
    CHECK_THAT(static_cast<double>(r3.top), WithinAbs(50.0, kEps));
}

TEST_CASE("Absolute positioning", "[layout]") {
    LayoutNode root;
    root.SetWidth(400);
    root.SetHeight(400);

    LayoutNode child;
    child.SetPositionType(PositionType::Absolute);
    child.SetPosition(Edge::Left, 50);
    child.SetPosition(Edge::Top, 30);
    child.SetWidth(100);
    child.SetHeight(80);

    root.InsertChild(child, 0);
    CalculateLayout(root, 400, 400);

    auto rect = child.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect.left), WithinAbs(50.0, kEps));
    CHECK_THAT(static_cast<double>(rect.top), WithinAbs(30.0, kEps));
    CHECK_THAT(static_cast<double>(rect.width), WithinAbs(100.0, kEps));
    CHECK_THAT(static_cast<double>(rect.height), WithinAbs(80.0, kEps));
}

TEST_CASE("Move semantics transfer ownership", "[layout]") {
    LayoutNode original;
    original.SetWidth(123);
    original.SetHeight(456);

    LayoutNode moved = std::move(original);
    CalculateLayout(moved, 200, 500);

    auto rect = moved.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect.width), WithinAbs(123.0, kEps));
    CHECK_THAT(static_cast<double>(rect.height), WithinAbs(456.0, kEps));

    // Move assignment
    LayoutNode another;
    another = std::move(moved);
    CalculateLayout(another, 200, 500);

    auto rect2 = another.GetLayoutRect();
    CHECK_THAT(static_cast<double>(rect2.width), WithinAbs(123.0, kEps));
    CHECK_THAT(static_cast<double>(rect2.height), WithinAbs(456.0, kEps));
}
