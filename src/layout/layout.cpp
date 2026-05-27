#include "jui/layout/layout.h"
#include <algorithm>

namespace jui {

// ============================================================
// 获取子组件
// ============================================================

std::vector<WidgetPtr> LayoutEngine::getChildWidgets(WidgetPtr parent) {
    std::vector<WidgetPtr> result;
    if (!parent || !resolver_) return result;

    const auto& children = parent->children();
    if (children.mode == Children::Explicit) {
        for (auto& childId : children.explicitList) {
            auto child = resolver_(childId);
            if (child) result.push_back(child);
        }
    }
    return result;
}

// ============================================================
// resolveSize
// ============================================================

float LayoutEngine::resolveSize(const JValue& val, float total, float fallback) {
    if (val.isNull()) return fallback;
    if (val.isNumber()) return val.asNumber();
    if (val.isString()) {
        const auto& s = val.asString();
        if (!s.empty() && s.back() == '%') {
            try { return total * std::stof(s) / 100.0f; }
            catch (...) {}
        }
    }
    return fallback;
}

// ============================================================
// layout — 入口
// ============================================================

void LayoutEngine::layout(WidgetPtr root, const Size& containerSize,
                           WidgetResolver resolver) {
    if (!root) return;

    resolver_ = std::move(resolver);
    measuredSizes_.clear();

    // 第一遍：测量（自底向上）
    measureWidget(root, {containerSize.w, containerSize.h});

    // 第二遍：排列（自顶向下）
    arrangeWidget(root, Rect{0, 0, containerSize.w, containerSize.h});
}

// ============================================================
// 估算文本宽度（中文约 1em/字，英文约 0.6em/字）
// ============================================================

static float estimateTextWidth(const std::string& text, float fontSize) {
    if (text.empty()) return 0;
    float total = 0;
    for (size_t i = 0; i < text.size(); ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) {
            total += fontSize * 0.6f; // ASCII 字符
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            total += fontSize * 0.9f; // 2-byte UTF-8
            i += 2;
        } else {
            total += fontSize * 1.0f; // 3/4-byte UTF-8 (中文等)
            i += 3;
        }
    }
    return total;
}

static float estimateTextHeight(float fontSize) {
    return fontSize * 1.6f;
}

// ============================================================
// measureWidget — 测量阶段
// ============================================================

Size LayoutEngine::measureWidget(WidgetPtr widget, const Size& constraint) {
    if (!widget || !widget->visible()) return {0, 0};

    Size size{0, 0};

    float explicitW = resolveSize(widget->property("width"), constraint.w, -1);
    float explicitH = resolveSize(widget->property("height"), constraint.h, -1);

    switch (widget->type()) {
        case WidgetType::Text: {
            std::string text;
            auto tv = widget->property("text");
            if (tv.isString()) text = tv.asString();

            float fontSize = widget->property("fontSize").isNumber()
                             ? widget->property("fontSize").asNumber() : 14.0f;

            float textW = estimateTextWidth(text, fontSize) + 8; // 加些留白
            size.w = std::max(40.0f, textW);
            size.h = estimateTextHeight(fontSize);
            break;
        }
        case WidgetType::Button: {
            std::string text;
            auto tv = widget->property("text");
            if (tv.isString()) text = tv.asString();

            float textW = estimateTextWidth(text, 14) + 24;
            size.w = std::max(60.0f, textW);
            size.h = 32;
            break;
        }
        case WidgetType::TextField:
            size.w = 150;
            size.h = 28;
            break;
        case WidgetType::CheckBox: {
            std::string label;
            auto lv = widget->property("text");
            if (lv.isString()) label = lv.asString();
            float labelW = estimateTextWidth(label, 13) + 32;
            size.w = std::max(100.0f, labelW);
            size.h = 24;
            break;
        }
        case WidgetType::Divider:
            size.w = constraint.w;
            size.h = 12; // 分割线上下留 12px 间距
            break;
        case WidgetType::Image:
            size.w = explicitW > 0 ? explicitW : 64;
            size.h = explicitH > 0 ? explicitH : 64;
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
        case WidgetType::List:
            size = measureColumn(widget, constraint); // List 内部 Column 布局
            size.h = std::max(size.h, 100.0f);
            break;
        default:
            break;
    }

    if (explicitW > 0) size.w = explicitW;
    if (explicitH > 0) size.h = explicitH;

    if (constraint.w > 0) size.w = (std::min)(size.w, constraint.w);
    if (constraint.h > 0) size.h = (std::min)(size.h, constraint.h);

    measuredSizes_[widget->id()] = size;
    return size;
}

// ============================================================
// measureRow
// ============================================================

Size LayoutEngine::measureRow(WidgetPtr widget, const Size& constraint) {
    float totalW = 0;
    float maxH = 0;

    auto children = getChildWidgets(widget);
    for (auto& child : children) {
        auto childSize = measureWidget(child, constraint);
        totalW += childSize.w;
        maxH = (std::max)(maxH, childSize.h);
    }

    return {totalW, maxH};
}

// ============================================================
// measureColumn
// ============================================================

Size LayoutEngine::measureColumn(WidgetPtr widget, const Size& constraint) {
    float maxW = 0;
    float totalH = 0;

    auto children = getChildWidgets(widget);
    for (auto& child : children) {
        auto childSize = measureWidget(child, constraint);
        maxW = (std::max)(maxW, childSize.w);
        totalH += childSize.h;
    }

    return {maxW, totalH};
}

// ============================================================
// measureCard
// ============================================================

Size LayoutEngine::measureCard(WidgetPtr widget, const Size& constraint) {
    float padding = 16;
    Size content = measureColumn(widget, constraint);
    return {content.w + padding * 2, content.h + padding * 2};
}

// ============================================================
// arrangeWidget — 布局阶段
// ============================================================

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
        case WidgetType::List:
            arrangeColumn(widget, bounds);
            break;
        default:
            break;
    }
}

// ============================================================
// arrangeRow — 水平排列，子元素垂直居中
// ============================================================

void LayoutEngine::arrangeRow(WidgetPtr widget, const Rect& bounds) {
    float x = bounds.x;
    auto children = getChildWidgets(widget);

    for (auto& child : children) {
        if (!child || !child->visible()) continue;

        auto it = measuredSizes_.find(child->id());
        Size childSize{0, 0};
        if (it != measuredSizes_.end()) childSize = it->second;

        // 垂直居中
        float childY = bounds.y + (bounds.h - childSize.h) / 2;
        Rect childBounds{x, childY, childSize.w, childSize.h};
        arrangeWidget(child, childBounds);
        x += childSize.w;
    }
}

// ============================================================
// arrangeColumn — 垂直排列，子元素占满列宽
// ============================================================

void LayoutEngine::arrangeColumn(WidgetPtr widget, const Rect& bounds) {
    float y = bounds.y;
    auto children = getChildWidgets(widget);

    for (auto& child : children) {
        if (!child || !child->visible()) continue;

        auto it = measuredSizes_.find(child->id());
        Size childSize{0, 0};
        if (it != measuredSizes_.end()) childSize = it->second;

        // 关键修复: Column 子元素宽度 = 列宽，防止互相重叠
        // 高度使用测量值，x 左对齐
        Rect childBounds{bounds.x, y, bounds.w, childSize.h};
        arrangeWidget(child, childBounds);
        y += childSize.h;
    }
}

// ============================================================
// arrangeCard
// ============================================================

void LayoutEngine::arrangeCard(WidgetPtr widget, const Rect& bounds) {
    float padding = 16;
    Rect inner = {bounds.x + padding, bounds.y + padding,
                  bounds.w - padding * 2, bounds.h - padding * 2};
    arrangeColumn(widget, inner);
}

} // namespace jui
