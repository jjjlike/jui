#pragma once
#include "widget.h"
#include "data_model.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace jui {

// ============================================================
// Surface — A2UI 的 Surface 概念
// 每个 Surface 拥有独立的组件树和数据模型
// ============================================================

class Surface {
public:
    explicit Surface(const std::string& id) : id_(id) {}

    const std::string& id() const { return id_; }

    // 组件管理（邻接表模型）
    void addWidget(WidgetPtr widget) {
        widgets_[widget->id()] = widget;
    }

    void removeWidget(const std::string& id) {
        widgets_.erase(id);
    }

    WidgetPtr getWidget(const std::string& id) const {
        auto it = widgets_.find(id);
        return it != widgets_.end() ? it->second : nullptr;
    }

    const std::map<std::string, WidgetPtr>& widgets() const { return widgets_; }

    // 更新组件（A2UI updateComponents）
    void updateComponents(const std::vector<WidgetPtr>& components) {
        for (auto& w : components) {
            widgets_[w->id()] = w;
        }
    }

    // 根组件
    void setRootWidgetId(const std::string& id) { rootWidgetId_ = id; }
    const std::string& rootWidgetId() const { return rootWidgetId_; }
    WidgetPtr rootWidget() const { return getWidget(rootWidgetId_); }

    // 数据模型
    DataModel& dataModel() { return dataModel_; }
    const DataModel& dataModel() const { return dataModel_; }

    // 内容尺寸
    void setContentSize(float w, float h) {
        contentSize_.w = w;
        contentSize_.h = h;
    }
    Size contentSize() const { return contentSize_; }

    // 脏标记
    bool needsLayout() const { return needsLayout_; }
    void setNeedsLayout(bool v) { needsLayout_ = v; }

private:
    std::string id_;
    std::map<std::string, WidgetPtr> widgets_;
    std::string rootWidgetId_;
    DataModel dataModel_;
    Size contentSize_{0, 0};
    bool needsLayout_ = true;
};

using SurfacePtr = std::shared_ptr<Surface>;

} // namespace jui
