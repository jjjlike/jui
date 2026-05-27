#include "jui/layout/layout.h"
#include <algorithm>

namespace jui {

// ============================================================
// 布局引擎实现
// ============================================================

std::vector<WidgetPtr> LayoutEngine::getChildWidgets(WidgetPtr parent) {
    // 这里需要从 widget 注册表中获取子组件
    // 实际使用时由 Surface 提供 widget 查找
    // 此处返回空，子类或外部应覆写
    return {};
}

float LayoutEngine::resolveSize(const JValue& val, float total, float fallback) {
    if (val.isNull()) return fallback;
    if (val.isNumber()) return val.asNumber();
    // 百分比支持（如 "50%"）
    if (val.isString()) {
        const auto& s = val.asString();
        if (!s.empty() && s.back() == '%') {
            try { return total * std::stof(s) / 100.0f; }
            catch (...) {}
        }
    }
    return fallback;
}

void LayoutEngine::layout(WidgetPtr root, const Size& containerSize) {
    if (!root) return;
    measuredSizes_.clear();

    // 第一遍：测量
    Size constraint{containerSize.w, containerSize.h};
    measureWidget(root, constraint);

    // 第二遍：排列
    arrangeWidget(root, Rect{0, 0, containerSize.w, containerSize.h});
}

Size LayoutEngine::measureWidget(WidgetPtr widget, const Size& constraint) {
    if (!widget || !widget->visible()) return {0, 0};

    Size size{0, 0};

    // 优先使用显式设置的宽高
    float explicitW = resolveSize(widget->property("width"), constraint.w, -1);
    float explicitH = resolveSize(widget->property("height"), constraint.h, -1);

    switch (widget->type()) {
        case WidgetType::Text:
            // 文本控件：根据内容估算
            size.w = 100; // 默认最小宽度
            size.h = 24;  // 默认高度
            break;
        case WidgetType::Button:
            size.w = 80;
            size.h = 32;
            break;
        case WidgetType::TextField:
            size.w = 150;
            size.h = 28;
            break;
        case WidgetType::CheckBox:
            size.w = 120;
            size.h = 24;
            break;
        case WidgetType::Divider:
            size.w = constraint.w;
            size.h = 1;
            break;
        case WidgetType::Image:
            size.w = 64;
            size.h = 64;
            break;
        case WidgetType::Row:
            size = measureRow(widget, constraint);
            break;
        case WidgetType::Column:
            size = measureColumn(widget, constraint);
            break;
        case WidgetType::Card:
            size = measureCard(widget, constraint);
            break;
        default:
            break;
    }

    // 应用显式尺寸（如果有）
    if (explicitW > 0) size.w = explicitW;
    if (explicitH > 0) size.h = explicitH;

    // 应用约束
    if (constraint.w > 0) size.w = (std::min)(size.w, constraint.w);
    if (constraint.h > 0) size.h = (std::min)(size.h, constraint.h);

    measuredSizes_[widget->id()] = size;
    return size;
}

Size LayoutEngine::measureRow(WidgetPtr widget, const Size& constraint) {
    float totalW = 0;
    float maxH = 0;
    float totalFlex = 0;

    const auto& children = widget->children();
    for (auto& childId : children.explicitList) {
        // 这里需要 Surface 上下文来获取子 Widget
        // 简化处理：由外部传入 widget 查询
    }

    return {totalW, maxH};
}

Size LayoutEngine::measureColumn(WidgetPtr widget, const Size& constraint) {
    float maxW = 0;
    float totalH = 0;

    const auto& children = widget->children();
    for (auto& childId : children.explicitList) {
        // 简化
    }

    return {maxW, totalH};
}

Size LayoutEngine::measureCard(WidgetPtr widget, const Size& constraint) {
    float padding = 16;
    Size content = measureColumn(widget, constraint);
    return {content.w + padding * 2, content.h + padding * 2};
}

void LayoutEngine::arrangeWidget(WidgetPtr widget, const Rect& bounds) {
    if (!widget || !widget->visible()) return;

    widget->setLayoutBounds(bounds);

    switch (widget->type()) {
        case WidgetType::Row:
            arrangeRow(widget, bounds);
            break;
        case WidgetType::Column:
            arrangeColumn(widget, bounds);
            break;
        case WidgetType::Card:
            arrangeCard(widget, bounds);
            break;
        default:
            break;
    }
}

void LayoutEngine::arrangeRow(WidgetPtr widget, const Rect& bounds) {
    float x = bounds.x + widget->property("padding").asNumber();
    const auto& children = widget->children();
    for (auto& childId : children.explicitList) {
        // 简化：需要 Surface 上下文
    }
}

void LayoutEngine::arrangeColumn(WidgetPtr widget, const Rect& bounds) {
    float y = bounds.y + widget->property("padding").asNumber();
    const auto& children = widget->children();
    for (auto& childId : children.explicitList) {
        // 简化：需要 Surface 上下文
    }
}

void LayoutEngine::arrangeCard(WidgetPtr widget, const Rect& bounds) {
    float padding = 16;
    Rect inner = {bounds.x + padding, bounds.y + padding,
                  bounds.w - padding * 2, bounds.h - padding * 2};
    arrangeColumn(widget, inner);
}

} // namespace jui
