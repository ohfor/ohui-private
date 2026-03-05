#include "ohui/hud/HUDWidgetManager.h"
#include "ohui/hud/HUDWidgetDefs.h"
#include "ohui/hud/HUDWidgetManifests.h"
#include "ohui/dsl/WidgetParser.h"
#include "ohui/core/Log.h"

#include <algorithm>

namespace ohui::hud {

HUDWidgetManager::HUDWidgetManager(dsl::DSLRuntimeEngine& dslEngine,
                                   widget::WidgetRegistry& widgetRegistry,
                                   binding::DataBindingEngine& bindings)
    : m_dslEngine(dslEngine), m_widgetRegistry(widgetRegistry), m_bindings(bindings) {}

const std::vector<HUDWidgetManager::HUDEntry>& HUDWidgetManager::GetDefaultEntries() {
    static const std::vector<HUDEntry> entries = {
        {"ohui_hud_health",        kHealthBarDef,        kHealthManifest},
        {"ohui_hud_stamina",       kStaminaBarDef,       kStaminaManifest},
        {"ohui_hud_magicka",       kMagickaBarDef,       kMagickaManifest},
        {"ohui_hud_compass",       kCompassDef,          kCompassManifest},
        {"ohui_hud_shout",         kShoutCooldownDef,    kShoutManifest},
        {"ohui_hud_detection",     kDetectionDef,        kDetectionManifest},
        {"ohui_hud_enemy",         kEnemyHealthDef,      kEnemyManifest},
        {"ohui_hud_breath",        kBreathMeterDef,      kBreathManifest},
        {"ohui_hud_notifications", kNotificationToastDef, kNotificationManifest},
    };
    return entries;
}

Result<void> HUDWidgetManager::LoadDefaults() {
    dsl::WidgetParser parser;
    const auto& entries = GetDefaultEntries();

    for (const auto& entry : entries) {
        // Skip if already loaded
        if (m_dslEngine.HasWidget(entry.id)) {
            continue;
        }

        auto parseResult = parser.Parse(entry.dslSource, entry.id);
        if (!parseResult.has_value()) {
            ohui::log::debug("HUDWidgetManager: failed to parse '{}': {}",
                entry.id, parseResult.error().message);
            continue;
        }

        if (parseResult->hasErrors || parseResult->ast.widgets.empty()) {
            ohui::log::debug("HUDWidgetManager: parse errors for '{}'", entry.id);
            continue;
        }

        // Store parse result to keep AST alive (DSLRuntimeEngine holds raw pointers)
        m_parsedDefs.push_back(std::move(*parseResult));
        auto& stored = m_parsedDefs.back();

        auto loadResult = m_dslEngine.LoadWidget(stored.ast.widgets[0]);
        if (!loadResult.has_value()) {
            ohui::log::debug("HUDWidgetManager: failed to load widget '{}': {}",
                entry.id, loadResult.error().message);
            m_parsedDefs.pop_back();
            continue;
        }

        auto regResult = m_widgetRegistry.Register(entry.manifest);
        if (!regResult.has_value()) {
            ohui::log::debug("HUDWidgetManager: failed to register manifest '{}': {}",
                entry.id, regResult.error().message);
            m_parsedDefs.pop_back();
            continue;
        }

        m_loadedIds.push_back(entry.id);
    }

    return {};
}

Result<dsl::DrawCallList> HUDWidgetManager::Evaluate(std::string_view widgetId,
                                                      float /*screenWidth*/, float /*screenHeight*/,
                                                      float deltaTime) {
    const auto* state = m_widgetRegistry.GetWidgetState(widgetId);
    if (!state) {
        return std::unexpected(Error{ErrorCode::WidgetNotFound,
            "HUD widget not registered: " + std::string(widgetId)});
    }

    widget::ViewportRect viewport{
        state->position.x,
        state->position.y,
        state->size.x,
        state->size.y
    };

    return m_dslEngine.Evaluate(widgetId, viewport, deltaTime);
}

std::vector<std::string> HUDWidgetManager::GetWidgetIds() const {
    return m_loadedIds;
}

size_t HUDWidgetManager::WidgetCount() const {
    return m_loadedIds.size();
}

bool HUDWidgetManager::HasWidget(std::string_view id) const {
    return std::find(m_loadedIds.begin(), m_loadedIds.end(), id) != m_loadedIds.end();
}

}  // namespace ohui::hud
