#pragma once
#include "ohui/input/InputContext.h"
#include "ohui/core/Result.h"
#include <memory>
#include <vector>

namespace ohui::input {

class InputContextStack {
public:
    InputContextStack();

    // Context management
    Result<void> Push(std::shared_ptr<InputContext> context);
    Result<void> Pop(std::string_view contextId);
    InputContext* GetActiveContext();
    const InputContext* GetActiveContext() const;

    // Input routing -- dispatches to top context only
    bool HandleInput(const InputEvent& event);

    // Stack queries
    size_t Depth() const;
    bool HasContext(std::string_view contextId) const;
    const InputContext* FindContext(std::string_view contextId) const;
    std::vector<std::string> GetStackOrder() const;

    // The gameplay context (always at bottom, never popped)
    InputContext& GetGameplayContext();

    static constexpr const char* kGameplayContextId = "gameplay";

private:
    std::shared_ptr<InputContext> m_gameplayContext;
    std::vector<std::shared_ptr<InputContext>> m_stack;  // bottom to top
};

}  // namespace ohui::input
