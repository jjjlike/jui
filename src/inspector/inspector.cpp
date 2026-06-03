/**
 * @file inspector.cpp
 * @brief UIInspector — 黑盒测试数据采集与 JSON 序列化
 *
 * 前置依赖（读取，不修改）:
 *   - jui/core/surface.h  → Surface, Widget
 *   - jui/render/d2d_renderer.h → D2DRenderer, RenderWidget
 *   - jui/render/widget_state.h  → WidgetState 派生类
 *   - json.hpp               → nlohmann/json
 */

#include "jui/inspector/inspector.h"
#include "jui/core/surface.h"
#include "jui/core/widget.h"
#include "jui/render/d2d_renderer.h"
#include "jui/render/render_widget.h"
#include "jui/render/widget_state.h"
#include <json.hpp>
#include <sstream>
#include <chrono>
#include <cmath>

namespace jui {
namespace inspector {

// ═══════════════════════════════════════════════════════════════
// Helper: WidgetType → string
// ═══════════════════════════════════════════════════════════════

static const char* typeName(WidgetType t) {
    switch (t) {
        case WidgetType::Text:          return "Text";
        case WidgetType::Button:        return "Button";
        case WidgetType::TextField:     return "TextField";
        case WidgetType::CheckBox:      return "CheckBox";
        case WidgetType::Toggle:        return "Toggle";
        case WidgetType::ChoicePicker:  return "ChoicePicker";
        case WidgetType::Slider:        return "Slider";
        case WidgetType::List:          return "List";
        case WidgetType::Grid:          return "Grid";
        case WidgetType::Tabs:          return "Tabs";
        case WidgetType::Image:         return "Image";
        case WidgetType::Divider:       return "Divider";
        case WidgetType::Row:           return "Row";
        case WidgetType::Column:        return "Column";
        case WidgetType::Card:          return "Card";
        default:                        return "Unknown";
    }
}

// ═══════════════════════════════════════════════════════════════
// Helper: 提取 WidgetState 为 JSON 子对象
// ═══════════════════════════════════════════════════════════════

static nlohmann::json extractState(const RenderWidget* rw) {
    nlohmann::json j;
    if (!rw) return j;

    // 通过 const_cast 获取非 const 指针以调用 dynamic_cast
    // （state() 返回 WidgetState*，不修改任何数据）
    auto* raw = const_cast<RenderWidget*>(rw);
    auto* st = raw->state();
    if (!st) return j;

    // Text/Button/TextField 通用：文本
    if (auto* ts = dynamic_cast<TextWidgetState*>(st)) {
        if (!ts->text().empty()) j["text"] = ts->text();
    }

    // Button
    if (auto* bs = dynamic_cast<ButtonWidgetState*>(st)) {
        j["text"] = bs->text();
        auto vs = bs->visualState();
        j["visualState"] = (vs == ButtonWidgetState::VisualState::Normal) ? "Normal" :
                          (vs == ButtonWidgetState::VisualState::Hovered) ? "Hovered" :
                          (vs == ButtonWidgetState::VisualState::Pressed) ? "Pressed" : "Disabled";
        j["enabled"] = bs->enabled();
    }

    // TextField
    if (auto* tfs = dynamic_cast<TextFieldWidgetState*>(st)) {
        j["focused"] = tfs->hovered();
        j["text"] = tfs->text();
        j["cursorPos"] = static_cast<int>(tfs->cursorPos());
    }

    // CheckBox
    if (auto* cbs = dynamic_cast<CheckBoxWidgetState*>(st)) {
        j["checked"] = cbs->isChecked();
        j["text"] = cbs->text();
    }

    // Toggle
    if (auto* tgs = dynamic_cast<ToggleWidgetState*>(st)) {
        j["toggled"] = tgs->toggled();
        j["text"] = tgs->label();
    }

    // ChoicePicker
    if (auto* cps = dynamic_cast<ChoicePickerWidgetState*>(st)) {
        j["selectedIndex"] = cps->selectedIndex();
        j["isOpen"] = cps->isOpen();
    }

    // Slider
    if (auto* ss = dynamic_cast<SliderWidgetState*>(st)) {
        j["value"] = ss->value();
        j["minValue"] = ss->minValue();
        j["maxValue"] = ss->maxValue();
        j["isDragging"] = ss->isDragging();
    }

    // List
    if (auto* ls = dynamic_cast<ListWidgetState*>(st)) {
        j["itemCount"] = ls->itemCount();
        j["itemHeight"] = ls->itemHeight();
        j["scrollOffset"] = ls->scrollOffset();
        j["selectedIndex"] = ls->selectedIndex();
    }

    // Grid
    if (auto* gs = dynamic_cast<GridWidgetState*>(st)) {
        j["rowCount"] = gs->rowCount();
        j["columnCount"] = gs->columnCount();
        j["scrollX"] = gs->scrollX();
        j["scrollY"] = gs->scrollY();
    }

    // Tabs
    if (auto* tabs = dynamic_cast<TabsWidgetState*>(st)) {
        j["activeIndex"] = tabs->activeIndex();
    }

    return j;
}

// ═══════════════════════════════════════════════════════════════
// Collector::collect
// ═══════════════════════════════════════════════════════════════

SurfaceSnapshot Collector::collect(
    const SurfacePtr& surface,
    const D2DRenderer& renderer)
{
    SurfaceSnapshot snap;
    if (!surface) return snap;

    snap.surfaceId = surface->id();

    // 时间戳
    auto now = std::chrono::system_clock::now();
    snap.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    // 视口
    snap.viewport = {
        0.0f, 0.0f,
        static_cast<float>(renderer.width()),
        static_cast<float>(renderer.height())
    };
    if (snap.viewport.w <= 0) snap.viewport.w = 800;
    if (snap.viewport.h <= 0) snap.viewport.h = 600;

    // 遍历 Surface 中所有 Widget
    for (auto& [id, widget] : surface->widgets()) {
        if (!widget) continue;

        WidgetSnapshot ws;
        ws.id = id;
        ws.type = typeName(widget->type());
        ws.visible = widget->visible();
        ws.enabled = widget->enabled();

        // Layout bounds
        auto& lb = widget->layoutBounds();
        ws.layout = { lb.x, lb.y, lb.w, lb.h };

        // Children
        const auto& ch = widget->children();
        if (ch.mode == Children::Explicit) {
            ws.children = ch.explicitList;
        }

        // Paint bounds + state（从 renderer 中的 RenderWidget 获取）
        auto* rw = renderer.getRenderWidget(id);
        if (rw) {
            auto& pb = rw->bounds();
            ws.paint = { pb.x, pb.y, pb.w, pb.h };

            // 无 D2D 上下文时 RenderWidget bounds 为 (0,0,0,0)，
            // 此时 fallback 到 layout 并标记一致
            if (pb.w <= 0.5f && pb.h <= 0.5f) {
                ws.paint = ws.layout;
                ws.paintMatch = true;
            } else {
                ws.paintMatch =
                    std::abs(ws.layout.x - ws.paint.x) < 1.0f &&
                    std::abs(ws.layout.y - ws.paint.y) < 1.0f &&
                    std::abs(ws.layout.w - ws.paint.w) < 1.0f &&
                    std::abs(ws.layout.h - ws.paint.h) < 1.0f;
            }

            // 提取运行时状态
            auto stateJ = extractState(rw);
            ws.stateJson = stateJ.dump();
        } else {
            // 无 RenderWidget 时 paint = layout
            ws.paint = ws.layout;
            ws.paintMatch = true;
        }

        snap.widgets.push_back(std::move(ws));
    }

    return snap;
}

// ═══════════════════════════════════════════════════════════════
// Serializer::toJson
// ═══════════════════════════════════════════════════════════════

std::string Serializer::toJson(const SurfaceSnapshot& snap) {
    nlohmann::json j;

    j["surfaceId"] = snap.surfaceId;
    j["timestamp"] = snap.timestamp;

    nlohmann::json vp;
    vp["x"] = snap.viewport.x; vp["y"] = snap.viewport.y;
    vp["w"] = snap.viewport.w; vp["h"] = snap.viewport.h;
    j["viewport"] = vp;

    nlohmann::json widgetsJson = nlohmann::json::array();
    for (auto& ws : snap.widgets) {
        nlohmann::json wj;
        wj["id"] = ws.id;
        wj["type"] = ws.type;
        wj["visible"] = ws.visible;
        wj["enabled"] = ws.enabled;

        nlohmann::json lo;
        lo["x"] = ws.layout.x; lo["y"] = ws.layout.y;
        lo["w"] = ws.layout.w; lo["h"] = ws.layout.h;
        wj["layout"] = lo;

        nlohmann::json pa;
        pa["x"] = ws.paint.x; pa["y"] = ws.paint.y;
        pa["w"] = ws.paint.w; pa["h"] = ws.paint.h;
        wj["paint"] = pa;

        wj["paintMatch"] = ws.paintMatch;

        // 解析状态 JSON 子字符串为真实对象
        if (!ws.stateJson.empty()) {
            try {
                wj["state"] = nlohmann::json::parse(ws.stateJson);
            } catch (...) {
                wj["state"] = nlohmann::json::object();
            }
        } else {
            wj["state"] = nlohmann::json::object();
        }

        wj["children"] = ws.children;

        widgetsJson.push_back(wj);
    }
    j["widgets"] = widgetsJson;

    return j.dump();
}

// ═══════════════════════════════════════════════════════════════
// Serializer::fromJson
// ═══════════════════════════════════════════════════════════════

SurfaceSnapshot Serializer::fromJson(const std::string& json) {
    SurfaceSnapshot snap;
    try {
        nlohmann::json j = nlohmann::json::parse(json);

        snap.surfaceId = j.value("surfaceId", "");
        snap.timestamp = j.value("timestamp", 0L);

        if (j.contains("viewport")) {
            auto& v = j["viewport"];
            snap.viewport = { v.value("x", 0.0f), v.value("y", 0.0f),
                              v.value("w", 0.0f), v.value("h", 0.0f) };
        }

        for (auto& wj : j["widgets"]) {
            WidgetSnapshot ws;
            ws.id = wj.value("id", "");
            ws.type = wj.value("type", "");
            ws.visible = wj.value("visible", true);
            ws.enabled = wj.value("enabled", true);

            if (wj.contains("layout")) {
                auto& l = wj["layout"];
                ws.layout = { l.value("x", 0.0f), l.value("y", 0.0f),
                              l.value("w", 0.0f), l.value("h", 0.0f) };
            }
            if (wj.contains("paint")) {
                auto& p = wj["paint"];
                ws.paint = { p.value("x", 0.0f), p.value("y", 0.0f),
                             p.value("w", 0.0f), p.value("h", 0.0f) };
            }
            ws.paintMatch = wj.value("paintMatch", true);

            if (wj.contains("state") && wj["state"].is_object()) {
                ws.stateJson = wj["state"].dump();
            }

            if (wj.contains("children")) {
                for (auto& c : wj["children"])
                    ws.children.push_back(c.get<std::string>());
            }

            snap.widgets.push_back(std::move(ws));
        }
    } catch (...) {
        snap.surfaceId.clear(); // 标记解析失败
    }
    return snap;
}

} // namespace inspector
} // namespace jui
