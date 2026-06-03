# JUI v0.2.0 框架优化重构设计文档

> **设计日期**：2026-06-03  
> **设计目标**：架构清晰化、AI自动化测试原生支持、示例体系完备、文档精确同步

---

## 目录

- [1. 当前架构诊断](#1-当前架构诊断)
- [2. 目标架构设计](#2-目标架构设计)
- [3. 模块职责重新划分](#3-模块职责重新划分)
- [4. AI自动化测试基础设施](#4-ai自动化测试基础设施)
- [5. 示例体系设计](#5-示例体系设计)
- [6. 重构实施路径](#6-重构实施路径)
- [7. 文档同步更新计划](#7-文档同步更新计划)

---

## 1. 当前架构诊断

### 1.1 现状架构图

```
┌────────────────────────────────────────────────────────┐
│                      JUIEngine                         │
│  ┌─────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │ EngineImpl│  │ D2D    │  │ map<string,Surface> │  │
│  │ (PIMPL)  │  │ Renderer │  │    surfaces_        │  │
│  └─────────┘  └──────────┘  └──────────────────────┘  │
│  ▲ friended   ▲ raw member   ▲ raw member              │
│                                                         │
│  processMessage → 直接内联解析 JSON (15KB 单体函数)      │
└────────────────────────────────────────────────────────┘
```

### 1.2 已识别问题

| # | 问题 | 影响 | 严重度 |
|---|------|------|--------|
| 1 | **PIMPL 不一致**：`EngineImpl` 声明为友元但 `D2DRenderer`/`LayoutEngine`/`surfaces_` 直接暴露在头文件 | 接口不清晰，修改私有成员触发全量重编译 | 中 |
| 2 | **引擎解析器单体化**：`engine.cpp` 中 `processMessage` 内联所有 JSON 解析逻辑（15KB+） | 难以扩展新消息类型，测试覆盖困难 | 高 |
| 3 | **无事件总线**：控件交互通过 `dynamic_cast` 链式判断分发 | 添加新控件需修改 D2DRenderer 核心循环 | 高 |
| 4 | **无 AI 测试接口**：无法在无窗口环境下驱动 UI、验证状态 | AI 无法自动化验证 UI 逻辑正确性 | 高 |
| 5 | **示例单一**：仅 1 个 demo 程序，内联所有 JSON | AI/new developer 缺乏渐进式学习路径 | 中 |
| 6 | **渲染器耦合交互**：`onMouseDown` 同时处理焦点、hittest、状态切换、回调触发 | 难以单独测试交互逻辑 | 中 |
| 7 | **无变更回调机制**：Widget 属性变更不通知渲染层 | 需手动 invalidate rect 触发重绘 | 中 |
| 8 | **布局引擎与渲染无解耦接口**：layout() 直接操作 Widget 对象 | 无法替换布局算法 | 低 |

### 1.3 数据流痛点

```
当前：用户点击 → D2DRenderer::onMouseDown
  → for each renderWidget_ (dynamic_cast 链式判断类型)
    → 修改 WidgetState → 设置 needsRedraw_
  → WM_PAINT → render() → paint()

问题：添加新控件需要修改 D2DRenderer 核心循环（违反开放封闭原则）
```

---

## 2. 目标架构设计

### 2.1 目标架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           JUIEngine v0.2.0                              │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                       EventBus（事件总线）                          │ │
│  │  InputEvent → Dispatcher → ActionHandler → StateChange → Callback │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────────────┐ │
│  │ MessageParser │  │ LayoutEngine │  │      D2DRenderer             │ │
│  │               │  │              │  │                               │ │
│  │ parseCreateSrf│  │ measure()    │  │ render(FrameContext)          │ │
│  │ parseUpdateCmp│  │ arrange()    │  │ dispatchInput(InputEvent)     │ │
│  │ parseDataModel│  │ resolveSize()│  │ refreshRenderTree()            │ │
│  │ parseBeginRndr│  │              │  │                               │ │
│  └──────┬────────┘  └──────┬───────┘  └───────────────┬───────────────┘ │
│         │                  │                           │                  │
│         ▼                  ▼                           ▼                  │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                      Surface (每个独立界面)                       │   │
│  │  ┌──────────┐  ┌──────────┐  ┌────────────┐  ┌──────────────┐   │   │
│  │  │ Widget树  │  │ DataModel│  │RenderWidget│  │ WidgetState  │   │   │
│  │  │ (邻接表)  │  │(JSON Ptr)│  │   树       │  │   层         │   │   │
│  │  └──────────┘  └──────────┘  └────────────┘  └──────────────┘   │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │                    AI Test Infrastructure                         │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐ │   │
│  │  │ TestDriver │  │ Recorder   │  │ Validator  │  │ Scenario   │ │   │
│  │  │(headless)  │  │(rec/play)  │  │(assertions)│  │ Runner     │ │   │
│  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 新增/重构模块清单

| 模块 | 类型 | 说明 |
|------|------|------|
| `EventBus` | **新增** | 统一事件分发，解除 D2DRenderer 对控件类型的硬编码依赖 |
| `MessageParser` | **重构** | 从 `engine.cpp` 拆分出独立的消息解析器，每种 A2UI 消息独立方法 |
| `AiTestDriver` | **新增** | 无头模式下驱动 UI，注入模拟事件，读取控件状态 |
| `InteractionRecorder` | **新增** | 录制/回放用户交互序列 |
| `UiValidator` | **新增** | 声明式 UI 状态断言（控件存在/可见/文本/位置/启用态） |
| `ChangeNotifier` | **新增** | Widget 属性变更时自动通知渲染层刷新 |

---

## 3. 模块职责重新划分

### 3.1 core 层

| 模块 | v0.1.0 职责 | v0.2.0 职责 | 变更 |
|------|-----------|-----------|------|
| `JValue` | 值容器 | 不变 | — |
| `Widget` | 属性+布局+子组件 | 不变 | — |
| `DataModel` | 数据绑定 | 不变 | — |
| `Surface` | Widget容器+DataModel | 不变 | — |
| `JUIEngine` | 解析+布局+渲染+事件 | 协调器（委托给子模块） | PIMPL 整合，暴露最少接口 |
| `MessageParser` | (内联在 engine.cpp) | 独立模块，解析所有 A2UI 消息 | **重构提取** |
| `EventBus` | (不存在) | 事件注册/分发/注销 | **新增** |
| `ChangeNotifier` | (不存在) | Widget属性→渲染层通知 | **新增** |

### 3.2 render 层

| 模块 | v0.1.0 职责 | v0.2.0 职责 | 变更 |
|------|-----------|-----------|------|
| `RenderWidget` | paint+measure+hitTest | paint+measure+hitTest+onEvent(InputEvent) | 新增统一事件接口 |
| `D2DRenderer` | 渲染+事件分发+剪贴板+IME | 渲染+剪贴板+IME；事件分发委托给 EventBus | 事件分发逻辑剥离 |
| `WidgetState` | 控件状态 | 不变 | — |

### 3.3 test 层（全新）

| 模块 | 职责 |
|------|------|
| `AiTestDriver` | 创建虚拟 Surface、注入 JSON、驱动布局、读取状态 |
| `InteractionRecorder` | 记录操作序列（点击/输入/滚动）、JSON 序列化、回放 |
| `UiValidator` | 检查控件存在/文本/位置/尺寸/可见性/启用态/焦点/选中态 |
| `TestScenario` | 预定义测试场景（登录表单、列表滚动、表格排序等） |

---

## 4. AI 自动化测试基础设施

### 4.1 设计原则

> **声明式测试 + 自动验证 = AI 可独立编写、运行、分析结果的测试体系**

```
AI 编写测试 → TestScenario JSON → AiTestDriver 加载 → 
  EventBus 注入事件 → Surface 状态变更 → 
  UiValidator 断言 → 测试结果（PASS/FAIL + 详细日志）
```

### 4.2 AiTestDriver（无头测试驱动）

```cpp
// 不依赖 HWND、无需 D2D 渲染的测试驱动
class AiTestDriver {
public:
    // 初始化（不需要窗口句柄）
    bool initialize();

    // 加载 JSON 界面并触发完整布局管线
    void loadUI(const std::string& json);

    // 模拟用户操作（通过 EventBus 注入）
    void click(const std::string& widgetId);
    void type(const std::string& widgetId, const std::string& text);
    void scroll(const std::string& widgetId, float delta);
    void clickTab(const std::string& tabsId, int index);
    void dragSlider(const std::string& sliderId, float value);

    // 读取控件状态（用于断言）
    std::string widgetText(const std::string& widgetId);
    bool widgetVisible(const std::string& widgetId);
    bool widgetEnabled(const std::string& widgetId);
    bool widgetChecked(const std::string& widgetId);
    bool widgetFocused(const std::string& widgetId);
    float sliderValue(const std::string& sliderId);
    int listSelectedIndex(const std::string& listId);
    int tabsActiveIndex(const std::string& tabsId);
    Rect widgetBounds(const std::string& widgetId);

    // 布局验证
    bool boundsOverlap(const std::string& a, const std::string& b);
    bool boundsContains(const std::string& parent, const std::string& child);
};
```

### 4.3 UiValidator（声明式断言）

```cpp
// 使用链式调用进行 UI 状态断言
class UiValidator {
public:
    UiValidator& expectWidget(const std::string& id);
    UiValidator& visible();
    UiValidator& notVisible();
    UiValidator& enabled();
    UiValidator& disabled();
    UiValidator& hasText(const std::string& text);
    UiValidator& hasTextContaining(const std::string& substr);
    UiValidator& checked();
    UiValidator& unchecked();
    UiValidator& focused();
    UiValidator& hasBounds(float x, float y, float w, float h);
    UiValidator& hasWidthGreaterThan(float minW);
    UiValidator& hasHeightGreaterThan(float minH);
    UiValidator& notOverlapping(const std::string& otherId);

    // 收集所有断言结果
    struct AssertResult { bool pass; std::string widgetId; std::string message; };
    std::vector<AssertResult> results();
    bool allPassed();
    std::string report();  // 人类/AI 可读的测试报告
};
```

### 4.4 InteractionRecorder（录制/回放）

```cpp
class InteractionRecorder {
public:
    // 开始录制
    void startRecord();
    // 结束录制，返回操作序列 JSON
    std::string stopRecord();

    // 从 JSON 加载并回放
    void loadScenario(const std::string& scenarioJson);
    bool playNext();      // 播放下一帧操作
    bool isFinished();

    // 录制内容格式：
    // [{"type":"click","target":"btn","time":0},
    //  {"type":"keyinput","target":"tf1","text":"hello","time":100},
    //  {"type":"assert","target":"txt","property":"text","expected":"hello"}]
};
```

### 4.5 AI 测试场景示例

```json
{
  "scenario": "登录表单基本流程",
  "steps": [
    {"action":"loadUI",      "jsonFile": "form_login.jui"},
    {"action":"assert",      "check": "widget('tf1').visible().enabled()"},
    {"action":"assert",      "check": "widget('btn_submit').visible().hasText('登录')"},
    {"action":"click",       "target": "tf1"},
    {"action":"assert",      "check": "widget('tf1').focused()"},
    {"action":"typeChars",   "target": "tf1", "text": "admin"},
    {"action":"assert",      "check": "widget('tf1').hasText('admin')"},
    {"action":"click",       "target": "btn_submit"},
    {"action":"assertCallback", "expectAction": "login"}
  ]
}
```

### 4.6 默认 AI 测试覆盖（jui_hello）

```
场景 1: 页面 Tab 切换
  点击 Tab "Button"  → 断言 tabs.activeIndex() == 0
  点击 Tab "List"    → 断言 tabs.activeIndex() == 4
  点击 Tab "Grid"    → 断言 tabs.activeIndex() == 5

场景 2: 按钮交互
  点击 btnB 按钮 → 断言收到 action="click", source="btnB"

场景 3: 编辑框输入
  点击 tf1 → 断言 tf1.focused() == true
  输入 "test" → 断言 tf1.hasText("test")

场景 4: 复选框切换
  点击 cb1 → 断言 cb1.checked() == true
  再次点击 → 断言 cb1.checked() == false

场景 5: 滑动条操作
  设置 sl1.value = 75 → 断言 sliderValue("sl1") ≈ 75

场景 6: 列表滚动
  列表有 100000 项 → 断言 visibleStart() < visibleEnd()
  滚动 → 断言 visibleStart() 已变更

场景 7: 表格数据
  断言 Grid 有 5 列、50000 行
  断言 visibleRowEnd() - visibleRowStart() < 30
```

---

## 5. 示例体系设计

### 5.1 示例梯度

| # | 示例名 | 难度 | 学习焦点 | 控件覆盖 |
|---|--------|------|---------|---------|
| 01 | `hello_window` | ★☆☆ | 最小启动：引擎初始化、窗口消息、单个Text | Text |
| 02 | `hello_button` | ★☆☆ | 第一个交互：按钮点击 + action 回调 | Button, Text |
| 03 | `layout_demo` | ★★☆ | Row/Column/Card 布局系统 | Row, Column, Card, Text, Button |
| 04 | `form_demo` | ★★☆ | 编辑框 + 表单提交 | TextField, Button, Text |
| 05 | `controls_demo` | ★★☆ | 复选框/开关/下拉/滑块 | CheckBox, Toggle, Slider |
| 06 | `tabs_demo` | ★★☆ | Tabs 分页切换 | Tabs, 全部控件 |
| 07 | `list_demo` | ★★★ | 虚拟列表 + Provider | List |
| 08 | `grid_demo` | ★★★ | 虚拟表格 + 列定义 | Grid |
| 09 | `binding_demo` | ★★★ | DataModel 数据绑定 | Text, Button, TextField |
| 10 | `full_app` | ★★★ | 完整多Surface应用 | 全部 |

> 当前仅 1 个示例 → 目标 10 个（基础学习需求的 2 倍）

### 5.2 样例编写规范

每个示例必须包含：
1. **文件头部注释**：学习目标、覆盖控件、预期效果
2. **步骤编号注释**：每个关键步骤用 `// Step 1/2/3` 标记
3. **行内参数注释**：每个非自明参数标注含义
4. **状态流转注释**：交互前后状态变化的说明
5. **自包含性**：单个 `.cpp` 文件可独立编译运行

---

## 6. 重构实施路径

### 阶段一：基础设施（不影响现有功能）

| 步骤 | 任务 | 产出文件 | 预计变更行数 |
|------|------|---------|------------|
| 1.1 | 新增 `include/jui/test/` 目录，创建 `driver.h`、`recorder.h`、`validator.h` | 3 个头文件 | ~300 行 |
| 1.2 | 实现 `src/test/driver.cpp`、`recorder.cpp`、`validator.cpp` | 3 个源文件 | ~500 行 |
| 1.3 | 更新 CMakeLists.txt，添加 `jui_test_infra` 目标 | CMakeLists.txt | ~15 行 |
| 1.4 | 新增 `tests/test_ai_integration.cpp`，实现 jui_hello 全链路测试 | 1 个测试文件 | ~400 行 |

### 阶段二：架构重构

| 步骤 | 任务 | 产出文件 | 预计变更行数 |
|------|------|---------|------------|
| 2.1 | 从 `engine.cpp` 提取 `MessageParser` 模块 | `include/jui/core/message_parser.h` + `src/core/message_parser.cpp` | ~400 行 |
| 2.2 | 修复 PIMPL：将 `D2DRenderer`/`LayoutEngine`/`surfaces_` 移入 `EngineImpl` | `engine.h` + `engine.cpp` | ~80 行变更 |
| 2.3 | 新增 `ChangeNotifier`：Widget 属性变更 → 自动标记 dirty | `include/jui/core/change_notifier.h` | ~50 行 |
| 2.4 | 在 `RenderWidget` 基类新增 `onInputEvent(InputEvent)` 虚方法 | `render_widget.h` + `render_widget.cpp` | ~60 行 |
| 2.5 | 更新 `D2DRenderer::onMouseDown` 使用 `onInputEvent` 替代 `dynamic_cast` 链 | `d2d_renderer.cpp` | ~100 行变更 |

### 阶段三：示例扩充

| 步骤 | 任务 | 产出文件 | 预计变更行数 |
|------|------|---------|------------|
| 3.1 | 创建示例 01-10 目录和 CMakeLists.txt | 10 个目录 + 10 个 CMakeLists.txt | ~100 行 |
| 3.2 | 编写示例 01-05（入门-中级） | 5 个 main.cpp | ~800 行 |
| 3.3 | 编写示例 06-10（中-高级） | 5 个 main.cpp | ~800 行 |
| 3.4 | 保留并重构原 `jui_hello` 为示例 06（Tabs 分页） | `examples/06_tabs_demo/main.cpp` | ~200 行 |

### 阶段四：文档同步

| 步骤 | 任务 | 产出文件 |
|------|------|---------|
| 4.1 | 更新 `ARCHITECTURE.md` | 新增模块说明 + 架构图 |
| 4.2 | 更新 `USER_MANUAL.md` | 新增示例索引 + AI测试指南 |
| 4.3 | 新增 `EXAMPLES_GUIDE.md` | 10 个示例的学习路线图 |

### 重构总预估

| 指标 | 数值 |
|------|------|
| 新增文件 | ~20 个 |
| 修改文件 | ~8 个 |
| 新增代码 | ~3500 行 |
| 修改代码 | ~400 行 |
| 新增测试 | ~130 个用例（整合现有 358 个） |
| 预计时间 | 一次性执行 |

---

## 7. 文档同步更新计划

### 7.1 ARCHITECTURE.md 更新点

- 新增 `test/` 层架构图
- 新增 MessageParser 模块说明
- 新增 EventBus / ChangeNotifier 交互流
- 更新目录结构（含 example 子目录）
- 更新技术栈（AI测试框架）

### 7.2 USER_MANUAL.md 更新点

- 新增「AI 测试接口」章节
- 新增「示例索引」表格（链接到 10 个示例）
- 更新属性参考表（新增字段）
- 更新常见问题（新增 AI 测试相关问题）

### 7.3 EXAMPLES_GUIDE.md（全新文档）

- 10 个示例的学习路线图
- 每个示例的：学习目标、覆盖控件、运行方法、预期效果截图描述
- 从零到完整应用的进阶路径

---

## 附录：关键接口变化对比

### before (v0.1.0)

```cpp
// D2DRenderer 中硬编码的类型判断
if (rw->widget()->type() == WidgetType::Button) { ... press(); }
if (rw->widget()->type() == WidgetType::CheckBox) { ... toggle(); }
if (rw->widget()->type() == WidgetType::Slider) { ... setDragging(); }
// 每增加一个控件，需修改此函数
```

### after (v0.2.0)

```cpp
// 统一事件接口，控件自处理
InputEvent evt{InputEventType::MouseDown, fx, fy};
rw->onInputEvent(evt);
// ButtonRenderWidget 内部：bs->press()
// SliderRenderWidget 内部：ss->setDragging(true)
// 新控件只需实现 onInputEvent，不修改 D2DRenderer
```

### AiTestDriver 使用示意

```cpp
AiTestDriver driver;
driver.initialize();
driver.loadUI(formJson);

// AI 可读的测试断言
UiValidator v;
v.expectWidget("tf1").visible().enabled();
v.expectWidget("btn_submit").hasText("登录").enabled();

// 模拟交互
driver.type("tf1", "admin");
driver.click("btn_submit");

// 验证结果
assert(v.allPassed());
```
