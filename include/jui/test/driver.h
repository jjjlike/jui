/**
 * @file driver.h
 * @brief AI自动化测试驱动 — 无窗口环境下驱动UI引擎，注入事件，读取状态
 * 
 * 设计目标：
 * - 零人工干预的自动化UI测试
 * - 不依赖HWND，无需D2D实际渲染
 * - 支持完整的事件注入链路：JSON加载 → 布局计算 → 事件分发 → 状态验证
 * - 提供声明式断言API，AI可直接理解和使用
 */
#pragma once

#include "jui/jui.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace jui::test {

// ============================================================
// 输入事件类型枚举
// ============================================================
enum class InputEventType {
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel,
    KeyDown,
    KeyUp,
    CharInput,
    Focus,
    Blur
};

// ============================================================
// 模拟输入事件结构体
// ============================================================
struct SimulatedEvent {
    InputEventType type;
    float x = 0, y = 0;        // 鼠标坐标（相对于窗口）
    int button = 0;             // 鼠标按键：0=左键
    int keyCode = 0;            // 虚拟键码
    uint32_t charCode = 0;      // Unicode 字符
    float wheelDelta = 0;       // 滚轮增量
    std::string targetId;       // 目标控件ID（可选，用于直接路由）
};

// ============================================================
// AiTestDriver — 无头测试驱动
// ============================================================
class AiTestDriver {
public:
    // 构造函数：内部创建虚拟HWND（或使用空句柄）
    AiTestDriver();
    ~AiTestDriver();

    // ---- 初始化与加载 ----
    // 初始化驱动（创建引擎但不显示窗口）
    bool initialize();
    // 加载JSON界面并触发完整布局管线（createSurface→surfaceUpdate→beginRendering）
    void loadUI(const std::string& surfaceId, const std::string& componentsJson);
    // 直接从完整JSONL流加载
    void loadJSONL(const std::string& jsonl);

    // ---- 交互模拟 ----
    // 模拟鼠标点击指定控件（自动计算控件中心坐标）
    void click(const std::string& widgetId);
    // 模拟鼠标按下/释放
    void mouseDown(const std::string& widgetId);
    void mouseUp(const std::string& widgetId);
    // 模拟键盘输入字符
    void typeText(const std::string& widgetId, const std::string& text);
    // 模拟键盘按键
    void pressKey(int vkCode);
    // 模拟鼠标滚轮
    void scroll(const std::string& widgetId, float delta);
    // 模拟点击Tab（直接切换）
    void clickTab(const std::string& tabsId, int index);
    // 模拟拖动滑块到指定值
    void dragSlider(const std::string& sliderId, float targetValue);
    // 注入原始事件（高级API）
    void injectEvent(const SimulatedEvent& event);

    // ---- 状态读取 ----
    // 获取控件文本（Text/Button/TextField 的当前内容）
    std::string widgetText(const std::string& widgetId);
    // 控件是否可见
    bool widgetVisible(const std::string& widgetId);
    // 控件是否启用
    bool widgetEnabled(const std::string& widgetId);
    // 复选框/开关是否勾选
    bool widgetChecked(const std::string& widgetId);
    // 编辑框是否获得焦点
    bool widgetFocused(const std::string& widgetId);
    // 滑动条当前值
    float sliderValue(const std::string& sliderId);
    // 列表选中索引
    int listSelectedIndex(const std::string& listId);
    // Tabs 当前激活索引
    int tabsActiveIndex(const std::string& tabsId);
    // 控件布局边界
    Rect widgetBounds(const std::string& widgetId);
    // 控件是否存在
    bool widgetExists(const std::string& widgetId);
    // 控件类型
    WidgetType widgetType(const std::string& widgetId);

    // ---- 表面管理 ----
    // 切换当前操作的Surface
    void setActiveSurface(const std::string& surfaceId);
    // 获取Surface上所有控件ID
    std::vector<std::string> allWidgetIds();

    // ---- 回调捕获 ----
    // 设置action回调捕获（记录所有触发的action）
    std::vector<std::string> capturedActions() const;
    // 检查最后触发的action名称
    std::string lastAction() const;
    // 检查最后触发的action来源控件
    std::string lastActionSource() const;

    // ---- 布局验证 ----
    // 两个控件的bounds是否重叠
    bool boundsOverlap(const std::string& a, const std::string& b);
    // parent 的 bounds 是否完全包含 child
    bool boundsContains(const std::string& parent, const std::string& child);

private:
    // 内部实现
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace jui::test
