#pragma once
#include "ohui/input/InputTypes.h"
#include "ohui/input/InputContext.h"
#include "ohui/core/Result.h"
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ohui::input {

struct RebindEntry {
    std::string contextId;
    std::string actionId;
    KeyCode key{KeyCode::None};
    GamepadButton button{GamepadButton::None};
};

class RebindingStore {
public:
    // Rebind operations
    Result<void> SetKeyBinding(std::string_view contextId, std::string_view actionId, KeyCode key);
    Result<void> SetButtonBinding(std::string_view contextId, std::string_view actionId, GamepadButton button);

    // Conflict detection (within same context)
    bool HasKeyConflict(std::string_view contextId, std::string_view actionId, KeyCode key) const;
    bool HasButtonConflict(std::string_view contextId, std::string_view actionId, GamepadButton button) const;
    std::string GetConflictingAction(std::string_view contextId, KeyCode key) const;
    std::string GetConflictingAction(std::string_view contextId, GamepadButton button) const;

    // Apply stored rebindings to a context
    void ApplyToContext(InputContext& context) const;

    // Reset
    void ResetContext(std::string_view contextId);
    void ResetAll();

    // Query
    bool HasRebinding(std::string_view contextId, std::string_view actionId) const;
    const RebindEntry* GetRebinding(std::string_view contextId, std::string_view actionId) const;
    size_t RebindingCount() const;

    // Cosave serialization (BlockType::InputBindings = 0x0007)
    std::vector<uint8_t> Serialize() const;
    Result<void> Deserialize(std::span<const uint8_t> data);

private:
    // key = "contextId.actionId"
    std::unordered_map<std::string, RebindEntry> m_rebindings;
};

}  // namespace ohui::input
