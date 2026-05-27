#pragma once
#include "jui/core/widget.h"
#include <map>
#include <memory>

namespace jui {

// ============================================================
// LayoutEngine — 布局引擎
// 负责计算所有控件的尺寸和位置
// ============================================================

class LayoutEngine {
public:
    // 布局计算入口
    void layout(WidgetPtr root, const Size& containerSize);

    // 解析尺寸值（公开，方便测试和外部使用）
    static float resolveSize(const JValue& val, float total, float fallback = -1);

private:
    // 测量阶段：自底向上计算每个控件的期望尺寸
    Size measureWidget(WidgetPtr widget, const Size& constraint);

    // 布局阶段：自顶向下分配位置
    void arrangeWidget(WidgetPtr widget, const Rect& bounds);

    // 容器布局方法
    Size measureRow(WidgetPtr widget, const Size& constraint);
    Size measureColumn(WidgetPtr widget, const Size& constraint);
    Size measureCard(WidgetPtr widget, const Size& constraint);

    void arrangeRow(WidgetPtr widget, const Rect& bounds);
    void arrangeColumn(WidgetPtr widget, const Rect& bounds);
    void arrangeCard(WidgetPtr widget, const Rect& bounds);

    // 获取子组件列表
    std::vector<WidgetPtr> getChildWidgets(WidgetPtr parent);

    std::map<std::string, Size> measuredSizes_;
};

} // namespace jui
