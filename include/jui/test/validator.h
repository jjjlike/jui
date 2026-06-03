/**
 * @file validator.h
 * @brief UI状态声明式验证器 — 链式断言API，AI可直接生成验证代码
 *
 * 设计原则：每个断言方法返回自身引用（链式调用），失败时收集详细错误信息
 * 
 * 使用示例（AI可读）：
 *   UiValidator v;
 *   v.expect("tf1").visible().enabled().hasText("hello");
 *   v.expect("btn_submit").hasText("登录").notDisabled();
 *   assert(v.allPassed());
 */
#pragma once

#include "jui/core/widget.h"
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace jui::test {

// ============================================================
// 单条断言结果
// ============================================================
struct AssertResult {
    bool pass = false;
    std::string widgetId;
    std::string check;       // 检查项名称："visible", "enabled", "hasText"等
    std::string expected;    // 期望值
    std::string actual;      // 实际值
    std::string message() const {
        std::stringstream ss;
        if (pass) ss << "[PASS] ";
        else ss << "[FAIL] ";
        ss << widgetId << "." << check;
        if (!expected.empty()) ss << " expected=" << expected;
        if (!actual.empty()) ss << " actual=" << actual;
        return ss.str();
    }
};

// ============================================================
// UiValidator — 声明式UI验证器
// ============================================================
class UiValidator {
public:
    UiValidator() = default;

    // 开始对指定控件的断言链
    UiValidator& expectWidget(const std::string& id);

    // ---- 存在性 ----
    UiValidator& exists();         // 控件存在
    UiValidator& notExists();      // 控件不存在

    // ---- 可见性 ----
    UiValidator& visible();        // 控件可见
    UiValidator& notVisible();     // 控件不可见

    // ---- 启用状态 ----
    UiValidator& enabled();        // 控件启用
    UiValidator& disabled();       // 控件禁用

    // ---- 文本内容 ----
    UiValidator& hasText(const std::string& expected);
    UiValidator& hasTextContaining(const std::string& substring);
    UiValidator& isEmpty();           // 文本为空
    UiValidator& notEmpty();          // 文本非空

    // ---- 勾选状态（CheckBox/Toggle） ----
    UiValidator& isChecked();      // 已勾选
    UiValidator& notChecked();     // 未勾选

    // ---- 焦点 ----
    UiValidator& focused();        // 获得焦点
    UiValidator& notFocused();     // 未获得焦点

    // ---- 位置与尺寸 ----
    // 精确匹配
    UiValidator& hasBounds(float x, float y, float w, float h);
    // 宽度约束
    UiValidator& hasWidth(float expected);
    UiValidator& hasWidthGreaterThan(float minW);
    UiValidator& hasWidthLessThan(float maxW);
    // 高度约束
    UiValidator& hasHeight(float expected);
    UiValidator& hasHeightGreaterThan(float minH);
    UiValidator& hasHeightLessThan(float maxH);
    // X/Y 位置约束
    UiValidator& hasX(float expected);
    UiValidator& hasY(float expected);
    UiValidator& hasYGreaterThan(float minY); // 控件在某个Y位置之下

    // ---- 布局关系 ----
    // 两个控件不重叠
    UiValidator& notOverlapping(const std::string& otherId);
    // 当前控件完全包含另一个控件
    UiValidator& containsWidget(const std::string& otherId);

    // ---- 数值（Slider） ----
    UiValidator& hasValue(float expected);
    UiValidator& hasValueInRange(float min, float max);

    // ---- 列表/网格 ----
    // List 有指定数量项
    UiValidator& hasItemCount(int expected);
    // List 可见项不少于指定数量
    UiValidator& hasVisibleItemsAtLeast(int minCount);
    // List 选中指定索引
    UiValidator& hasSelectedIndex(int expected);

    // ---- Tabs ----
    // Tabs 激活指定索引
    UiValidator& hasActiveIndex(int expected);

    // ---- 类型 ----
    UiValidator& hasType(const std::string& typeName);

    // ---- 结果收集 ----
    // 获取所有断言结果
    const std::vector<AssertResult>& results() const { return results_; }
    // 是否全部通过
    bool allPassed() const;
    // 失败断言数量
    int failCount() const;
    // 格式化报告（人类/AI可读）
    std::string report() const;
    // 重置所有断言结果
    void reset();

private:
    // 需要外部驱动器提供回调来查询控件状态
    // 回调签名：(widgetId) -> WidgetStateSnapshot
    struct WidgetSnapshot {
        bool exists = false;
        bool visible = false;
        bool enabled = false;
        std::string text;
        bool checked = false;
        bool focused = false;
        jui::Rect bounds{0,0,0,0};
        float value = 0;
        int itemCount = 0;
        int selectedIndex = -1;
        int activeIndex = -1;
        std::string typeName;
    };

    void addResult(bool pass, const std::string& check,
                   const std::string& expected, const std::string& actual);
    WidgetSnapshot snapshot(const std::string& id) const;

    std::string currentId_;
    std::vector<AssertResult> results_;

public:
    // 设置快照查询回调（由AiTestDriver注入）
    using SnapshotProvider = std::function<WidgetSnapshot(const std::string&)>;
    void setSnapshotProvider(SnapshotProvider provider) { provider_ = std::move(provider); }

private:
    SnapshotProvider provider_;
};

} // namespace jui::test
