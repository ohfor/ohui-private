#include "ohui/widget/EditCommands.h"

namespace ohui::widget {

// --- MoveCommand ---

MoveCommand::MoveCommand(WidgetRegistry& registry, std::string widgetId, Vec2 oldPos, Vec2 newPos)
    : m_registry(registry), m_widgetId(std::move(widgetId)), m_oldPos(oldPos), m_newPos(newPos) {}

void MoveCommand::Execute() {
    m_registry.Move(m_widgetId, m_newPos);
}

void MoveCommand::Undo() {
    m_registry.Move(m_widgetId, m_oldPos);
}

std::string MoveCommand::Description() const {
    return "Move " + m_widgetId;
}

// --- ResizeCommand ---

ResizeCommand::ResizeCommand(WidgetRegistry& registry, std::string widgetId, Vec2 oldSize, Vec2 newSize)
    : m_registry(registry), m_widgetId(std::move(widgetId)), m_oldSize(oldSize), m_newSize(newSize) {}

void ResizeCommand::Execute() {
    m_registry.Resize(m_widgetId, m_newSize);
}

void ResizeCommand::Undo() {
    m_registry.Resize(m_widgetId, m_oldSize);
}

std::string ResizeCommand::Description() const {
    return "Resize " + m_widgetId;
}

// --- VisibilityCommand ---

VisibilityCommand::VisibilityCommand(WidgetRegistry& registry, std::string widgetId,
                                     bool oldVisible, bool newVisible)
    : m_registry(registry), m_widgetId(std::move(widgetId)),
      m_oldVisible(oldVisible), m_newVisible(newVisible) {}

void VisibilityCommand::Execute() {
    m_registry.SetVisible(m_widgetId, m_newVisible);
}

void VisibilityCommand::Undo() {
    m_registry.SetVisible(m_widgetId, m_oldVisible);
}

std::string VisibilityCommand::Description() const {
    return "Toggle visibility " + m_widgetId;
}

// --- GroupMoveCommand ---

GroupMoveCommand::GroupMoveCommand(WidgetRegistry& registry, std::vector<Entry> entries)
    : m_registry(registry), m_entries(std::move(entries)) {}

void GroupMoveCommand::Execute() {
    for (const auto& entry : m_entries) {
        m_registry.Move(entry.widgetId, entry.newPos);
    }
}

void GroupMoveCommand::Undo() {
    for (const auto& entry : m_entries) {
        m_registry.Move(entry.widgetId, entry.oldPos);
    }
}

std::string GroupMoveCommand::Description() const {
    return "Move " + std::to_string(m_entries.size()) + " widgets";
}

}  // namespace ohui::widget
