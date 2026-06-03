# JUI — 极简轻量 Windows C++ UI 引擎

[![Tests](https://img.shields.io/badge/tests-461%2F461-brightgreen)](../../actions)
[![C++](https://img.shields.io/badge/C++-17-blue)](../../)
[![Platform](https://img.shields.io/badge/Windows-10%2F11-lightgrey)](../../)

**JUI** 是一个面向嵌入式 UI 场景的极简 Windows C++ GUI 引擎。基于 Direct2D + DirectWrite 渲染，兼容 **A2UI 协议**，实现了逻辑层与渲染层的完全分离。

---

## 目录

- [1. 项目背景](#1-项目背景)
- [2. 核心功能](#2-核心功能)
- [3. 架构概览](#3-架构概览)
- [4. 环境要求](#4-环境要求)
- [5. 快速开始](#5-快速开始)
- [6. 使用示例](#6-使用示例)
- [7. 目录结构](#7-目录结构)
- [8. API 接口](#8-api-接口)
- [9. 测试体系](#9-测试体系)
- [10. 常见问题](#10-常见问题)
- [11. 模块清单](#11-模块清单)

---

## 1. 项目背景

JUI 的设计目标是提供一套**极简、高性能、易测试**的 UI 解决方案：

- **逻辑-渲染分离**：Widget 树与 D2D 渲染完全解耦，可独立单元测试逻辑层
- **A2UI 协议驱动**：UI 完全由 JSON 声明，支持远程下发和动态更新
- **零依赖渲染**：仅依赖 Windows SDK 内置的 D2D/DWrite，无第三方 GUI 库
- **AI 友好**：内置 Inspector（结构化 JSON 快照）、DebugLogger（全流程追踪）、EventBus（模块解耦）

### 设计原则

| 原则 | 实现 |
|------|------|
| 逻辑-渲染分离 | Widget 树 + 状态类 vs D2D RenderWidget |
| JSON 驱动 | A2UI 协议消息（createSurface/surfaceUpdate/dataModelUpdate/beginRendering） |
| 可测试性 | 461 个自动化测试，Inspect API 输出 JSON Schema 契约 |
| 模块化 | EventBus + 接口隔离，每个模块 ≤ 300 行 |

---

## 2. 核心功能

| 功能 | 说明 |
|------|------|
| **16 种控件** | Text, Button, TextField, CheckBox, Toggle, ChoicePicker, Slider, List, Grid, Tabs, Image, Divider, Row, Column, Card |
| **虚拟滚动** | List/Grid 支持 10 万级数据量，仅渲染可见项 |
| **A2UI 协议** | 完整实现 createSurface / surfaceUpdate / dataModelUpdate / beginRendering / deleteSurface |
| **数据绑定** | JSON Pointer 路径绑定（`/user/name`），DataModel 驱动 UI 更新 |
| **中文输入** | IME 集成，支持拼音/五笔等输入法 |
| **剪贴板** | Ctrl+C/V/X/A 全支持 |
| **自动化测试** | Inspect API 输出 JSON Schema 格式快照，AI 可直接校验 |
| **事件总线** | EventBus 发布/订阅，模块间零直接依赖 |

---

## 3. 架构概览

```
┌──────────────────────────────────────────────────────────┐
│                     JUIEngine (外观)                       │
│  processMessage() | inspect() | render() | onSize/Input  │
├──────────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐  │
│  │ A2UI     │  │ Widget   │  │ Layout   │  │ Render  │  │
│  │ Parser   │→│ Tree     │→│ Engine   │→│ Pipeline│  │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘  │
├──────────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐               │
│  │ EventBus │  │ DataModel│  │ Input    │               │
│  │ (发布/订阅)│  │ (JSON路径)│  │ Dispatcher│              │
│  └──────────┘  └──────────┘  └──────────┘               │
└──────────────────────────────────────────────────────────┘
```

**数据流**：`JSON 消息 → A2UI Parser → Widget Tree → Layout Engine → Render Pipeline → D2D/DWrite 屏幕输出`

---

## 4. 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10/11 x64 |
| 编译器 | MSVC 2022 (Visual Studio 2022) |
| CMake | ≥ 3.16 |
| 图形 API | Direct2D + DirectWrite (Windows SDK 内置) |
| Python | ≥ 3.8（文档同步工具，可选） |

### 安装依赖

```bash
# 1. 克隆项目
git clone <repo-url> jui
cd jui

# 2. 配置 CMake
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 3. 编译
cmake --build build --config Debug
```

### 项目集成

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(path/to/jui)
target_link_libraries(your_app PRIVATE jui imm32)
target_include_directories(your_app PRIVATE ${jui_SOURCE_DIR}/thirdparty)
```

---

## 5. 快速开始

### 5.1 最小示例（10 行代码）

```cpp
#include <jui/jui.h>
using namespace jui;

// 1. 创建引擎
JUIEngine engine;
engine.initialize(hwnd);

// 2. 加载 UI（A2UI 协议）
engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
engine.processMessage(R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["txt"]}}}},
        {"id":"txt","component":{"Text":{"text":{"literalString":"Hello, JUI!"},
            "fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}}
    ]}})");
engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root",
    "width":800,"height":600}})");

// 3. 渲染（在 WM_PAINT 中调用）
engine.render();
```

### 5.2 Win32 窗口消息集成

```cpp
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:     e = new JUIEngine(); e->initialize(hwnd); /* load UI... */
                        SetTimer(hwnd, 1, 16, nullptr); return 0;
    case WM_TIMER:      InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT:      { PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
                          if(e) e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE:       if(e) e->onSize(LOWORD(lp), HIWORD(lp));
                        InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_LBUTTONDOWN:if(e) e->onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
                        InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_DESTROY:    delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
```

### 5.3 运行 Demo

```bash
# 编译后运行示例
build\Debug\jui_example_01_hello_window.exe   # 最小启动
build\Debug\jui_example_03_layout_demo.exe     # 布局演示
build\Debug\jui_example_10_full_app.exe        # 完整应用
build\Debug\jui_hello.exe                      # 全控件综合
```

---

## 6. 使用示例

### 6.1 A2UI JSON 协议

```json
// 1. 创建 Surface
{"createSurface": {"surfaceId": "main"}}

// 2. 声明组件树
{"surfaceUpdate": {"surfaceId": "main", "components": [
    {"id": "root", "component": {"Column": {"children": {"explicitList": ["txt"]}}}},
    {"id": "txt",  "component": {"Text":   {"text": {"literalString": "Hello"}}}}
]}}

// 3. 数据模型更新（支持 JSON Pointer 路径）
{"dataModelUpdate": {"surfaceId": "main", "path": "/user/name", "value": "Alice"}}

// 4. 开始渲染
{"beginRendering": {"surfaceId": "main", "root": "root", "width": 800, "height": 600}}

// 5. 销毁 Surface
{"deleteSurface": {"surfaceId": "main"}}
```

### 6.2 控件属性

```json
// Button — 带 action 回调
{"id":"btn","component":{"Button":{"text":{"literalString":"点击"},"action":"submit"}}}

// TextField — 占位文字 + 指定宽度
{"id":"tf","component":{"TextField":{"placeholder":{"literalString":"输入..."},"width":{"literalNumber":200}}}}

// CheckBox — 选中状态
{"id":"cb","component":{"CheckBox":{"text":{"literalString":"同意"},"value":{"literalString":"true"}}}}

// List — 虚拟滚动 10 万项
{"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},
    "itemHeight":{"literalNumber":32},"width":{"literalNumber":400}}}}
```

### 6.3 Action 回调

```cpp
engine.setActionCallback([](const std::string& surfaceId,
                             const std::string& action,
                             const std::string& sourceId,
                             const std::string& context) {
    if (action == "submit") {
        MessageBoxA(nullptr, ("Clicked: " + sourceId).c_str(), "JUI", MB_OK);
    }
});
```

### 6.4 Inspect API（自动化测试）

```cpp
// 获取 UI 状态快照（JSON Schema 格式）
std::string json = engine.inspect("main");

// json 结构:
// { surfaceId, timestamp, viewport, widgets: [
//   { id, type, visible, layout{x,y,w,h}, paint{x,y,w,h},
//     paintMatch, state{text,checked,...}, children[] }
// ]}

// AI 可直接基于 Schema 编写断言
auto j = nlohmann::json::parse(json);
for (auto& w : j["widgets"]) {
    assert(w["visible"] == true);
    assert(w["paintMatch"] == true);
}
```

---

## 7. 目录结构

```
jui/
├── include/jui/                 # 公开头文件
│   ├── jui.h                    # 统一入口
│   ├── core/
│   │   ├── engine.h             # JUIEngine 主类
│   │   ├── widget.h / surface.h # Widget 树 + Surface 容器
│   │   ├── value.h              # JValue 动态类型
│   │   ├── data_model.h         # DataModel (JSON Pointer)
│   │   ├── geometry.h           # Rect / Size
│   │   └── widget_types.h       # WidgetType 枚举
│   ├── render/
│   │   ├── d2d_renderer.h       # D2D 渲染器
│   │   ├── render_widget.h      # RenderWidget 工厂
│   │   ├── widget_state.h       # 状态类（整体）
│   │   ├── *_state.h            # 模块化状态类（10+个）
│   │   └── irender_target.h     # 渲染目标抽象接口
│   ├── layout/
│   │   ├── layout.h             # 布局引擎
│   │   └── ilayout_engine.h     # 布局接口
│   ├── bus/
│   │   ├── event_bus.h          # EventBus 实现
│   │   ├── ievent_bus.h         # EventBus 接口
│   │   └── service_locator.h    # DI 容器
│   ├── input/
│   │   └── iinput_dispatcher.h  # 输入分发接口
│   ├── inspector/
│   │   └── inspector.h          # UI 快照采集
│   └── test/
│       ├── test.h               # 测试工具入口
│       ├── driver.h             # AI 测试驱动
│       └── debug_logger.h       # 调试日志
├── src/                         # 实现文件
│   ├── core/                    # engine.cpp, widget.cpp, ...
│   ├── render/                  # d2d_renderer.cpp, render_widget.cpp
│   ├── layout/                  # layout.cpp
│   ├── inspector/               # inspector.cpp
│   └── test/                    # driver.cpp, validator.cpp, ...
├── examples/                    # 11 个渐进式示例
│   ├── 01_hello_window/         # 最小启动
│   ├── 02_hello_button/         # 按钮交互
│   ├── 03_layout_demo/          # 布局系统
│   ├── 04_form_demo/            # 表单编辑
│   ├── 05_controls_demo/        # 控件合集
│   ├── 06_tabs_demo/            # 分页切换
│   ├── 07_list_demo/            # 虚拟列表
│   ├── 08_grid_demo/            # 表格
│   ├── 09_binding_demo/         # 数据绑定
│   ├── 10_full_app/             # 完整应用
│   └── hello/                   # 全控件综合
├── tests/                       # 自动化测试（49 套件, 461 用例）
├── docs/                        # 文档
│   ├── REFACTOR_V3.md           # 模块化架构设计
│   ├── INSPECTOR_DESIGN.md      # Inspector 设计文档
│   ├── ARCHITECTURE.md          # 框架架构
│   ├── USER_MANUAL.md           # 使用手册
│   ├── API_REFERENCE.md         # API 参考（自动生成）
│   ├── CHANGELOG.md             # 变更日志（自动生成）
│   └── README.md                # 本文件
├── tools/                       # 工具
│   ├── doc_sync.py              # 文档自动同步
│   └── doc_sync_config.json     # 同步配置
└── CMakeLists.txt
```

---

## 8. API 接口

### JUIEngine

| 方法 | 说明 |
|------|------|
| `initialize(HWND)` | 初始化 D2D 渲染器 |
| `processMessage(jsonStr)` | 处理 A2UI JSON 消息 |
| `processMessageStream(jsonlStr)` | 批量处理 JSONL |
| `render()` | 执行一帧渲染 |
| `inspect(surfaceId?)` | 获取 UI 状态快照（JSON） |
| `setActionCallback(cb)` | 注册交互回调 |
| `onSize(w, h)` | 窗口尺寸变化 |
| `onMouseDown/Up/Move(x, y, btn)` | 鼠标事件 |
| `onCharInput(ch)` | 字符输入 |
| `onKeyDown/Up(vk)` | 键盘事件 |
| `onIMEStart/Composition/End(s)` | 中文 IME |

### 接口体系

| 接口 | 文件 | 说明 |
|------|------|------|
| `IEventBus` | `ievent_bus.h` | 发布/订阅事件总线 |
| `IWidgetTree` | `iwidget_tree.h` | Widget 树查询 |
| `IDataModel` | `idata_model.h` | JSON Pointer 数据模型 |
| `IRenderTarget` | `irender_target.h` | 渲染目标抽象 |
| `ILayoutEngine` | `ilayout_engine.h` | 布局引擎接口 |
| `IInputDispatcher` | `iinput_dispatcher.h` | 输入分发接口 |
| `IWidgetState` | `iwidget_state.h` | 控件状态接口 |

### 状态类命名空间 `jui::state`

```cpp
#include <jui/render/jui_states.h>

jui::state::ButtonState  btn;    // → jui::ButtonWidgetState
jui::state::SliderState  sld;    // → jui::SliderWidgetState
jui::state::ListState    lst;    // → jui::ListWidgetState
// ... 10 种状态类
```

---

## 9. 测试体系

| 类别 | 套件数 | 用例数 | 说明 |
|------|--------|--------|------|
| 核心逻辑 | 8 | ~200 | Value/Widget/DataModel/Surface/Engine |
| 状态类 | 1 | 10 | 模块化状态类别名验证 |
| 渲染 | 2 | ~80 | RenderWidget/D2DRenderer/Layout |
| AI 测试 | 4 | ~30 | Driver/Validator/Recorder/Integration |
| Inspect | 2 | 18 | Inspector API + 全示例验证 |
| 黑盒 | 1 | 5 | 独立进程 + 日志分析 |
| 文字可见性 | 1 | 8 | 颜色对比度 / WCAG 验证 |
| 事件总线 | 1 | 10 | EventBus + ServiceLocator |
| **总计** | **49** | **461** | **全量通过** |

### 运行测试

```bash
cmake --build build --config Debug --target jui_tests
build\Debug\jui_tests.exe --gtest_brief=1
```

---

## 10. 常见问题

### Q: 界面空白/白屏

**原因**：`WM_CREATE` 时 D2D Render Target 创建失败（窗口尺寸为 0×0），后续 `WM_SIZE` 未重建。

**解决**：已修复 `d2d_renderer.cpp` 中 `onSize()` 方法，确保 RT 为 null 时调用 `createDeviceResources()` 重建。

### Q: JSON 解析失败

**原因**：`R"(...)"` 原始字符串内的 `// 注释` 被当作 JSON 文本内容。

**解决**：纯 JSON 不要包含 `//` 注释。`processMessage` 的 catch 块现已输出 `OutputDebugString` 错误信息。

### Q: 中文乱码

**解决**：在 CMakeLists.txt 中添加 `/utf-8` 编译选项。确保 JSON 文件和源码都是 UTF-8 编码。

### Q: 字体不显示

**原因**：`Microsoft YaHei` 字体可能未安装（极简系统）。

**解决**：DWrite 会自动 fallback 到系统字体。如仍不可见，检查 `parseHexColor` 返回值 —— `#888888` 等浅色在白色背景上对比度不足。

### Q: 控件不响应点击

**解决**：确保 WndProc 中绑定了完整事件链：
```cpp
case WM_LBUTTONDOWN: engine->onMouseDown(...); InvalidateRect(...); return 0;
case WM_LBUTTONUP:   engine->onMouseUp(...); InvalidateRect(...); return 0;
case WM_MOUSEMOVE:   engine->onMouseMove(...); return 0;
```

### Q: List/Grid 滚动不可用

**解决**：确保绑定了 `WM_MOUSEWHEEL` 事件：
```cpp
case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
    engine->onMouseWheel(-delta * 40.0f);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}
```

---

## 11. 模块清单

| 模块 | 文件 | 说明 |
|------|------|------|
| event_bus | `include/jui/bus/event_bus.h` | 线程安全 EventBus 实现 |
| ievent_bus | `include/jui/bus/ievent_bus.h` | EventBus 抽象接口 |
| jui_interfaces | `include/jui/bus/jui_interfaces.h` | 接口聚合入口 |
| service_locator | `include/jui/bus/service_locator.h` | 类型安全 DI 容器 |
| state_base | `include/jui/render/state_base.h` | WidgetState 基类 |
| button_state | `include/jui/render/button_state.h` | Button 状态 |
| checkbox_state | `include/jui/render/checkbox_state.h` | CheckBox 状态 |
| grid_state | `include/jui/render/grid_state.h` | Grid 状态 |
| iwidget_state | `include/jui/render/iwidget_state.h` | WidgetState 接口 |
| list_state | `include/jui/render/list_state.h` | List 状态 |
| picker_state | `include/jui/render/picker_state.h` | ChoicePicker 状态 |
| slider_state | `include/jui/render/slider_state.h` | Slider 状态 |
| tabs_state | `include/jui/render/tabs_state.h` | Tabs 状态 |
| text_state | `include/jui/render/text_state.h` | Text 状态 |
| textfield_state | `include/jui/render/textfield_state.h` | TextField 状态 |
| toggle_state | `include/jui/render/toggle_state.h` | Toggle 状态 |
| widget_state | `include/jui/render/widget_state.h` | 状态类整体定义 |
