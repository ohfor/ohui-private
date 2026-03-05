#pragma once

#include "ohui/widget/EditTypes.h"
#include "ohui/widget/UndoRedoStack.h"
#include "ohui/widget/MultiSelectState.h"
#include "ohui/widget/WidgetRegistry.h"
#include "ohui/core/Result.h"

namespace ohui::widget {

class EditSession {
public:
    explicit EditSession(WidgetRegistry& registry);

    Result<void> Enter();
    Result<void> Exit();
    EditModeState GetState() const;
    bool IsActive() const;

    Result<void> MoveWidget(std::string_view id, Vec2 newPos);
    Result<void> ResizeWidget(std::string_view id, Vec2 newSize);
    Result<void> ToggleVisibility(std::string_view id);
    Result<void> MoveSelected(Vec2 delta);

    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();

    MultiSelectState& Selection();
    const MultiSelectState& Selection() const;

private:
    WidgetRegistry& m_registry;
    EditModeState m_state{EditModeState::Inactive};
    UndoRedoStack m_undoRedo;
    MultiSelectState m_selection;
};

}  // namespace ohui::widget
