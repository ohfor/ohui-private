#pragma once

#include <string_view>

namespace ohui::hud {

inline constexpr std::string_view kHealthBarDef = R"(
    widget ohui_hud_health {
        ResourceBar {
            value: bind(player.health.pct);
            maxValue: 1.0;
            resourceType: health;
            width: 200;
            height: 16;
        }
    }
)";

inline constexpr std::string_view kStaminaBarDef = R"(
    widget ohui_hud_stamina {
        ResourceBar {
            value: bind(player.stamina.pct);
            maxValue: 1.0;
            resourceType: stamina;
            width: 200;
            height: 16;
        }
    }
)";

inline constexpr std::string_view kMagickaBarDef = R"(
    widget ohui_hud_magicka {
        ResourceBar {
            value: bind(player.magicka.pct);
            maxValue: 1.0;
            resourceType: magicka;
            width: 200;
            height: 16;
        }
    }
)";

inline constexpr std::string_view kCompassDef = R"(
    widget ohui_hud_compass {
        CompassRose {
            heading: bind(player.heading);
            width: 400;
            height: 30;
        }
    }
)";

inline constexpr std::string_view kShoutCooldownDef = R"(
    widget ohui_hud_shout {
        ShoutMeter {
            value: bind(player.shout.cooldown.pct);
            maxValue: 1.0;
            segments: bind(equipped.shout.words);
            width: 150;
            height: 12;
        }
    }
)";

inline constexpr std::string_view kDetectionDef = R"(
    widget ohui_hud_detection {
        StealthEye {
            detectionLevel: bind(player.detection.level);
            width: 40;
            height: 20;
        }
    }
)";

inline constexpr std::string_view kEnemyHealthDef = R"(
    widget ohui_hud_enemy {
        Panel {
            flex-direction: column;
            padding: 4;
            Label {
                text: bind(enemy.name);
                fontSize: 14;
                color: #E8E8E8;
                height: 18;
            }
            ValueBar {
                value: bind(enemy.health.pct);
                maxValue: 1.0;
                fillColor: #8B0000;
                width: 200;
                height: 10;
            }
        }
    }
)";

inline constexpr std::string_view kBreathMeterDef = R"(
    widget ohui_hud_breath {
        ValueBar {
            value: bind(player.breath.pct);
            maxValue: 1.0;
            fillColor: #1565C0;
            width: 150;
            height: 10;
        }
    }
)";

inline constexpr std::string_view kNotificationToastDef = R"(
    widget ohui_hud_notifications {
        NotificationToast {
            text: "Notification";
            lifetime: 5.0;
            elapsed: 0.0;
            width: 300;
            height: 40;
        }
    }
)";

inline constexpr std::string_view kMessageFeedDef = R"(
    widget ohui_hud_messages {
        MessageFeed {
            maxVisible: 5;
            fadeInDuration: 0.3;
            fadeOutDuration: 0.5;
            defaultLifetime: 5.0;
            currentTime: bind(hud.realtime);
            itemHeight: 24;
            width: 350;
            height: 130;
        }
    }
)";

}  // namespace ohui::hud
