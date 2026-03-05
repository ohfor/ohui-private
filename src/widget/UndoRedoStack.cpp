#include "ohui/widget/UndoRedoStack.h"

namespace ohui::widget {

void UndoRedoStack::Execute(std::unique_ptr<EditCommand> command) {
    command->Execute();
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
}

bool UndoRedoStack::CanUndo() const {
    return !m_undoStack.empty();
}

bool UndoRedoStack::CanRedo() const {
    return !m_redoStack.empty();
}

void UndoRedoStack::Undo() {
    if (m_undoStack.empty()) return;
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    command->Undo();
    m_redoStack.push_back(std::move(command));
}

void UndoRedoStack::Redo() {
    if (m_redoStack.empty()) return;
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    command->Execute();
    m_undoStack.push_back(std::move(command));
}

void UndoRedoStack::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

size_t UndoRedoStack::UndoCount() const {
    return m_undoStack.size();
}

size_t UndoRedoStack::RedoCount() const {
    return m_redoStack.size();
}

}  // namespace ohui::widget
