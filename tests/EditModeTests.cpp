#include <catch2/catch_test_macros.hpp>

#include "ohui/widget/SnapGrid.h"
#include "ohui/widget/MultiSelectState.h"
#include "ohui/widget/UndoRedoStack.h"
#include "ohui/widget/EditCommands.h"
#include "ohui/widget/EditSession.h"

using namespace ohui;
using namespace ohui::widget;

static WidgetManifest MakeManifest(const std::string& id, Vec2 defPos = {10.0f, 20.0f},
                                   Vec2 defSize = {100.0f, 50.0f}) {
    return WidgetManifest{id, id + "_display", defPos, defSize, {}, {}, true};
}

// --- SnapGrid tests ---

TEST_CASE("Snap rounds value to nearest grid point", "[edit]") {
    CHECK(Snap(17.0f, 10.0f) == 20.0f);
    CHECK(Snap(13.0f, 10.0f) == 10.0f);
    CHECK(Snap(15.0f, 10.0f) == 20.0f);  // round half up
    CHECK(Snap(25.0f, 10.0f) == 30.0f);  // round half up (2.5 -> 3)
}

TEST_CASE("Snap with zero grid size returns original value", "[edit]") {
    CHECK(Snap(17.3f, 0.0f) == 17.3f);
    CHECK(Snap(42.0f, -5.0f) == 42.0f);
}

TEST_CASE("SnapPosition snaps both axes", "[edit]") {
    auto result = SnapPosition({17.0f, 23.0f}, 10.0f);
    CHECK(result == Vec2{20.0f, 20.0f});
}

// --- MultiSelectState tests ---

TEST_CASE("MultiSelect Add/Remove/Toggle/Clear/GetSelected/Count", "[edit]") {
    MultiSelectState sel;
    CHECK(sel.IsEmpty());
    CHECK(sel.Count() == 0);

    sel.Add("a");
    sel.Add("b");
    CHECK(sel.Count() == 2);
    CHECK(sel.IsSelected("a"));
    CHECK(sel.IsSelected("b"));
    CHECK(!sel.IsSelected("c"));

    sel.Remove("a");
    CHECK(sel.Count() == 1);
    CHECK(!sel.IsSelected("a"));

    sel.Toggle("b");  // removes b
    CHECK(sel.Count() == 0);

    sel.Toggle("c");  // adds c
    CHECK(sel.IsSelected("c"));

    sel.Add("d");
    sel.Clear();
    CHECK(sel.IsEmpty());
}

// --- UndoRedoStack tests ---

namespace {
struct TestCommand : EditCommand {
    int& value;
    int oldVal;
    int newVal;
    TestCommand(int& v, int nv) : value(v), oldVal(v), newVal(nv) {}
    void Execute() override { value = newVal; }
    void Undo() override { value = oldVal; }
    std::string Description() const override { return "test"; }
};
}

TEST_CASE("UndoRedoStack Execute pushes to undo", "[edit]") {
    UndoRedoStack stack;
    int val = 0;
    stack.Execute(std::make_unique<TestCommand>(val, 10));
    CHECK(val == 10);
    CHECK(stack.UndoCount() == 1);
    CHECK(stack.RedoCount() == 0);
}

TEST_CASE("UndoRedoStack Execute clears redo", "[edit]") {
    UndoRedoStack stack;
    int val = 0;
    stack.Execute(std::make_unique<TestCommand>(val, 10));
    stack.Undo();
    CHECK(stack.RedoCount() == 1);

    stack.Execute(std::make_unique<TestCommand>(val, 20));
    CHECK(stack.RedoCount() == 0);
}

TEST_CASE("UndoRedoStack Undo reverses last command", "[edit]") {
    UndoRedoStack stack;
    int val = 0;
    stack.Execute(std::make_unique<TestCommand>(val, 10));
    stack.Undo();
    CHECK(val == 0);
}

TEST_CASE("UndoRedoStack Redo re-applies command", "[edit]") {
    UndoRedoStack stack;
    int val = 0;
    stack.Execute(std::make_unique<TestCommand>(val, 10));
    stack.Undo();
    stack.Redo();
    CHECK(val == 10);
}

TEST_CASE("UndoRedoStack CanUndo/CanRedo reflect stack state", "[edit]") {
    UndoRedoStack stack;
    CHECK(!stack.CanUndo());
    CHECK(!stack.CanRedo());

    int val = 0;
    stack.Execute(std::make_unique<TestCommand>(val, 10));
    CHECK(stack.CanUndo());
    CHECK(!stack.CanRedo());

    stack.Undo();
    CHECK(!stack.CanUndo());
    CHECK(stack.CanRedo());
}

// --- EditCommands tests ---

TEST_CASE("MoveCommand Execute/Undo", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("w", {10.0f, 20.0f}));

    MoveCommand cmd(reg, "w", {10.0f, 20.0f}, {50.0f, 60.0f});
    cmd.Execute();
    CHECK(reg.GetWidgetState("w")->position == Vec2{50.0f, 60.0f});

    cmd.Undo();
    CHECK(reg.GetWidgetState("w")->position == Vec2{10.0f, 20.0f});
}

TEST_CASE("ResizeCommand Execute/Undo", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("w", {}, {100.0f, 50.0f}));

    ResizeCommand cmd(reg, "w", {100.0f, 50.0f}, {200.0f, 100.0f});
    cmd.Execute();
    CHECK(reg.GetWidgetState("w")->size == Vec2{200.0f, 100.0f});

    cmd.Undo();
    CHECK(reg.GetWidgetState("w")->size == Vec2{100.0f, 50.0f});
}

TEST_CASE("VisibilityCommand Execute/Undo", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("w"));

    VisibilityCommand cmd(reg, "w", true, false);
    cmd.Execute();
    CHECK(reg.GetWidgetState("w")->visible == false);

    cmd.Undo();
    CHECK(reg.GetWidgetState("w")->visible == true);
}

TEST_CASE("GroupMoveCommand Execute/Undo", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("a", {10.0f, 10.0f}));
    reg.Register(MakeManifest("b", {20.0f, 20.0f}));

    std::vector<GroupMoveCommand::Entry> entries = {
        {"a", {10.0f, 10.0f}, {30.0f, 30.0f}},
        {"b", {20.0f, 20.0f}, {40.0f, 40.0f}}
    };

    GroupMoveCommand cmd(reg, entries);
    cmd.Execute();
    CHECK(reg.GetWidgetState("a")->position == Vec2{30.0f, 30.0f});
    CHECK(reg.GetWidgetState("b")->position == Vec2{40.0f, 40.0f});

    cmd.Undo();
    CHECK(reg.GetWidgetState("a")->position == Vec2{10.0f, 10.0f});
    CHECK(reg.GetWidgetState("b")->position == Vec2{20.0f, 20.0f});
}

// --- EditSession tests ---

TEST_CASE("EditSession Enter/Exit transitions", "[edit]") {
    WidgetRegistry reg;
    EditSession session(reg);

    CHECK(session.GetState() == EditModeState::Inactive);
    CHECK(!session.IsActive());

    auto enter = session.Enter();
    REQUIRE(enter.has_value());
    CHECK(session.GetState() == EditModeState::Active);
    CHECK(session.IsActive());

    auto exit = session.Exit();
    REQUIRE(exit.has_value());
    CHECK(session.GetState() == EditModeState::Inactive);
}

TEST_CASE("EditSession Enter when already active returns error", "[edit]") {
    WidgetRegistry reg;
    EditSession session(reg);
    session.Enter();

    auto result = session.Enter();
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::EditModeNotActive);
}

TEST_CASE("EditSession Exit when inactive returns EditModeNotActive", "[edit]") {
    WidgetRegistry reg;
    EditSession session(reg);

    auto result = session.Exit();
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::EditModeNotActive);
}

TEST_CASE("EditSession MoveWidget creates undoable command", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("w", {10.0f, 20.0f}));
    EditSession session(reg);
    session.Enter();

    session.MoveWidget("w", {50.0f, 60.0f});
    CHECK(reg.GetWidgetState("w")->position == Vec2{50.0f, 60.0f});
    CHECK(session.CanUndo());

    session.Undo();
    CHECK(reg.GetWidgetState("w")->position == Vec2{10.0f, 20.0f});

    session.Redo();
    CHECK(reg.GetWidgetState("w")->position == Vec2{50.0f, 60.0f});
}

TEST_CASE("EditSession MoveSelected moves all selected", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("a", {10.0f, 10.0f}));
    reg.Register(MakeManifest("b", {20.0f, 20.0f}));
    reg.Register(MakeManifest("c", {30.0f, 30.0f}));

    EditSession session(reg);
    session.Enter();
    session.Selection().Add("a");
    session.Selection().Add("b");

    session.MoveSelected({5.0f, 5.0f});
    CHECK(reg.GetWidgetState("a")->position == Vec2{15.0f, 15.0f});
    CHECK(reg.GetWidgetState("b")->position == Vec2{25.0f, 25.0f});
    CHECK(reg.GetWidgetState("c")->position == Vec2{30.0f, 30.0f});  // not selected

    session.Undo();
    CHECK(reg.GetWidgetState("a")->position == Vec2{10.0f, 10.0f});
    CHECK(reg.GetWidgetState("b")->position == Vec2{20.0f, 20.0f});
}

TEST_CASE("EditSession operations fail when not active", "[edit]") {
    WidgetRegistry reg;
    reg.Register(MakeManifest("w"));
    EditSession session(reg);

    auto move = session.MoveWidget("w", {0.0f, 0.0f});
    REQUIRE(!move.has_value());
    CHECK(move.error().code == ErrorCode::EditModeNotActive);

    auto resize = session.ResizeWidget("w", {0.0f, 0.0f});
    REQUIRE(!resize.has_value());
    CHECK(resize.error().code == ErrorCode::EditModeNotActive);

    auto toggle = session.ToggleVisibility("w");
    REQUIRE(!toggle.has_value());
    CHECK(toggle.error().code == ErrorCode::EditModeNotActive);

    auto moveSelected = session.MoveSelected({0.0f, 0.0f});
    REQUIRE(!moveSelected.has_value());
    CHECK(moveSelected.error().code == ErrorCode::EditModeNotActive);
}
