#pragma once

#include <memory>
#include <string>

namespace ohui::widget {

enum class EditModeState { Inactive, Active };

class EditCommand {
public:
    virtual ~EditCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string Description() const = 0;
};

}  // namespace ohui::widget
