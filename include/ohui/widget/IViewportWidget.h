#pragma once

#include "ohui/widget/WidgetTypes.h"

#include <cstdint>

namespace ohui::widget {

struct ViewportRect {
    float left{};
    float top{};
    float width{};
    float height{};
    bool operator==(const ViewportRect&) const = default;
};

enum class ViewportNotification {
    Activated,
    Deactivated,
    Resized,
    Moved,
    VisibilityChanged,
    EditModeEntered,
    EditModeExited,
};

struct ViewportEvent {
    ViewportNotification type;
    ViewportRect viewport;
    bool visible{true};
};

class IViewportWidget {
public:
    virtual ~IViewportWidget() = default;
    virtual void Render(const ViewportRect& viewport) = 0;
    virtual void OnViewportEvent(const ViewportEvent& event) = 0;
    virtual const WidgetManifest& GetManifest() const = 0;
};

}  // namespace ohui::widget
