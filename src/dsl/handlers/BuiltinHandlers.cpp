#include "BuiltinHandlers.h"
#include "AtomHandlers.h"
#include "TextHandlers.h"
#include "ValueHandlers.h"
#include "ContainerHandlers.h"
#include "NavigationHandlers.h"
#include "InputHandlers.h"
#include "IndicatorHandlers.h"
#include "MediaHandlers.h"
#include "ListHandlers.h"
#include "EditModeHandlers.h"
#include "HUDHandlers.h"

#include <algorithm>
#include <cmath>

namespace ohui::dsl {

// --- RegisterBuiltins ---

void ComponentRegistry::RegisterBuiltins() {
    Register("Panel", std::make_unique<PanelHandler>());
    Register("Label", std::make_unique<LabelHandler>());
    Register("ValueBar", std::make_unique<ValueBarHandler>());
    Register("Icon", std::make_unique<IconHandler>());
    Register("Image", std::make_unique<ImageHandler>());
    Register("Line", std::make_unique<LineHandler>());
    Register("Divider", std::make_unique<DividerHandler>());
    Register("Viewport", std::make_unique<ViewportPlaceholderHandler>());
    Register("Caption", std::make_unique<CaptionHandler>());
    Register("RichText", std::make_unique<RichTextHandler>());
    Register("LoreText", std::make_unique<LoreTextHandler>());
    Register("StatValue", std::make_unique<StatValueHandler>());
    Register("StatDelta", std::make_unique<StatDeltaHandler>());
    Register("DeltaList", std::make_unique<DeltaListHandler>());
    Register("TimerBar", std::make_unique<TimerBarHandler>());
    Register("CountBadge", std::make_unique<CountBadgeHandler>());
    Register("SplitPanel", std::make_unique<SplitPanelHandler>());
    Register("ScrollPanel", std::make_unique<ScrollPanelHandler>());
    Register("Modal", std::make_unique<ModalHandler>());
    Register("Tooltip", std::make_unique<TooltipHandler>());
    Register("Drawer", std::make_unique<DrawerHandler>());
    Register("TabBar", std::make_unique<TabBarHandler>());
    Register("Breadcrumb", std::make_unique<BreadcrumbHandler>());
    Register("Pagination", std::make_unique<PaginationHandler>());
    Register("TextInput", std::make_unique<TextInputHandler>());
    Register("SearchField", std::make_unique<SearchFieldHandler>());
    Register("Toggle", std::make_unique<ToggleHandler>());
    Register("Slider", std::make_unique<SliderHandler>());
    Register("Dropdown", std::make_unique<DropdownHandler>());
    Register("Stepper", std::make_unique<StepperHandler>());
    Register("ContextMenu", std::make_unique<ContextMenuHandler>());
    Register("StatusBadge", std::make_unique<StatusBadgeHandler>());
    Register("IndicatorDot", std::make_unique<IndicatorDotHandler>());
    Register("AlertBanner", std::make_unique<AlertBannerHandler>());
    Register("CompletionRing", std::make_unique<CompletionRingHandler>());
    Register("Portrait", std::make_unique<PortraitHandler>());
    Register("CharacterViewport", std::make_unique<CharacterViewportHandler>());
    Register("SceneViewport", std::make_unique<SceneViewportHandler>());
    Register("MapViewport", std::make_unique<MapViewportHandler>());
    Register("ListEntry", std::make_unique<ListEntryHandler>());
    Register("ListEntryCompact", std::make_unique<ListEntryCompactHandler>());
    Register("ScrollList", std::make_unique<ScrollListHandler>());
    Register("FacetedList", std::make_unique<FacetedListHandler>());
    Register("WidgetBoundingBox", std::make_unique<WidgetBoundingBoxHandler>());
    Register("AlignmentGuide", std::make_unique<AlignmentGuideHandler>());
    Register("EditModeToolbar", std::make_unique<EditModeToolbarHandler>());
    Register("GridOverlay", std::make_unique<GridOverlayHandler>());
    Register("ResourceBar", std::make_unique<ResourceBarHandler>());
    Register("ShoutMeter", std::make_unique<ShoutMeterHandler>());
    Register("CompassRose", std::make_unique<CompassRoseHandler>());
    Register("DetectionMeter", std::make_unique<DetectionMeterHandler>());
    Register("StealthEye", std::make_unique<StealthEyeHandler>());
    Register("NotificationToast", std::make_unique<NotificationToastHandler>());
}

// --- PanelHandler ---

void PanelHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawRect dr;
    dr.x = ctx.absX;
    dr.y = ctx.absY;
    dr.width = ctx.rect.width;
    dr.height = ctx.rect.height;

    if (auto* p = ctx.FindProp("fillColor")) {
        dr.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    } else if (auto* p2 = ctx.FindProp("backgroundColor")) {
        dr.fillColor = ctx.parseColor(ctx.resolveString(p2->value));
    } else if (auto* p3 = ctx.FindProp("color")) {
        dr.fillColor = ctx.parseColor(ctx.resolveString(p3->value));
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dr.opacity = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("borderWidth")) {
        dr.borderWidth = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("borderColor")) {
        dr.borderColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("borderRadius")) {
        dr.borderRadius = ctx.resolveFloat(p->value);
    }

    output.calls.push_back({DrawCallType::Rect, dr, 0});
}

// --- LabelHandler ---

void LabelHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawText dt;
    dt.x = ctx.absX;
    dt.y = ctx.absY;
    dt.width = ctx.rect.width;
    dt.height = ctx.rect.height;

    if (auto* p = ctx.FindProp("text")) {
        dt.text = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("fontSize")) {
        dt.fontSize = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("fontFamily")) {
        dt.fontFamily = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("color")) {
        dt.color = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("textAlign")) {
        auto val = ctx.resolveString(p->value);
        if (val == "center") dt.align = TextAlign::Center;
        else if (val == "right") dt.align = TextAlign::Right;
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dt.opacity = ctx.resolveFloat(p->value);
    }

    output.calls.push_back({DrawCallType::Text, dt, 0});
}

// --- ValueBarHandler ---

void ValueBarHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    // Background rect
    DrawRect bg;
    bg.x = ctx.absX;
    bg.y = ctx.absY;
    bg.width = ctx.rect.width;
    bg.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("backgroundColor")) {
        bg.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    output.calls.push_back({DrawCallType::Rect, bg, 0});

    // Fill rect
    float value = 0.0f;
    float maxValue = 100.0f;
    if (auto* p = ctx.FindProp("value")) {
        value = ctx.resolveFloat(p->value);
    }
    if (auto* p = ctx.FindProp("maxValue")) {
        maxValue = ctx.resolveFloat(p->value);
    }
    float ratio = (maxValue > 0.0f) ? std::clamp(value / maxValue, 0.0f, 1.0f) : 0.0f;

    DrawRect fill;
    fill.x = ctx.absX;
    fill.y = ctx.absY;
    fill.width = ctx.rect.width * ratio;
    fill.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("fillColor")) {
        fill.fillColor = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("opacity")) {
        fill.opacity = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Rect, fill, 0});
}

// --- IconHandler ---

void IconHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawIcon di;
    di.x = ctx.absX;
    di.y = ctx.absY;
    di.width = ctx.rect.width;
    di.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("source")) {
        di.source = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("tint")) {
        di.tint = ctx.parseColor(ctx.resolveString(p->value));
    }
    if (auto* p = ctx.FindProp("opacity")) {
        di.opacity = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Icon, di, 0});
}

// --- ImageHandler ---

void ImageHandler::Emit(const ComponentContext& ctx, DrawCallList& output) {
    DrawImage dim;
    dim.x = ctx.absX;
    dim.y = ctx.absY;
    dim.width = ctx.rect.width;
    dim.height = ctx.rect.height;
    if (auto* p = ctx.FindProp("source")) {
        dim.source = ctx.resolveString(p->value);
    }
    if (auto* p = ctx.FindProp("opacity")) {
        dim.opacity = ctx.resolveFloat(p->value);
    }
    output.calls.push_back({DrawCallType::Image, dim, 0});
}

}  // namespace ohui::dsl
