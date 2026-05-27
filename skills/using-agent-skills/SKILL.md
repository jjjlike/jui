---
name: using-agent-skills
description: 发现并调用 C++ Agent Skills。在开始新会话或需要确定当前任务适用哪个技能时使用。这是管理所有其他技能如何被发现和调用的元技能。
---

# 使用 C++ Agent Skills

## 概述

C++ Agent Skills 是一套专为 C++ 客户端独立开发者定制的工程工作流技能集合。每个技能都编码了资深 C++ 工程师所遵循的特定流程。本元技能帮助你发现并应用适合当前任务的技能。

## 自动化原则

作为独立开发者，你是唯一的决策者。Agent 将采用以下自动化策略以减少打断你的次数：

- **自动推进**：对于可逆操作（编译、测试、格式化），自动执行不询问。
- **通知即继续**：对于低风险操作（添加文件、重构函数），通知后自动继续。
- **仅关键确认**：仅在以下情况才需要你的确认：修改公共 API（头文件中的接口签名）、更改构建系统（CMakeLists.txt 结构变化）、引入新依赖（第三方库）。
- **安全默认优先**：不确定时，默认选择最安全、最保守的方案，并说明理由后自动执行。

## 技能发现

当任务到达时，识别开发阶段并应用对应的技能：

```
任务到达
    │
    ├── 有个模糊想法，需要打磨？ ──────→ cpp-idea-refine
    ├── 新功能/新项目/大改动？ ────────→ cpp-spec-driven-development
    ├── 有规格说明，需要拆解？ ────────→ cpp-planning-and-task-breakdown
    ├── 开始写代码？ ──────────────────→ cpp-incremental-implementation
    ├── 编写/运行测试？ ────────────────→ cpp-test-driven-development
    ├── 出了 Bug / 崩溃？ ────────────→ cpp-debugging
    ├── 审查自己的代码？ ──────────────→ cpp-code-review
    ├── 程序变慢了？ ──────────────────→ cpp-performance-optimization
    ├── 内存泄漏 / 野指针？ ──────────→ cpp-memory-safety
    ├── 编译/依赖/CMake 问题？ ────────→ cpp-build-system
    └── 代码太复杂，需要简化？ ────────→ cpp-code-simplification
```

## 核心操作行为

以下行为在任何时候、任何技能中都适用，不可协商。

### 1. 明示假设

在实现任何非平凡任务之前，明确陈述你的假设：

```
我做的假设：
1. [关于需求的假设]
2. [关于架构的假设]
3. [关于平台/编译器的假设]
→ 有异议请说，否则我将按此继续。
```

### 2. 主动管理困惑

遇到不一致、需求冲突或规格不明确时：

1. **停下来。** 不要凭猜测继续。
2. 明确指出具体的困惑点。
3. 提出权衡方案或澄清问题。
4. 等待解决后再继续。

### 3. 强制保持简洁

C++ 代码尤其容易过度设计。抵制模板元编程的诱惑，除非它确实减少重复代码。

每完成一段实现后问自己：
- 能直接用标准库解决吗？（`<algorithm>`、`<memory>`、`<string_view>`）
- 这个模板真的必要吗，还是普通函数就够？
- 这个抽象值得它的复杂度吗？
- 如果用 C with Classes 风格写，会不会更清晰？

### 4. 保持范围纪律

只动你被要求动的部分。不要：

- 顺带"清理"与任务无关的文件
- 删除你不理解的注释（可能是为某个 Bug 留的线索）
- 大规模重构相邻模块
- 添加"看起来有用"的功能

### 5. 验证而非假设

编译通过 ≠ 正确。每步都需要验证：
- 测试通过（Google Test / Catch2）
- AddressSanitizer 无报错
- 手动跑一遍关键路径

### 6. 中文注释规范

所有代码注释**必须使用中文编写**，且必须**详细、有意义**：

```cpp
// ✗ 差：无注释或注释过于简略
int m_maxRetry;

// ✗ 差：英文注释
// Maximum number of retry attempts
int m_maxRetry;

// ✓ 好：中文详细注释，包含业务含义、约束条件和设计理由
// 失败重试的最大次数。设为 0 表示不重试。
// 注意：重试间隔为指数退避（1s, 2s, 4s...），实际最长等待约为 2^kMaxRetries 秒。
// 当前值 3 意味着最多等待 1+2+4 = 7 秒后放弃。
static constexpr int kMaxRetries = 3;
```

注释要求：
- 每个类/结构体需要中文注释说明其职责
- 每个公开函数需要中文注释说明功能、参数、返回值、异常
- 关键算法段落需要中文注释解释逻辑（尤其是非显而易见的优化）
- 每个成员变量需要中文注释说明其含义和约束
- TODO/FIXME/HACK 需要用中文写清上下文

### 7. UI 实现原则：原生 API 优先

实现任何 UI 界面时，严格遵循以下优先级：

1. **优先使用 Win32 原生 API** —— `CreateWindowEx`、`SetWindowPos` 等系统接口
2. **需要自绘时使用 Direct2D** —— 结合原生 API 窗口，用 D2D 进行高性能渲染
3. **允许封装小型界面库** —— 在满足前两条的前提下，可将常用 UI 模式封装成轻量级内部库（窗口基类、控件封装、D2D 渲染器），但禁止引入第三方 UI 框架（Qt 等除外，如项目已全面使用 Qt 则继续使用 Qt）

```cpp
// ✓ 好：原生 API + D2D 自绘
class CustomButton {
public:
    // 创建按钮窗口（基于 Win32 原生 API）
    bool Create(HWND parent, const RECT& rect, std::wstring_view text);
    
    // 使用 D2D 渲染按钮外观
    void Render(ID2D1RenderTarget* rt);
    
private:
    HWND m_hwnd = nullptr;               // 原生窗口句柄
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush; // D2D 画刷
    std::wstring m_text;                 // 按钮文本
};
```

**自动执行原则：** 除非你明确要求，Agent 不应主动引入大型 UI 框架。用原生 API + D2D 构建的轻量封装足以满足大多数客户端 UI 需求。

## 技能规则

1. **开始工作前检查是否有适用的技能。**
2. **技能是工作流，不是建议。** 按顺序执行步骤。
3. **多个技能可以串联使用。** 例如：`cpp-spec-driven-development` → `cpp-planning-and-task-breakdown` → `cpp-incremental-implementation` → `cpp-test-driven-development` → `cpp-code-review`
4. **有疑问时，从规格开始。**

## 生命周期序列

对于一个完整功能，典型技能执行序列：

```
1. cpp-spec-driven-development     → 定义要构建什么
2. cpp-planning-and-task-breakdown → 拆解为可验证的模块
3. cpp-incremental-implementation  → 逐层构建
4. cpp-test-driven-development     → 证明每个模块正确
5. cpp-code-review                 → 自我审查
6. cpp-code-simplification         → 消除过度设计
7. cpp-memory-safety               → 内存安全检查
8. cpp-performance-optimization    → 按需优化
```

并非每个任务都需要所有技能。一个 Bug 修复可能只需：`cpp-debugging` → `cpp-test-driven-development` → `cpp-code-review`。

## 快速参考

| 阶段 | 技能 | 一句话概述 |
|------|------|-----------|
| 定义 | cpp-idea-refine | 通过结构化思维精炼想法 |
| 定义 | cpp-spec-driven-development | 先写规格再写代码 |
| 规划 | cpp-planning-and-task-breakdown | 拆解为小型可验证任务 |
| 构建 | cpp-incremental-implementation | 薄垂直切片，逐层构建 |
| 构建 | cpp-build-system | CMake / vcpkg / 依赖管理，含编码配置 |
| 验证 | cpp-test-driven-development | GTest 先行，红-绿-重构 |
| 验证 | cpp-debugging | 复现 → 定位 → 修复 → 防护 |
| 审查 | cpp-code-review | 七轴自审查（含中文注释+编码检查） |
| 审查 | cpp-code-simplification | 消除过度设计 |
| 审查 | cpp-memory-safety | RAII + 智能指针 + Sanitizer |
| 审查 | cpp-performance-optimization | 先测量，再优化 |

### 基础约束（所有技能生效）

- **中文注释**：所有注释、文档字符串用中文编写，详细说明"为什么"而不仅是"是什么"
- **中文编码**：源文件 UTF-8 with BOM；运行时宽字符串用 `std::wstring` + Windows Unicode API；控制台输出用 `SetConsoleOutputCP(CP_UTF8)` 或 `std::wcout`
- **UI 策略**：原生 Win32 API 优先，自绘用 Direct2D，可封装轻量内部 UI 库，不主动引入第三方 UI 框架
