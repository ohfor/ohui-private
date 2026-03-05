#pragma once

#include "ohui/widget/EditTypes.h"
#include "ohui/widget/WidgetTypes.h"
#include "ohui/widget/WidgetRegistry.h"

#include <string>
#include <vector>

namespace ohui::widget {

class MoveCommand : public EditCommand {
public:
    MoveCommand(WidgetRegistry& registry, std::string widgetId, Vec2 oldPos, Vec2 newPos);
    void Execute() override;
    void Undo() override;
    std::string Description() const override;

private:
    WidgetRegistry& m_registry;
    std::string m_widgetId;
    Vec2 m_oldPos;
    Vec2 m_newPos;
};

class ResizeCommand : public EditCommand {
public:
    ResizeCommand(WidgetRegistry& registry, std::string widgetId, Vec2 oldSize, Vec2 newSize);
    void Execute() override;
    void Undo() override;
    std::string Description() const override;

private:
    WidgetRegistry& m_registry;
    std::string m_widgetId;
    Vec2 m_oldSize;
    Vec2 m_newSize;
};

class VisibilityCommand : public EditCommand {
public:
    VisibilityCommand(WidgetRegistry& registry, std::string widgetId, bool oldVisible, bool newVisible);
    void Execute() override;
    void Undo() override;
    std::string Description() const override;

private:
    WidgetRegistry& m_registry;
    std::string m_widgetId;
    bool m_oldVisible;
    bool m_newVisible;
};

class GroupMoveCommand : public EditCommand {
public:
    struct Entry {
        std::string widgetId;
        Vec2 oldPos;
        Vec2 newPos;
    };

    GroupMoveCommand(WidgetRegistry& registry, std::vector<Entry> entries);
    void Execute() override;
    void Undo() override;
    std::string Description() const override;

private:
    WidgetRegistry& m_registry;
    std::vector<Entry> m_entries;
};

}  // namespace ohui::widget
