#pragma once

#include <string_view>

namespace ohui::dsl {

inline constexpr std::string_view kDefaultTokensUSS = R"(
:root {
    /* Colour - Text */
    --color-text-primary: #E8E8E8;
    --color-text-secondary: #A0A0A0;
    --color-text-disabled: #606060;
    --color-text-positive: #4CAF50;
    --color-text-negative: #F44336;
    --color-text-warning: #FFC107;
    --color-text-critical: #FF1744;
    --color-text-lore: #D4A574;

    /* Colour - Surface */
    --color-surface-base: #1A1A1AE6;
    --color-surface-raised: #2A2A2AE6;
    --color-surface-overlay: #000000CC;
    --color-surface-input: #0D0D0D;

    /* Colour - Border */
    --color-border-default: #404040;
    --color-border-focus: #8090A0;
    --color-border-subtle: #2A2A2A;

    /* Colour - Accent */
    --color-accent-primary: #8090A0;
    --color-accent-secondary: #607080;

    /* Colour - Resource */
    --color-resource-health: #8B0000;
    --color-resource-stamina: #2E7D32;
    --color-resource-magicka: #1565C0;
    --color-resource-xp: #D4A017;

    /* Colour - Status */
    --color-status-stolen: #FF1744;
    --color-status-quest: #FFD54F;
    --color-status-enchanted: #7C4DFF;
    --color-status-museum-needed: #42A5F5;
    --color-status-museum-donated: #66BB6A;

    /* Spacing */
    --spacing-xs: 2;
    --spacing-sm: 4;
    --spacing-md: 8;
    --spacing-lg: 16;
    --spacing-xl: 24;
    --spacing-xxl: 32;

    /* Typography - Size */
    --font-size-xs: 10;
    --font-size-sm: 12;
    --font-size-md: 14;
    --font-size-lg: 18;
    --font-size-xl: 24;
    --font-size-display: 32;

    /* Typography - Weight */
    --font-weight-regular: 400;
    --font-weight-medium: 500;
    --font-weight-bold: 700;

    /* Typography - Family */
    --font-family-ui: Futura Condensed;
    --font-family-lore: Garamond;

    /* Shape */
    --radius-sm: 2;
    --radius-md: 4;
    --radius-lg: 8;
    --radius-pill: 9999;
}
)";

}  // namespace ohui::dsl
