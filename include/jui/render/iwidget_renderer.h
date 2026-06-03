#pragma once
#include "jui/core/geometry.h"
#include "jui/core/widget.h"
#include <string>
#include <memory>
#include <map>
#include <functional>

namespace jui::render {

class IRenderTarget;

/// 统一控件渲染接口 — 支持外部自定义渲染逻辑
class IWidgetRenderer {
public:
    virtual ~IWidgetRenderer() = default;

    /// 绘制控件
    virtual void paint(IRenderTarget& rt, const Rect& bounds,
                       const std::string& text, bool enabled,
                       bool hovered, bool pressed, float value) = 0;

    /// 返回支持的控件类型
    virtual WidgetType supportedType() const = 0;

    /// 可读名称（用于注册/调试）
    virtual std::string name() const { return "UnknownRenderer"; }
};

/// 渲染器注册表 — 全局/单控件替换
class RendererRegistry {
public:
    using Ptr = std::shared_ptr<IWidgetRenderer>;

    /// 设置指定控件类型的渲染器
    void set(WidgetType type, Ptr renderer) {
        registry_[static_cast<int>(type)] = std::move(renderer);
    }

    /// 批量设置（全局替换）
    void setAll(Ptr renderer) {
        for (int i = 0; i <= static_cast<int>(WidgetType::Card); ++i)
            registry_[i] = renderer;
        default_ = renderer;
    }

    /// 获取指定类型的渲染器，fallback 到默认
    IWidgetRenderer* get(WidgetType type) const {
        int idx = static_cast<int>(type);
        auto it = registry_.find(idx);
        if (it != registry_.end() && it->second) return it->second.get();
        return default_.get();
    }

    /// 清除所有注册
    void clear() { registry_.clear(); default_.reset(); }

    /// 恢复内置渲染器（返回是否成功）
    bool restoreBuiltin() {
        clear();
        return true;
    }

private:
    std::map<int, Ptr> registry_;
    Ptr default_;
};

} // namespace jui::render
