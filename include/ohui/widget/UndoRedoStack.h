#pragma once

#include "ohui/widget/EditTypes.h"

#include <memory>
#include <vector>

namespace ohui::widget {

class UndoRedoStack {
public:
    void Execute(std::unique_ptr<EditCommand> command);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void Clear();
    size_t UndoCount() const;
    size_t RedoCount() const;

private:
    std::vector<std::unique_ptr<EditCommand>> m_undoStack;
    std::vector<std::unique_ptr<EditCommand>> m_redoStack;
};

}  // namespace ohui::widget
