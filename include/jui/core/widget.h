#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace jui {

// ============================================================
// Widget — 纯逻辑控件（与渲染完全分离）
// 遵循 A2UI 邻接表模型：扁平列表 + ID 引用
// ============================================================

enum class WidgetType {
    Text,
    Button,
    TextField,
    CheckBox,
    Image,
    Row,
    Column,
    Card,
    List,
    Divider,
    Custom
};

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool isEmpty() const { return w <= 0 || h <= 0; }
};

struct Size {
    float w = 0, h = 0;
};

struct Point {
    float x = 0, y = 0;
};

// 样式属性
struct WidgetStyle {
    // 布局
    float width = 0;       // 0=auto
    float height = 0;      // 0=auto
    float flex = 0;        // flex 权重
    float margin[4] = {0}; // top right bottom left
    float padding[4] = {0};

    // 外观
    std::string backgroundColor;
    std::string textColor;
    std::string borderColor;
    float borderRadius = 0;
    float fontSize = 14;
    std::string fontWeight = "normal"; // normal, bold
    std::string textAlign = "left";    // left, center, right
};

// 子元素定义
struct Children {
    enum Mode { Explicit, Template };
    Mode mode = Explicit;
    std::vector<std::string> explicitList;  // 静态子组件 ID 列表
    struct {
        std::string dataBinding;   // 数据路径
        std::string componentId;   // 模板组件 ID
    } templateInfo;
};

// ============================================================
// Widget 基类 — 纯数据，不含任何渲染逻辑
// ============================================================
class Widget {
public:
    explicit Widget(const std::string& id, WidgetType type)
        : id_(id), type_(type) {}
    virtual ~Widget() = default;

    const std::string& id() const { return id_; }
    WidgetType type() const { return type_; }

    // 属性访问
    void setProperty(const std::string& key, const JValue& val) {
        properties_[key] = val;
    }

    const JValue& property(const std::string& key) const {
        static JValue nullVal;
        auto it = properties_.find(key);
        return it != properties_.end() ? it->second : nullVal;
    }

    bool hasProperty(const std::string& key) const {
        return properties_.count(key) > 0;
    }

    const std::map<std::string, JValue>& properties() const { return properties_; }

    // 子组件
    void setChildren(const Children& c) { children_ = c; }
    const Children& children() const { return children_; }

    // 绑定值（A2UI BoundValue）
    void setBoundValue(const std::string& name, const JValue& val) {
        boundValues_[name] = val;
    }

    const JValue& boundValue(const std::string& name) const {
        static JValue nullVal;
        auto it = boundValues_.find(name);
        return it != boundValues_.end() ? it->second : nullVal;
    }

    // 布局缓存（由布局引擎设置）
    void setLayoutBounds(const Rect& r) { layoutBounds_ = r; }
    const Rect& layoutBounds() const { return layoutBounds_; }

    // 事件回调
    void setAction(const std::string& action) { action_ = action; }
    const std::string& action() const { return action_; }

    // 可见性
    void setVisible(bool v) { visible_ = v; }
    bool visible() const { return visible_; }

    // 启用状态
    void setEnabled(bool e) { enabled_ = e; }
    bool enabled() const { return enabled_; }

protected:
    std::string id_;
    WidgetType type_;
    std::map<std::string, JValue> properties_;
    std::map<std::string, JValue> boundValues_;
    Children children_;
    Rect layoutBounds_;
    std::string action_;
    bool visible_ = true;
    bool enabled_ = true;
};

using WidgetPtr = std::shared_ptr<Widget>;

} // namespace jui
