#pragma once
#include "jui/core/geometry.h"

namespace jui::core { class IWidgetTree; class IWidgetNode; }

namespace jui::layout {

class ILayoutEngine {
public:
    virtual ~ILayoutEngine() = default;
    virtual void layout(core::IWidgetTree& tree, const Size& container) = 0;
    virtual Size measure(core::IWidgetNode& node, const Size& constraint) = 0;
};

} // namespace jui::layout
