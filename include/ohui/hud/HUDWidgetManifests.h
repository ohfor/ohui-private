#pragma once

#include "ohui/widget/WidgetTypes.h"

namespace ohui::hud {

inline const widget::WidgetManifest kHealthManifest{
    "ohui_hud_health", "Health", {540.0f, 680.0f}, {200.0f, 16.0f},
    {100.0f, 10.0f}, {400.0f, 40.0f}, true};
inline const widget::WidgetManifest kStaminaManifest{
    "ohui_hud_stamina", "Stamina", {540.0f, 700.0f}, {200.0f, 16.0f},
    {100.0f, 10.0f}, {400.0f, 40.0f}, true};
inline const widget::WidgetManifest kMagickaManifest{
    "ohui_hud_magicka", "Magicka", {540.0f, 660.0f}, {200.0f, 16.0f},
    {100.0f, 10.0f}, {400.0f, 40.0f}, true};
inline const widget::WidgetManifest kCompassManifest{
    "ohui_hud_compass", "Compass", {440.0f, 10.0f}, {400.0f, 30.0f},
    {200.0f, 20.0f}, {600.0f, 50.0f}, true};
inline const widget::WidgetManifest kShoutManifest{
    "ohui_hud_shout", "Shout Cooldown", {565.0f, 720.0f}, {150.0f, 12.0f},
    {80.0f, 8.0f}, {300.0f, 24.0f}, false};
inline const widget::WidgetManifest kDetectionManifest{
    "ohui_hud_detection", "Detection", {600.0f, 620.0f}, {40.0f, 20.0f},
    {30.0f, 15.0f}, {80.0f, 40.0f}, false};
inline const widget::WidgetManifest kEnemyManifest{
    "ohui_hud_enemy", "Enemy Health", {540.0f, 50.0f}, {200.0f, 32.0f},
    {120.0f, 24.0f}, {400.0f, 60.0f}, false};
inline const widget::WidgetManifest kBreathManifest{
    "ohui_hud_breath", "Breath", {565.0f, 640.0f}, {150.0f, 10.0f},
    {80.0f, 6.0f}, {300.0f, 20.0f}, false};
inline const widget::WidgetManifest kNotificationManifest{
    "ohui_hud_notifications", "Notifications", {880.0f, 100.0f}, {300.0f, 40.0f},
    {200.0f, 30.0f}, {500.0f, 60.0f}, true};

inline const widget::WidgetManifest kMessageFeedManifest{
    "ohui_hud_messages", "Messages", {10.0f, 500.0f}, {350.0f, 130.0f},
    {200.0f, 60.0f}, {500.0f, 300.0f}, true};

}  // namespace ohui::hud
