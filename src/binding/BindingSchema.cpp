#include "ohui/binding/BindingSchema.h"
#include "ohui/core/Log.h"

namespace ohui::binding {

static const std::vector<SchemaEntry>& BuildSchema() {
    static const std::vector<SchemaEntry> schema = {
        // --- Player Vitals (15 Float) ---
        {"player.health",              BindingType::Float,  "Current health",                  PollRateCategory::PerFrame},
        {"player.health.max",          BindingType::Float,  "Maximum health",                  PollRateCategory::ThrottledSlow},
        {"player.health.pct",          BindingType::Float,  "Health percentage 0-1",            PollRateCategory::PerFrame},
        {"player.magicka",             BindingType::Float,  "Current magicka",                 PollRateCategory::PerFrame},
        {"player.magicka.max",         BindingType::Float,  "Maximum magicka",                 PollRateCategory::ThrottledSlow},
        {"player.magicka.pct",         BindingType::Float,  "Magicka percentage 0-1",           PollRateCategory::PerFrame},
        {"player.stamina",             BindingType::Float,  "Current stamina",                 PollRateCategory::PerFrame},
        {"player.stamina.max",         BindingType::Float,  "Maximum stamina",                 PollRateCategory::ThrottledSlow},
        {"player.stamina.pct",         BindingType::Float,  "Stamina percentage 0-1",           PollRateCategory::PerFrame},
        {"player.carry.weight",        BindingType::Float,  "Current carry weight",            PollRateCategory::ThrottledFast},
        {"player.carry.max",           BindingType::Float,  "Maximum carry weight",            PollRateCategory::ThrottledSlow},
        {"player.carry.pct",           BindingType::Float,  "Carry weight percentage 0-1",      PollRateCategory::ThrottledFast},
        {"player.shout.cooldown",      BindingType::Float,  "Remaining shout cooldown seconds", PollRateCategory::PerFrame},
        {"player.shout.cooldown.pct",  BindingType::Float,  "Shout cooldown percentage 0-1",    PollRateCategory::PerFrame},
        {"player.enchant.charge",      BindingType::Float,  "Weapon enchantment charge 0-1",    PollRateCategory::ThrottledFast},

        // --- Player Stats (12 Int) ---
        {"player.level",               BindingType::Int,    "Player level",                    PollRateCategory::OnEvent},
        {"player.gold",                BindingType::Int,    "Gold amount",                     PollRateCategory::ThrottledSlow},
        {"player.lockpicks",           BindingType::Int,    "Lockpick count",                  PollRateCategory::ThrottledSlow},
        {"player.arrows",              BindingType::Int,    "Equipped arrow count",            PollRateCategory::ThrottledFast},
        {"player.bounty",              BindingType::Int,    "Active bounty",                   PollRateCategory::ThrottledSlow},
        {"player.dragon.souls",        BindingType::Int,    "Unspent dragon souls",            PollRateCategory::OnEvent},
        {"player.perk.points",         BindingType::Int,    "Unspent perk points",             PollRateCategory::OnEvent},
        {"player.vampirism.stage",     BindingType::Int,    "Vampirism stage 0-4",             PollRateCategory::ThrottledSlow},
        {"player.lycanthropy.stage",   BindingType::Int,    "Lycanthropy stage",               PollRateCategory::ThrottledSlow},
        {"player.pickpocket.chance",   BindingType::Int,    "Pickpocket success percentage",   PollRateCategory::ThrottledSlow},
        {"player.crime.bounty.total",  BindingType::Int,    "Total bounty across holds",       PollRateCategory::ThrottledSlow},
        {"player.quests.active",       BindingType::Int,    "Active quest count",              PollRateCategory::ThrottledSlow},

        // --- Skills (18 Float) ---
        {"skill.onehanded",            BindingType::Float,  "One-Handed skill level",          PollRateCategory::OnEvent},
        {"skill.twohanded",            BindingType::Float,  "Two-Handed skill level",          PollRateCategory::OnEvent},
        {"skill.archery",              BindingType::Float,  "Archery skill level",             PollRateCategory::OnEvent},
        {"skill.block",                BindingType::Float,  "Block skill level",               PollRateCategory::OnEvent},
        {"skill.smithing",             BindingType::Float,  "Smithing skill level",            PollRateCategory::OnEvent},
        {"skill.heavyarmor",           BindingType::Float,  "Heavy Armor skill level",         PollRateCategory::OnEvent},
        {"skill.lightarmor",           BindingType::Float,  "Light Armor skill level",         PollRateCategory::OnEvent},
        {"skill.pickpocket",           BindingType::Float,  "Pickpocket skill level",          PollRateCategory::OnEvent},
        {"skill.lockpicking",          BindingType::Float,  "Lockpicking skill level",         PollRateCategory::OnEvent},
        {"skill.sneak",                BindingType::Float,  "Sneak skill level",               PollRateCategory::OnEvent},
        {"skill.alchemy",              BindingType::Float,  "Alchemy skill level",             PollRateCategory::OnEvent},
        {"skill.speech",               BindingType::Float,  "Speech skill level",              PollRateCategory::OnEvent},
        {"skill.alteration",           BindingType::Float,  "Alteration skill level",          PollRateCategory::OnEvent},
        {"skill.conjuration",          BindingType::Float,  "Conjuration skill level",         PollRateCategory::OnEvent},
        {"skill.destruction",          BindingType::Float,  "Destruction skill level",         PollRateCategory::OnEvent},
        {"skill.illusion",             BindingType::Float,  "Illusion skill level",            PollRateCategory::OnEvent},
        {"skill.restoration",          BindingType::Float,  "Restoration skill level",         PollRateCategory::OnEvent},
        {"skill.enchanting",           BindingType::Float,  "Enchanting skill level",          PollRateCategory::OnEvent},

        // --- Player State (9 Bool) ---
        {"player.sneaking",            BindingType::Bool,   "Player is sneaking",              PollRateCategory::PerFrame},
        {"player.detected",            BindingType::Bool,   "Player is detected by enemies",   PollRateCategory::PerFrame},
        {"player.in.combat",           BindingType::Bool,   "Player is in combat",             PollRateCategory::PerFrame},
        {"player.weapon.drawn",        BindingType::Bool,   "Weapon is drawn",                 PollRateCategory::PerFrame},
        {"player.on.horse",            BindingType::Bool,   "Player is on horseback",          PollRateCategory::ThrottledFast},
        {"player.swimming",            BindingType::Bool,   "Player is swimming",              PollRateCategory::ThrottledFast},
        {"player.sprinting",           BindingType::Bool,   "Player is sprinting",             PollRateCategory::PerFrame},
        {"player.trespassing",         BindingType::Bool,   "Player is trespassing",           PollRateCategory::ThrottledSlow},
        {"player.is.vampire",          BindingType::Bool,   "Player is a vampire",             PollRateCategory::ThrottledSlow},

        // --- Location (2 String) ---
        {"location.name",              BindingType::String, "Current location name",           PollRateCategory::OnEvent},
        {"location.hold",              BindingType::String, "Current hold name",               PollRateCategory::OnEvent},

        // --- Equipped Items (8 String) ---
        {"equipped.left.name",         BindingType::String, "Left hand item/spell name",       PollRateCategory::OnEvent},
        {"equipped.right.name",        BindingType::String, "Right hand item/spell name",      PollRateCategory::OnEvent},
        {"equipped.shout.name",        BindingType::String, "Equipped shout name",             PollRateCategory::OnEvent},
        {"equipped.head.name",         BindingType::String, "Equipped head armor name",        PollRateCategory::OnEvent},
        {"equipped.chest.name",        BindingType::String, "Equipped chest armor name",       PollRateCategory::OnEvent},
        {"equipped.feet.name",         BindingType::String, "Equipped feet armor name",        PollRateCategory::OnEvent},
        {"equipped.hands.name",        BindingType::String, "Equipped hands armor name",       PollRateCategory::OnEvent},
        {"equipped.ammo.name",         BindingType::String, "Equipped ammo name",              PollRateCategory::OnEvent},

        // --- Enemy/Target (5 mixed) ---
        {"enemy.name",                 BindingType::String, "Current target name",             PollRateCategory::ThrottledFast},
        {"enemy.health.pct",           BindingType::Float,  "Target health percentage 0-1",    PollRateCategory::PerFrame},
        {"enemy.level",                BindingType::Int,    "Target level",                    PollRateCategory::ThrottledFast},
        {"enemy.is.essential",         BindingType::Bool,   "Target is essential",             PollRateCategory::ThrottledFast},
        {"enemy.distance",             BindingType::Float,  "Distance to target",              PollRateCategory::PerFrame},

        // --- Time & World (10 mixed) ---
        {"time.hour",                  BindingType::Float,  "Current hour 0-23.99",            PollRateCategory::ThrottledFast},
        {"time.day",                   BindingType::Int,    "Day of month",                    PollRateCategory::ThrottledSlow},
        {"time.month",                 BindingType::Int,    "Month 1-12",                      PollRateCategory::ThrottledSlow},
        {"time.year",                  BindingType::Int,    "Year in game",                    PollRateCategory::ThrottledSlow},
        {"time.day.name",              BindingType::String, "Day of week name",                PollRateCategory::ThrottledSlow},
        {"time.month.name",            BindingType::String, "Month name",                      PollRateCategory::ThrottledSlow},
        {"weather.current",            BindingType::String, "Current weather type",            PollRateCategory::ThrottledSlow},
        {"weather.temperature",        BindingType::Float,  "Current temperature",             PollRateCategory::ThrottledSlow},
        {"world.name",                 BindingType::String, "Current worldspace name",         PollRateCategory::OnEvent},
        {"world.is.interior",          BindingType::Bool,   "Player is in interior cell",      PollRateCategory::OnEvent},

        // --- HUD Widget Bindings (5 mixed) ---
        {"player.heading",             BindingType::Float,  "Compass heading 0-360",           PollRateCategory::PerFrame},
        {"player.detection.level",     BindingType::Float,  "Detection level 0.0-1.0",         PollRateCategory::PerFrame},
        {"player.breath.pct",          BindingType::Float,  "Breath percentage 0-1",           PollRateCategory::PerFrame},
        {"equipped.shout.words",       BindingType::Int,    "Equipped shout word count 1-3",   PollRateCategory::OnEvent},
        {"player.has.target",          BindingType::Bool,   "Whether enemy target exists",     PollRateCategory::PerFrame},
    };
    return schema;
}

const std::vector<SchemaEntry>& BindingSchema::GetBuiltinBindings() {
    return BuildSchema();
}

size_t BindingSchema::BuiltinBindingCount() {
    return BuildSchema().size();
}

static BindingValue DefaultForType(BindingType type) {
    switch (type) {
        case BindingType::Float:  return 0.0f;
        case BindingType::Int:    return int64_t{0};
        case BindingType::Bool:   return false;
        case BindingType::String: return std::string{};
    }
    return 0.0f;  // unreachable
}

Result<void> BindingSchema::RegisterBuiltinBindings(DataBindingEngine& engine) {
    for (const auto& entry : BuildSchema()) {
        BindingDefinition def{entry.id, entry.type, entry.description};
        auto defaultVal = DefaultForType(entry.type);
        auto result = engine.RegisterBinding(def,
            [defaultVal]() -> BindingValue { return defaultVal; });
        if (!result.has_value()) {
            ohui::log::debug("BindingSchema: failed to register '{}': {}",
                entry.id, result.error().message);
            return result;
        }
    }
    return {};
}

Result<void> BindingSchema::RegisterCustomBinding(
    DataBindingEngine& engine, std::string_view namespacePrefix,
    std::string_view id, BindingType type,
    std::string_view description, PollFunction pollFn) {

    // Validate namespace: id must start with "{namespacePrefix}."
    std::string expectedPrefix = std::string(namespacePrefix) + ".";
    if (id.substr(0, expectedPrefix.size()) != expectedPrefix) {
        return std::unexpected(Error{ErrorCode::InvalidNamespace,
            "Binding ID '" + std::string(id) + "' must start with '" + expectedPrefix + "'"});
    }

    BindingDefinition def{std::string(id), type, std::string(description)};
    return engine.RegisterBinding(def, std::move(pollFn));
}

}  // namespace ohui::binding
