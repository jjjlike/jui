# JUI 使用手册

> **版本**：0.1.0  
> **更新日期**：2026-06-03  
> **目标平台**：Windows 10/11 x64

---

## 目录

- [1. 环境配置与启动](#1-环境配置与启动)
  - [1.1 系统要求](#11-系统要求)
  - [1.2 依赖安装](#12-依赖安装)
  - [1.3 编译步骤](#13-编译步骤)
  - [1.4 集成到自己的项目](#14-集成到自己的项目)
  - [1.5 运行 Demo](#15-运行-demo)
- [2. 快速入门](#2-快速入门)
  - [2.1 最小示例](#21-最小示例)
  - [2.2 窗口消息集成](#22-窗口消息集成)
  - [2.3 A2UI JSON 协议](#23-a2ui-json-协议)
- [3. 控件详解](#3-控件详解)
  - [3.1 Text — 文字标签](#31-text--文字标签)
  - [3.2 Button — 按钮](#32-button--按钮)
  - [3.3 TextField — 编辑框](#33-textfield--编辑框)
  - [3.4 CheckBox — 复选框](#34-checkbox--复选框)
  - [3.5 Toggle — 开关](#35-toggle--开关)
  - [3.6 ChoicePicker — 下拉选择器](#36-choicepicker--下拉选择器)
  - [3.7 Slider — 滑动条](#37-slider--滑动条)
  - [3.8 List — 虚拟滚动列表](#38-list--虚拟滚动列表)
  - [3.9 Grid — 表格](#39-grid--表格)
  - [3.10 Tabs — 选项卡](#310-tabs--选项卡)
  - [3.11 布局容器（Row / Column / Card）](#311-布局容器row--column--card)
  - [3.12 Divider — 分割线](#312-divider--分割线)
- [4. 输入输出参数详解](#4-输入输出参数详解)
  - [4.1 processMessage JSON 消息格式](#41-processmessage-json-消息格式)
  - [4.2 Action 回调格式](#42-action-回调格式)
  - [4.3 Widget 属性参考表](#43-widget-属性参考表)
- [5. 常见问题排查指南](#5-常见问题排查指南)
  - [5.1 界面空白/无内容](#51-界面空白无内容)
  - [5.2 中文显示乱码](#52-中文显示乱码)
  - [5.3 编辑框无法输入/无法获取焦点](#53-编辑框无法输入无法获取焦点)
  - [5.4 按钮点击无响应](#54-按钮点击无响应)
  - [5.5 Tab 页面无法切换](#55-tab-页面无法切换)
  - [5.6 List/Grid 滚动不可用](#56-listgrid-滚动不可用)
  - [5.7 Slider 拖动后程序崩溃](#57-slider-拖动后程序崩溃)
  - [5.8 程序启动/切换页面崩溃](#58-程序启动切换页面崩溃)
- [附录 A：完整示例代码](#附录-a完整示例代码)
- [附录 B：A2UI 组件树示例](#附录-ba2ui-组件树示例)

---

## 1. 环境配置与启动

### 1.1 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 或 Windows 11（x64） |
| 编译器 | MSVC 2022 或更高版本 |
| CMake | 3.14 或更高版本 |
| Windows SDK | 10.0.19041.0 或更高（含 D2D/DWrite） |
| 磁盘空间 | ~50MB（含第三方库下载） |

### 1.2 依赖安装

项目所有依赖自动管理，无需手动安装：

- **nlohmann/json v3.12.0**：`thirdparty/json.hpp` 已内置，单头文件，无需额外操作
- **Google Test v1.14.0**：通过 CMake `FetchContent` 首次构建时自动下载

### 1.3 编译步骤

```powershell
# 1. 进入项目目录
cd jui

# 2. 创建构建目录
mkdir build; cd build

# 3. CMake 配置（Release 或 Debug）
cmake .. -G "Visual Studio 17 2022" -A x64

# 4. 编译全部目标
cmake --build . --config Debug

# 5. 运行测试（可选）
.\Debug\jui_tests.exe

# 6. 运行 Demo（可选）
.\examples\Debug\jui_hello.exe
```

> **提示**：首次编译 Google Test 会从 GitHub 下载源码，请确保网络连接正常。

### 1.4 集成到自己的项目

在 CMakeLists.txt 中添加：

```cmake
# 添加 jui 子目录
add_subdirectory(path/to/jui)

# 链接 jui 库
target_link_libraries(your_app PRIVATE jui)

# 添加头文件路径
target_include_directories(your_app PRIVATE
    ${CMAKE_SOURCE_DIR}/path/to/jui/include
    ${CMAKE_SOURCE_DIR}/path/to/jui/thirdparty
)
```

### 1.5 运行 Demo

Demo 程序 `jui_hello.exe` 通过**顶部 Tabs** 分页展示所有控件：

| Tab | 页面 | 展示内容 |
|-----|------|---------|
| Button | 按钮与文字 | 多种字号、颜色、长/短按钮 |
| TextField | 编辑框 | 用户名/邮箱输入、剪贴板、中文 IME |
| Check | 复选框与开关 | 3 个复选框 + 1 个开关控件 |
| Picker | 滑动条 | 2 个滑动条（音量 50% / 亮度 80%） |
| List | 虚拟滚动列表 | 100,000 项虚拟列表 |
| Grid | 表格 | 50,000 行 × 5 列虚拟表格 |

---

## 2. 快速入门

### 2.1 最小示例

创建"Hello World"级别的 JUI 应用：

```cpp
#include <jui/jui.h>

// 步骤 1: 创建引擎实例
jui::JUIEngine engine;

// 步骤 2: 初始化（传入窗口句柄）
engine.initialize(hwnd);

// 步骤 3: 加载 JSON 界面（A2UI 协议）
engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");

engine.processMessage(R"(
  {"surfaceUpdate":{"surfaceId":"main","components":[
    {"id":"root","component":{"Column":{"children":{"explicitList":["txt","btn"]}}}},
    {"id":"txt","component":{"Text":{"text":{"literalString":"Hello JUI!"}}}},
    {"id":"btn","component":{"Button":{"text":{"literalString":"Click Me"}}}}
  ]}}
)");

engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");

// 步骤 4: 设置交互回调
engine.setActionCallback([](const std::string& surfaceId,
                             const std::string& action,
                             const std::string& sourceId,
                             const std::string& context) {
    // 处理按钮点击等事件
});

// 步骤 5: 在 WM_PAINT 中渲染
engine.render();
```

### 2.2 窗口消息集成

将引擎事件与 Win32 窗口消息绑定：

```cpp
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        engine.initialize(hwnd);
        // ... 加载 JSON 界面 ...
        SetTimer(hwnd, 1, 16, nullptr);  // 60fps 定时器驱动渲染
        return 0;

    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
        engine.render();                // ← 一帧绘制
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        engine.onSize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        engine.onMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
        engine.onMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        engine.onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        engine.onMouseWheel(static_cast<float>(-delta * 40));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_CHAR:
        engine.onCharInput(static_cast<uint32_t>(wParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_KEYDOWN:
        engine.onKeyDown(static_cast<int>(wParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_IME_STARTCOMPOSITION:
        engine.onIMEStart();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_IME_COMPOSITION: {
        engine.onIMEStart();
        if (lParam & GCS_RESULTSTR) {
            // ... 读取 IME 结果字符串 → engine.onIMEEnd(utf8) ...
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_IME_ENDCOMPOSITION:
        engine.onIMEEnd("");
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
```

### 2.3 A2UI JSON 协议

JUI 遵循 **A2UI v0.8** 协议的四步加载流程：

```
Step 1: createSurface      → 创建 UI 表面
Step 2: surfaceUpdate      → 声明组件树 + 属性 + children 关联
Step 3: dataModelUpdate    → 注入数据（可选，通过 JSON Pointer 绑定）
Step 4: beginRendering     → 指定根组件 ID，触发布局计算并开始渲染
```

一个简单的登录表单 JSON：

```json
// Step 1
{"createSurface": {"surfaceId": "login"}}

// Step 2
{"surfaceUpdate": {"surfaceId": "login", "components": [
  {"id": "root",  "component": {"Column": {"children": {"explicitList": ["title","row1","row2","submit"]}}}},
  {"id": "title", "component": {"Text": {"text": {"literalString": "用户登录"}}}},
  {"id": "row1",  "component": {"Row": {"children": {"explicitList": ["lbl1","tf1"]}}}},
  {"id": "lbl1",  "component": {"Text": {"text": {"literalString": "账号："}}}},
  {"id": "tf1",   "component": {"TextField": {"placeholder": {"literalString": "请输入账号"}}}},
  {"id": "row2",  "component": {"Row": {"children": {"explicitList": ["lbl2","tf2"]}}}},
  {"id": "lbl2",  "component": {"Text": {"text": {"literalString": "密码："}}}},
  {"id": "tf2",   "component": {"TextField": {}}},
  {"id": "submit","component": {"Button": {"text": {"literalString": "登录"}, "action": "login"}}}
]}}

// Step 4
{"beginRendering": {"surfaceId": "login", "root": "root"}}
```

> **注意**：组件间通过 `children.explicitList` 的 **字符串 ID 列表** 关联，而非嵌套 JSON 对象。这是 A2UI 邻接表模型的核心设计。

---

## 3. 控件详解

### 3.1 Text — 文字标签

基于 **DirectWrite** 渲染的纯文字控件。

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | BoundValue | `""` | 文字内容. 支持 `"literalString"` 或 `"path"` 绑定 |
| `fontSize` | BoundValue | 14 | 字号（像素） |
| `fontWeight` | BoundValue | `"normal"` | 字重：`"normal"` 或 `"bold"` |
| `textColor` | BoundValue | `"#333333"` | 文字颜色（6 位 hex） |
| `textAlign` | BoundValue | `"left"` | 对齐方式：`"left"` / `"center"` / `"right"` |
| `width` / `height` | Number | auto | 显式宽高 |

**JSON 示例：**

```json
{"id":"txt1","component":{"Text":{"text":{"literalString":"Hello 世界"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}}
```

**代码动态修改：**

```cpp
auto txt = surface->getWidget("txt1");
txt->setProperty("text", JValue("新文字"));
txt->setProperty("textColor", JValue("#FF0000"));
```

### 3.2 Button — 按钮

D2D 渲染的圆角按钮，支持四态视觉反馈。

**视觉状态：** `Normal`（蓝色 #2E7DE0）→ `Hovered`（亮蓝 #378FEB）→ `Pressed`（深蓝 #2166CC）→ `Disabled`（灰色）

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | BoundValue | `"Button"` | 按钮文字 |
| `action` | String | `""` | 点击时触发的动作名 |
| `enabled` | Boolean | `true` | 启用状态 |
| `width` / `height` | Number | auto（min 70） | 按钮尺寸 |

**JSON 示例：**

```json
{"id":"btn1","component":{"Button":{"text":{"literalString":"提交"},"action":"submit"}}}
```

**回调处理：**

```cpp
engine.setActionCallback([](const std::string& sid, const std::string& action,
                             const std::string& src, const std::string& ctx) {
    if (action == "submit") {
        // 处理提交逻辑
    }
});
```

### 3.3 TextField — 编辑框

基于 **DirectWrite** + **D2D** 渲染的编辑框，完整键盘交互支持。

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | BoundValue | `""` | 文本内容 |
| `placeholder` | BoundValue | `""` | 占位提示文字 |
| `width` | Number | 150 | 宽度 |
| `enabled` | Boolean | `true` | 启用状态（禁用后不可编辑） |

**键盘快捷键：**

| 按键 | 功能 |
|------|------|
| ← → | 移动光标 |
| Shift + ← → | 扩展选区 |
| Home / End | 跳到行首/行尾 |
| Backspace | 删除光标前一个字符 |
| Delete | 删除光标后一个字符 |
| Ctrl + A | 全选 |
| Ctrl + C | 复制选中文本 |
| Ctrl + V | 粘贴剪贴板文本 |
| Ctrl + X | 剪切选中文本 |

**中文输入：**

支持 Windows IME 输入法（拼音、五笔等），通过 `WM_IME_COMPOSITION` 消息处理合成状态。

**JSON 示例：**

```json
{"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"请输入用户名"},"width":{"literalNumber":200}}}}
```

**代码获取/设置文本：**

```cpp
auto tf = surface->getWidget("tf1");
// 读取（通过 RenderWidget state）
auto* rw = renderer.findRenderWidget("tf1");
auto* tfs = dynamic_cast<TextFieldWidgetState*>(rw->state());
std::string text = tfs->text();

// 写入
tfs->setText("新文本");
```

### 3.4 CheckBox — 复选框

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | BoundValue | `""` | 标签文字 |
| `value` | Boolean | `false` | 初始勾选状态 |
| `enabled` | Boolean | `true` | 启用状态 |

**JSON 示例：**

```json
{"id":"cb1","component":{"CheckBox":{"text":{"literalString":"同意用户协议"},"value":{"literalBoolean":true}}}}
```

**状态获取：**

```cpp
auto* cks = dynamic_cast<CheckBoxWidgetState*>(rw->state());
bool checked = cks->isChecked();  // 当前勾选状态
cks->toggle();                    // 程序切换状态
```

### 3.5 Toggle — 开关

iOS 风格圆角轨道 + 圆形滑块控件。

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` | String | `""` | 标签文字 |
| `value` | Boolean | `false` | 开关状态 |

**JSON 示例：**

```json
{"id":"tg1","component":{"Toggle":{"label":{"literalString":"启用通知"},"value":{"literalBoolean":true}}}}
```

### 3.6 ChoicePicker — 下拉选择器

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `options` | Array | `[]` | 选项字符串列表 |
| `selectedIndex` | Number | 0 | 选中项索引 |

**JSON 示例：**

```json
{"id":"cp1","component":{"ChoicePicker":{}}}
```

> **提示**：当前 ChoicePicker 通过代码设置 options 数组，未来版本将支持 JSON 直接声明。

### 3.7 Slider — 滑动条

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `min` | Number | 0 | 最小值 |
| `max` | Number | 100 | 最大值 |
| `value` | Number | 0 | 当前值 |
| `step` | Number | 1 | 步进值 |

**JSON 示例：**

```json
{"id":"sl1","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":50}}}}
```

**值变化回调：**

```cpp
// 滑动条值变化时收到 action="slider_change"，context 为 JSON 数字字符串
if (action == "slider_change") {
    int value = std::stoi(context);
    // 更新关联的文字标签
}
```

**状态获取/设置：**

```cpp
auto* ss = dynamic_cast<SliderWidgetState*>(rw->state());
float val = ss->value();           // 读取
ss->setValue(75.0f);               // 设置
ss->displayValue();                // 拖拽中返回临时值，否则返回最终值
```

### 3.8 List — 虚拟滚动列表

**核心特性：** 10 万+ 数据项仅渲染约 20 项可见区域，性能提升 ~5000 倍。

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `itemCount` | Number | 0 | 列表项总数 |
| `itemHeight` | Number | 32 | 每项高度（像素） |
| `width` / `height` | Number | 200 / 200 | 列表尺寸 |

**ItemProvider：** 通过代码设置数据源函数：

```cpp
auto* ls = dynamic_cast<ListWidgetState*>(rw->state());
ls->setItemProvider([](int i) -> std::string {
    return "Item " + std::to_string(i);
});
```

**交互：**

| 操作 | 效果 |
|------|------|
| 点击列表项 | 选中该项（高亮） |
| 点击/拖拽右侧滚动条 | 跳转/连续滚动 |
| 鼠标滚轮 | 上下滚动（每次 40px） |

**状态接口：**

```cpp
ls->selectedIndex();          // 当前选中索引
ls->scrollOffset();           // 当前滚动位置
ls->scrollToIndex(500);       // 滚动到指定项
ls->visibleStart();           // 第一个可见项索引
ls->visibleEnd();             // 最后一个可见项索引
ls->setSelectionMode(Single); // 单选模式（保留 Multi 扩展点）
```

**JSON 示例：**

```json
{"id":"list1","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":400},"height":{"literalNumber":380}}}}
```

### 3.9 Grid — 表格

**核心特性：** 50,000 行 × 5 列仅渲染可见行，固定表头+虚拟行滚动。

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `colCount` | Number | 0 | 列数 |
| `colTitles` | Array | `[]` | 列标题字符串数组 |
| `colWidths` | Array | `[]` | 列宽数值数组 |
| `rowCount` | Number | 0 | 行数 |
| `width` / `height` | Number | 400 / 200 | 表格尺寸 |

**CellProvider：**

```cpp
auto* gs = dynamic_cast<GridWidgetState*>(rw->state());
gs->setCellProvider([](int row, int col) -> std::string {
    return "R" + std::to_string(row) + "C" + std::to_string(col);
});
```

**交互：**

| 操作 | 效果 |
|------|------|
| 点击单元格 | 选中该单元格（高亮） |
| 鼠标滚轮 | 垂直滚动 |
| 列宽 | 当前版本为固定列宽 |

**状态接口：**

```cpp
gs->selectedRow(); gs->selectedCol();  // 当前选中单元格
gs->scrollX(); gs->scrollY();          // 当前滚动位置
gs->visibleRowStart();                 // 第一个可见行
gs->toggleSort(colIdx);                // 排序切换
```

**JSON 示例：**

```json
{"id":"grid1","component":{"Grid":{"width":{"literalNumber":520},"height":{"literalNumber":380}}}}

// 通过代码设置列和数据：
auto gw = surface->getWidget("grid1");
gw->setProperty("colCount", JValue(5));
gw->setProperty("colTitles", JValue::fromArray({
    JValue("ID"), JValue("姓名"), JValue("邮箱"), JValue("电话"), JValue("城市")
}));
gw->setProperty("colWidths", JValue::fromArray({
    JValue(60.0), JValue(100.0), JValue(200.0), JValue(120.0), JValue(100.0)
}));
gw->setProperty("rowCount", JValue(50000));
```

### 3.10 Tabs — 选项卡

**属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `tabs` | Array | `[]` | Tab 对象数组：`{"title":"标签名","id":"componentId"}` |
| `activeIndex` | Number | 0 | 当前激活 Tab 索引 |

**JSON 示例：**

```json
{"id":"tabs1","component":{"Tabs":{"tabs":[
  {"title":"首页","id":"tab_home"},
  {"title":"设置","id":"tab_settings"},
  {"title":"关于","id":"tab_about"}
]}}}
```

**切换回调：**

```cpp
// Tabs 点击时收到 action="tab_switch", source 为 tab 的 id
if (action == "tab_switch") {
    if (src == "tab_home")    switchPage(0);
    if (src == "tab_settings") switchPage(1);
    if (src == "tab_about")   switchPage(2);
}
```

**切换页面典型模式：**

```cpp
void switchPage(int pageIndex) {
    g_page = pageIndex;
    engine.processMessage(R"({"deleteSurface":{"surfaceId":"main"}})");
    engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    engine.processMessage(PAGE_JSONS[pageIndex]);
    // 设置 Tab 高亮
    auto sur = engine.getSurface("main");
    auto tabsW = sur->getWidget("tabs");
    if (tabsW) tabsW->setProperty("activeIndex", JValue(pageIndex));
    engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
}
```

### 3.11 布局容器（Row / Column / Card）

**Row** — 水平排列子控件。所有子元素从左到右排布，高度取最大子元素高度。

**Column** — 垂直排列子控件。所有子元素从上到下排布，宽度 = 容器宽度（块级布局）。

**Card** — 带内边距（16px）的垂直容器，渲染圆角白色背景 + 灰色边框。

**公共属性：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `children` | Object | `{}` | 子组件 ID 列表 |
| `width` / `height` | Number | auto | 显式尺寸 |

**children 格式：**

```json
"children": {"explicitList": ["child1", "child2", "child3"]}
```

**JSON 示例：**

```json
// Row 示例（水平排列的按钮栏）
{"id":"toolbar","component":{"Row":{"children":{"explicitList":["btnA","btnB","btnC"]}}}}

// Column 示例（垂直排列的表单）
{"id":"form","component":{"Column":{"children":{"explicitList":["title","field1","field2","submit"]}}}}

// Card 示例
{"id":"card1","component":{"Card":{"children":{"explicitList":["card_title","card_body"]}}}}
```

### 3.12 Divider — 分割线

1px 灰色水平分割线，宽度自动填满容器。

**JSON 示例：**

```json
{"id":"div1","component":{"Divider":{}}}
```

---

## 4. 输入输出参数详解

### 4.1 processMessage JSON 消息格式

#### createSurface

```json
{"createSurface": {"surfaceId": "main"}}
```

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `surfaceId` | String | 是 | 表面唯一标识符 |

#### surfaceUpdate

```json
{"surfaceUpdate": {
  "surfaceId": "main",
  "components": [
    {
      "id": "widget_id",
      "component": {
        "ComponentType": {
          "property": {"literalString": "value"},
          "children": {"explicitList": ["child1", "child2"]}
        }
      }
    }
  ]
}}
```

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `surfaceId` | String | 是 | 目标表面 ID |
| `components` | Array | 是 | 组件对象数组（邻接表模型） |
| `components[].id` | String | 是 | 组件唯一 ID |
| `components[].component` | Object | 是 | 组件定义对象（key 为类型名） |

#### beginRendering

```json
{"beginRendering": {"surfaceId": "main", "root": "root"}}
```

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `surfaceId` | String | 是 | 目标表面 ID |
| `root` | String | 是 | 根组件 ID（布局递归的起点） |

#### deleteSurface

```json
{"deleteSurface": {"surfaceId": "main"}}
```

### 4.2 Action 回调格式

```cpp
using ActionCallback = std::function<void(
    const std::string& surfaceId,   // 触发动作的表面 ID
    const std::string& action,       // 动作名称
    const std::string& sourceId,     // 触发控件的 ID
    const std::string& context       // 附加上下文（JSON 字符串）
)>;
```

| 控件 | action 值 | context 内容 | 触发时机 |
|------|----------|-------------|---------|
| Button | `"click"`（或自定义） | `""` | 鼠标释放 |
| Tabs | `"tab_switch"` | `""` | 点击 Tab |
| Slider | `"slider_change"` | 数字字符串（如 `"75"`） | 拖拽释放 |

### 4.3 Widget 属性参考表

#### 通用属性

| 属性 | 适用控件 | 值类型 | 说明 |
|------|---------|--------|------|
| `width` | 全部 | Number | 显式宽度（px），-1 为 auto |
| `height` | 全部 | Number | 显式高度（px），-1 为 auto |
| `visible` | 全部 | Boolean | 可见性 |
| `enabled` | Button/TextField/CheckBox/Toggle | Boolean | 启用状态 |

#### 文字属性

| 属性 | 适用控件 | 值类型 | 示例/说明 |
|------|---------|--------|----------|
| `text` | Text/Button/CheckBox | BoundValue | `{"literalString":"文字"}` 或 `{"path":"/data/title"}` |
| `fontSize` | Text | Number | 14 |
| `fontWeight` | Text | String | `"normal"` 或 `"bold"` |
| `textColor` | Text | String | `"#2266CC"`（6 位 hex） |
| `textAlign` | Text | String | `"left"` / `"center"` / `"right"` |
| `placeholder` | TextField | String | 占位提示 |
| `label` | Toggle | String | 标签文字 |

#### 数值属性

| 属性 | 适用控件 | 值类型 | 说明 |
|------|---------|--------|------|
| `min` / `max` | Slider | Number | 滑动条范围 |
| `value` | Slider/CheckBox/Toggle | Number/Boolean | 初始值 |
| `step` | Slider | Number | 步进值 |
| `itemCount` | List | Number | 列表项数 |
| `itemHeight` | List | Number | 项高度 |
| `rowCount` | Grid | Number | 表格行数 |
| `colCount` | Grid | Number | 表格列数 |

#### 数组/对象属性

| 属性 | 适用控件 | 值类型 | 说明 |
|------|---------|--------|------|
| `tabs` | Tabs | Array | `[{"title":"标签","id":"comId"}]` |
| `colTitles` | Grid | Array | 列标题字符串数组 |
| `colWidths` | Grid | Array | 列宽数值数组 |

---

## 5. 常见问题排查指南

### 5.1 界面空白/无内容

**症状：** 窗口显示为纯白色，没有任何控件。

**排查步骤：**

1. **确认 JSON 正确加载** — 检查 `processMessage` 是否按顺序调用了 4 步协议
2. **检查组件树连通性** — `root` 组件的 `children.explicitList` 必须包含所有子组件 ID，每个子组件必须在 `components` 数组中定义
3. **确认 `beginRendering` 的 root ID 正确** — `root` 参数必须与某组件的 `id` 匹配
4. **检查控件可见性** — `visible` 是否设为 `false`
5. **确认 Tabs 控件高度** — Tabs 控件仅占据 header 高度（32px），内容区需由其他控件填充

### 5.2 中文显示乱码

**症状：** 中文显示为 `???` 或方框。

**原因：** JUI 内部使用 `MultiByteToWideChar(CP_UTF8)` 进行 UTF-8 → UTF-16 转换。
- 确保源文件保存为 **UTF-8 编码**
- CMake 已配置 `/utf-8` 编译选项
- 如果通过代码动态设置文本，确保字符串是有效 UTF-8

### 5.3 编辑框无法输入/无法获取焦点

**症状：** 点击编辑框后无法输入文字。

**排查步骤：**

1. **确认 hitTest 穿透** — 如果有 Row/Column 容器包围 TextField，确认容器不消耗焦点（`canFocus() = false`）
2. **确认键盘事件已转发** — `WM_CHAR` → `engine.onCharInput()`
3. **确认 `enabled` 为 true** — 禁用的编辑框不接受输入
4. **中文输入** — 确保实现了 `WM_IME_COMPOSITION` 消息处理

### 5.4 按钮点击无响应

**症状：** 点击按钮后 `actionCallback` 没有被调用。

**排查步骤：**

1. **确认 `setActionCallback` 已设置** — 必须在交互前绑定回调
2. **确认按钮 `action` 属性已设置** — 空 action 不触发回调
3. **确认 hitTest 穿透** — 按钮被容器包围时，确认容器不阻断命中检测
4. **确认鼠标在按钮区域内释放** — 只有在按钮区域内释放才触发回调

### 5.5 Tab 页面无法切换

**症状：** 点击不同 Tab 后页面不切换。

**排查步骤：**

1. **确认 Tab 切换回调正确** — 检查 `action == "tab_switch"` 分支
2. **检查 tab id 匹配** — Tabs JSON 中 `"id"` 字段值必须与回调中的 `src` 匹配
3. **确认 `switchPage` 调用了 beginRendering** — 删除+重建 Surface 后必须重新 beginRendering
4. **确认 `activeIndex` 属性已设置** — 重建后需 `tabsW->setProperty("activeIndex", JValue(p))`
5. **确认 `setActionCallback` 已同步到渲染器** — 引擎的 callback 会自动同步到 D2DRenderer

### 5.6 List/Grid 滚动不可用

**症状：** 列表滚动条可见但点击/拖拽无反应。

**排查步骤：**

1. **确认 viewport 已同步** — `render()` 每帧调用 `ls->setViewportSize(bounds.w, bounds.h)`
2. **确认鼠标拖拽** — `onMouseDown` 中需调用 `ls->setScrolling(true)` + `SetCapture(hwnd_)`
3. **确认滚轮转发** — `WM_MOUSEWHEEL` → `engine.onMouseWheel(delta)`
4. **确认 itemCount > 0** — 空列表没有可滚动的内容
5. **检查 Grid 列数据** — Grid 需要在 `beginRendering` 前通过代码设置 `colCount`/`colTitles`/`colWidths`/`rowCount`

### 5.7 Slider 拖动后程序崩溃

**症状：** 拖动 Slider 后程序闪退。

**原因：** `actionCallback` 的 context 为空字符串 → `std::stod("")` 抛出异常。

**解决方案：**

```cpp
// 引擎内部已将 JValue 数字转为字符串，示例中加空检查：
if (action == "slider_change" && !ctx.empty()) {
    int val = std::stoi(ctx);
    // ...
}
```

### 5.8 程序启动/切换页面崩溃

**症状：** 程序一闪而过或切换页面时崩溃。

**排查步骤：**

1. **确认 CoInitializeEx 已调用** — D2D 需要 COM 初始化
2. **确认 `processMessage` 格式正确** — JSON 字符串必须有效，使用 `R"JSON(...)JSON"` 避免转义问题
3. **确认页面切换时的安全返回** — 在 Tabs 回调后，`onMouseDown` 必须 `return` 避免访问已销毁的 render tree
4. **确认 Divider 类型** — 目前 Divider 的 layout 代码已修复（之前为死代码）
5. **使用 DebugLogger 诊断** — 开启 DebugLogger 记录 Layout/Paint 全流程（见 5.9）

### 5.9 调试日志系统 (DebugLogger)

JUI 提供了专供自动化测试和调试的日志系统 `DebugLogger`。它能记录 Layout（Measure + Arrange）和 Paint 阶段的控件区域、文本内容、颜色等详细信息，支持按控件ID过滤和全局开关。

#### 5.9.1 基本用法

```cpp
#include "jui/test/test.h"
using namespace jui::test;

// 1. 开启日志
DebugLogger::instance().enable(true);

// 2. 可选：只记录特定控件
DebugLogger::instance().addWidgetFilter("txt");
DebugLogger::instance().addWidgetFilter("btn1");

// 3. 加载并渲染 UI（日志自动记录）
engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
engine.processMessage(R"({... surfaceUpdate ...})");
engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":800,"height":600}})");
engine.render();

// 4. 关闭日志
DebugLogger::instance().enable(false);

// 5. 检索日志
auto txtLogs = DebugLogger::instance().getLogs("txt");
for (const auto& entry : txtLogs) {
    std::cout << entry.toString() << std::endl;
}

// 6. 导出全部日志
std::string dump = DebugLogger::instance().exportToString();
DebugLogger::instance().exportToFile("layout_debug.log");
```

#### 5.9.2 日志阶段说明

| 阶段 (stage) | 位置 | 记录内容 |
|:--|:--|:--|
| `Measure` | `layout.cpp:measureWidget()` | 控件测量的尺寸 (Local 坐标系) |
| `Arrange` | `layout.cpp:arrangeWidget()` | 控件最终布局位置和尺寸 (Screen 坐标系) |
| `Paint` | `d2d_renderer.cpp:render()` | 绘制时的 bounds (Screen 坐标系，与 Arrange 应一致) |
| `PaintText` | `render_widget.cpp:TextRenderWidget::paint()` | 文字绘制区域、文本内容、颜色 RGB 值 |
| `Verify` | `DebugLogger::verifyBoundsConsistency()` | Layout 与 Paint 区域不一致时的错误日志 |

#### 5.9.3 日志格式示例

```
MEASURE[Local] id=txt type=1 constraint=(800,600) measured=(106,24) explicitW=-1 explicitH=-1
ARRANGE[Screen] id=txt type=1 bounds=(0,0)-(800x24)
PAINT[Screen] id=txt type=1 bounds=(0,0)-(800x24) visible=1
PAINT_TEXT[D2D] id=txt text="Hello, JUI!" rect=(0,0)-(800x24) color=(0.13,0.40,0.80) fontSize=16
```

#### 5.9.4 完整配置

```cpp
DebugLogConfig cfg;
cfg.enabled = true;                     // 全局开关（默认关闭）
cfg.minLevel = LogLevel::Debug;         // 最低日志级别 (Debug/Info/Warning/Error)
cfg.widgetFilters = {"txt", "btn1"};   // 控件ID白名单（空=全部）
cfg.includeTimestamp = true;            // 包含时间戳
cfg.includeLocation = true;             // 包含文件名和行号

// 自定义输出目标
cfg.sink = [](const LogEntry& e) {
    std::ofstream("debug.log", std::ios::app) << e.toString() << std::endl;
};

DebugLogger::instance().setConfig(cfg);
```

#### 5.9.5 日志级别过滤

```cpp
// 只记录 Warning 及以上级别的日志（忽略 Debug/Info）
cfg.minLevel = LogLevel::Warning;

// 代码中使用不同级别的宏
JUI_DEBUG_LOG("id", "Stage", "detailed info: %s", detail);     // Debug 级
JUI_INFO_LOG("id", "Stage", "general info");                   // Info 级
JUI_WARN_LOG("id", "Stage", "potential issue: %d", code);      // Warning 级
JUI_ERROR_LOG("id", "Stage", "critical error!");               // Error 级
```

#### 5.9.6 编排到自动化测试

```cpp
// 完整示例：验证 Layout 与 API 一致性
TEST(MyTest, LayoutConsistency) {
    DebugLogger::instance().enable(true);
    DebugLogger::instance().addWidgetFilter("txt");

    // 加载 UI（触发 Layout 和 Render）
    LoadMyUI();

    DebugLogger::instance().enable(false);

    // 从日志读取 Arrange bounds
    auto logs = DebugLogger::instance().getLogs("txt");
    float logX=0, logY=0, logW=0, logH=0;
    for (auto& e : logs) {
        if (e.stage == "Arrange") { /* 解析 bounds */ break; }
    }

    // 直接从 Widget API 交叉验证
    auto w = surface->getWidget("txt");
    EXPECT_NEAR(logX, w->layoutBounds().x, 0.5f);
    EXPECT_NEAR(logY, w->layoutBounds().y, 0.5f);
}
```

#### 5.9.7 注意事项

- **默认关闭**：性能无影响，仅在需要时开启
- **headless 限制**：无 D2D 渲染上下文时不产生 Paint/PaintText 日志（仅 Measure + Arrange）
- **控件过滤**：设置白名单后，不在白名单中的控件不产生任何日志
- **线程安全**：日志记录使用 mutex 保护，支持多线程调用

---

## 附录 A：完整示例代码

详见项目 `examples/hello/main.cpp`，展示了所有控件的完整用法，包括：

- 6 页 Tabs 分页展示
- 文字（多字号、多颜色、粗体）
- 按钮（默认 / 带 action / 不同宽度）
- 编辑框（用户名 / 邮箱 / 剪贴板 / IME）
- 复选框 / 开关
- 滑动条（音量 / 亮度，带文字联动）
- 虚拟滚动列表（100,000 项，点击选中 + 滚动条拖拽 + 滚轮）
- 虚拟表格（50,000 行 × 5 列，带排序箭头）

## 附录 B：A2UI 组件树示例

完整的 6 页面 JSON 树结构：

```
Surface("main")
├── Tabs("tabs")
│   ├── Tab("Button" → tbtn)
│   ├── Tab("TextField" → ttf)
│   ├── Tab("Check" → tck)
│   ├── Tab("Picker" → tpk)
│   ├── Tab("List" → tls)
│   └── Tab("Grid" → tgd)
├── Divider("d")
├── Text("hdr")                    ← 页面标题
└── (每页不同内容)
    Page 0: Text×3 + Row(Button×3)
    Page 1: Row(Text+TextField)×2 + Text
    Page 2: Text + CheckBox×3 + Toggle
    Page 3: Text+Slider × 2 (with text labels)
    Page 4: List(100,000 items)
    Page 5: Grid(50,000 rows × 5 cols)
```
