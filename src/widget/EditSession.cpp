#include "ohui/widget/EditSession.h"
#include "ohui/widget/EditCommands.h"

#include <memory>
#include <vector>

namespace ohui::widget {

EditSession::EditSession(WidgetRegistry& registry)
    : m_registry(registry) {}

Result<void> EditSession::Enter() {
    if (m_state == EditModeState::Active) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Edit mode is already active"});
    }
    m_state = EditModeState::Active;
    m_undoRedo.Clear();
    m_selection.Clear();
    return {};
}

Result<void> EditSession::Exit() {
    if (m_state == EditModeState::Inactive) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Edit mode is not active"});
    }
    m_state = EditModeState::Inactive;
    return {};
}

EditModeState EditSession::GetState() const {
    return m_state;
}

bool EditSession::IsActive() const {
    return m_state == EditModeState::Active;
}

Result<void> EditSession::MoveWidget(std::string_view id, Vec2 newPos) {
    if (!IsActive()) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Cannot move widget: edit mode not active"});
    }

    const auto* state = m_registry.GetWidgetState(id);
    if (!state) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }

    auto cmd = std::make_unique<MoveCommand>(m_registry, std::string(id), state->position, newPos);
    m_undoRedo.Execute(std::move(cmd));
    return {};
}

Result<void> EditSession::ResizeWidget(std::string_view id, Vec2 newSize) {
    if (!IsActive()) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Cannot resize widget: edit mode not active"});
    }

    const auto* state = m_registry.GetWidgetState(id);
    if (!state) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }

    auto cmd = std::make_unique<ResizeCommand>(m_registry, std::string(id), state->size, newSize);
    m_undoRedo.Execute(std::move(cmd));
    return {};
}

Result<void> EditSession::ToggleVisibility(std::string_view id) {
    if (!IsActive()) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Cannot toggle visibility: edit mode not active"});
    }

    const auto* state = m_registry.GetWidgetState(id);
    if (!state) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "Widget not found: " + std::string(id)});
    }

    auto cmd = std::make_unique<VisibilityCommand>(m_registry, std::string(id),
                                                    state->visible, !state->visible);
    m_undoRedo.Execute(std::move(cmd));
    return {};
}

Result<void> EditSession::MoveSelected(Vec2 delta) {
    if (!IsActive()) {
        return std::unexpected(Error{ErrorCode::EditModeNotActive,
            "Cannot move selected: edit mode not active"});
    }

    auto selected = m_selection.GetSelected();
    if (selected.empty()) return {};

    std::vector<GroupMoveCommand::Entry> entries;
    for (const auto& widgetId : selected) {
        const auto* state = m_registry.GetWidgetState(widgetId);
        if (!state) continue;
        entries.push_back({widgetId, state->position,
            Vec2{state->position.x + delta.x, state->position.y + delta.y}});
    }

    if (!entries.empty()) {
        auto cmd = std::make_unique<GroupMoveCommand>(m_registry, std::move(entries));
        m_undoRedo.Execute(std::move(cmd));
    }

    return {};
}

bool EditSession::CanUndo() const {
    return m_undoRedo.CanUndo();
}

bool EditSession::CanRedo() const {
    return m_undoRedo.CanRedo();
}

void EditSession::Undo() {
    m_undoRedo.Undo();
}

void EditSession::Redo() {
    m_undoRedo.Redo();
}

MultiSelectState& EditSession::Selection() {
    return m_selection;
}

const MultiSelectState& EditSession::Selection() const {
    return m_selection;
}

}  // namespace ohui::widget
