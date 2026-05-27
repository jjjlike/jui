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
// layout
// ============================================================

void LayoutEngine::layout(WidgetPtr root, const Size& containerSize,
                           WidgetResolver resolver) {
    if (!root) return;
    resolver_ = std::move(resolver);
    measuredSizes_.clear();
    measureWidget(root, {containerSize.w, containerSize.h});
    arrangeWidget(root, Rect{0, 0, containerSize.w, containerSize.h});
}

// ============================================================
// DirectWrite 文本测量估算（供布局引擎在无 DWrite 上下文时使用）
// ============================================================

static float estimateTextWidth(const std::string& text, float fontSize) {
    if (text.empty()) return 0;
    float total = 0;
    for (size_t i = 0; i < text.size(); ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) { total += fontSize * 0.6f; i += 1; }
        else if ((c & 0xE0) == 0xC0) { total += fontSize * 0.9f; i += 2; }
        else { total += fontSize * 1.0f; i += 3; }
    }
    return total;
}

// ============================================================
// measureWidget
// ============================================================

Size LayoutEngine::measureWidget(WidgetPtr widget, const Size& constraint) {
    if (!widget || !widget->visible()) return {0, 0};

    Size size{0, 0};
    float explicitW = resolveSize(widget->property("width"), constraint.w, -1);
    float explicitH = resolveSize(widget->property("height"), constraint.h, -1);

    switch (widget->type()) {
        case WidgetType::Text: {
            std::string text; auto tv = widget->property("text");
            if (tv.isString()) text = tv.asString();
            float fs = widget->property("fontSize").isNumber() ? widget->property("fontSize").asNumber() : 14.0f;
            float tw = estimateTextWidth(text, fs);
            size.w = std::max(40.0f, tw + 8);
            size.h = std::max(18.0f, fs * 1.5f);
            break;
        }
        case WidgetType::Button: {
            std::string text; auto tv = widget->property("text");
            if (tv.isString()) text = tv.asString();
            float tw = estimateTextWidth(text, 14);
            size.w = std::max(70.0f, tw + 24);
            size.h = 32;
            break;
        }
        case WidgetType::TextField:
            size.w = 150; size.h = 30;
            break;
        case WidgetType::CheckBox: {
            std::string label; auto lv = widget->property("text");
            if (lv.isString()) label = lv.asString();
            float lw = estimateTextWidth(label, 13);
            size.w = std::max(100.0f, lw + 28);
            size.h = 24;
            break;
        }
        case WidgetType::Toggle:
            size.w = 60; size.h = 22;
            break;
        case WidgetType::ChoicePicker:
            size.w = 120; size.h = 30;
            break;
        case WidgetType::Slider:
            size.w = 150; size.h = 24;
            break;
        case WidgetType::Divider:
            size.w = constraint.w; size.h = 12;
            break;
        case WidgetType::Image:
            size.w = explicitW > 0 ? explicitW : 64;
            size.h = explicitH > 0 ? explicitH : 64;
            break;
        case WidgetType::Row: size = measureRow(widget, constraint); break;
        case WidgetType::Column: size = measureColumn(widget, constraint); break;
        case WidgetType::Card: size = measureCard(widget, constraint); break;
        case WidgetType::List: size = measureColumn(widget, constraint); size.h = std::max(size.h, 100.0f); break;
        default: break;
    }

    if (explicitW > 0) size.w = explicitW;
    if (explicitH > 0) size.h = explicitH;
    if (constraint.w > 0) size.w = (std::min)(size.w, constraint.w);
    if (constraint.h > 0) size.h = (std::min)(size.h, constraint.h);

    measuredSizes_[widget->id()] = size;
    return size;
}

// ============================================================
// measureRow / measureColumn / measureCard
// ============================================================

Size LayoutEngine::measureRow(WidgetPtr widget, const Size& constraint) {
    float totalW = 0, maxH = 0;
    for (auto& c : getChildWidgets(widget)) {
        auto s = measureWidget(c, constraint);
        totalW += s.w; maxH = (std::max)(maxH, s.h);
    }
    return {totalW, maxH};
}

Size LayoutEngine::measureColumn(WidgetPtr widget, const Size& constraint) {
    float maxW = 0, totalH = 0;
    for (auto& c : getChildWidgets(widget)) {
        auto s = measureWidget(c, constraint);
        maxW = (std::max)(maxW, s.w); totalH += s.h;
    }
    return {maxW, totalH};
}

Size LayoutEngine::measureCard(WidgetPtr widget, const Size& constraint) {
    float pad = 16; Size content = measureColumn(widget, constraint);
    return {content.w + pad * 2, content.h + pad * 2};
}

// ============================================================
// arrangeWidget / arrangeRow / arrangeColumn / arrangeCard
// ============================================================

void LayoutEngine::arrangeWidget(WidgetPtr widget, const Rect& bounds) {
    if (!widget || !widget->visible()) return;
    widget->setLayoutBounds(bounds);
    switch (widget->type()) {
        case WidgetType::Row: arrangeRow(widget, bounds); break;
        case WidgetType::Column: arrangeColumn(widget, bounds); break;
        case WidgetType::Card: arrangeCard(widget, bounds); break;
        case WidgetType::List: arrangeColumn(widget, bounds); break;
        default: break;
    }
}

void LayoutEngine::arrangeRow(WidgetPtr widget, const Rect& bounds) {
    float x = bounds.x;
    for (auto& c : getChildWidgets(widget)) {
        if (!c || !c->visible()) continue;
        auto it = measuredSizes_.find(c->id());
        Size sz{0,0}; if (it != measuredSizes_.end()) sz = it->second;
        Rect cb{x, bounds.y + (bounds.h - sz.h) / 2, sz.w, sz.h};
        arrangeWidget(c, cb); x += sz.w;
    }
}

void LayoutEngine::arrangeColumn(WidgetPtr widget, const Rect& bounds) {
    float y = bounds.y;
    for (auto& c : getChildWidgets(widget)) {
        if (!c || !c->visible()) continue;
        auto it = measuredSizes_.find(c->id());
        Size sz{0,0}; if (it != measuredSizes_.end()) sz = it->second;
        // 子元素宽度 = 列宽（块级布局）
        Rect cb{bounds.x, y, bounds.w, sz.h};
        arrangeWidget(c, cb); y += sz.h;
    }
}

void LayoutEngine::arrangeCard(WidgetPtr widget, const Rect& bounds) {
    float pad = 16;
    Rect inner = {bounds.x + pad, bounds.y + pad, bounds.w - pad * 2, bounds.h - pad * 2};
    arrangeColumn(widget, inner);
}

} // namespace jui
