# JUI 项目框架文档

> **版本**：0.1.0  
> **更新日期**：2026-06-03  
> **目标平台**：Windows 10/11 x64  
> **语言标准**：C++17

---

## 目录

- [1. 项目概述](#1-项目概述)
- [2. 整体架构](#2-整体架构)
- [3. 目录结构](#3-目录结构)
- [4. 核心模块](#4-核心模块)
  - [4.1 Core 层](#41-core-层)
  - [4.2 Layout 层](#42-layout-层)
  - [4.3 Render 层](#43-render-层)
- [5. 模块间交互流程](#5-模块间交互流程)
- [6. 技术栈与依赖](#6-技术栈与依赖)
- [7. 构建系统](#7-构建系统)
- [8. 测试体系](#8-测试体系)

---

## 1. 项目概述

**JUI**（JSON-powered User Interface）是一个专为 Windows 桌面应用设计的**极简轻量级 C++ UI 引擎**。核心理念是将界面描述与渲染逻辑完全分离：用户界面通过 **A2UI 兼容的 JSON 协议** 声明，渲染层基于 **Direct2D + DirectWrite** 实现硬件加速绘制。

### 设计原则

| 原则 | 说明 |
|------|------|
| **逻辑-渲染分离** | `Widget`（纯数据）↔ `RenderWidget`（绘制），通过工厂映射 |
| **状态-视图分离** | `WidgetState` 管理控件状态，`RenderWidget` 负责绘制 |
| **JSON 驱动** | 一切界面由 JSON 描述，符合 A2UI v0.8 协议规范 |
| **虚拟滚动** | List/Grid 仅渲染可见区域，支持 10 万+ 数据项 |
| **DWrite 文字** | 所有文字处理基于 DirectWrite，原生支持 UTF-8/CJK |

---

## 2. 整体架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            JUIEngine（引擎入口）                         │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────────────────┐  │
│  │  processMessage│  │  setActionCall│  │  onMouseDown/Up/Move/Wheel  │  │
│  │   (JSON输入)   │  │   back(回调)  │  │  onKeyDown/Up/CharInput    │  │
│  └───────┬───────┘  └───────────────┘  │  onSize/initialize          │  │
│          │                              │  onIMEStart/Composition/End │  │
│          ▼                              └─────────────┬───────────────┘  │
│  ┌────────────────────────────────────────────────────┴───────────────┐ │
│  │  ┌─────────────┐   ┌──────────────┐   ┌────────────────────────┐  │ │
│  │  │   Surface    │◄──│  DataModel   │   │     LayoutEngine       │  │ │
│  │  │  (界面表面)  │   │ (数据绑定)   │   │  (布局计算)            │  │ │
│  │  │             │   │              │   │  measureWidget()        │  │ │
│  │  │ ┌─────────┐ │   │ JSON Pointer │   │  arrangeWidget()        │  │ │
│  │  │ │ Widget树 │ │   │  路径绑定   │   │    ├─ measureRow()      │  │ │
│  │  │ │(邻接表)  │ │   │              │   │    ├─ measureColumn()   │  │ │
│  │  │ └─────────┘ │   └──────────────┘   │    └─ measureCard()     │  │ │
│  │  └──────┬──────┘                       └───────────┬────────────┘  │ │
│  │         │                                          │                │ │
│  │         │    Widget.layoutBounds_ ←───────────────┘                │ │
│  │         ▼                                                          │ │
│  │  ┌──────────────────────────────────────────────────────────────┐ │ │
│  │  │                   D2D Renderer（渲染器）                      │ │ │
│  │  │  ┌─────────────────┐  ┌──────────────────────────────────┐  │ │ │
│  │  │  │  RenderWidget   │  │        WidgetState（状态）       │  │ │ │
│  │  │  │  (D2D绘制)      │  │  TextFieldWidgetState            │  │ │ │
│  │  │  │                 │  │  ButtonWidgetState               │  │ │ │
│  │  │  │ TextRenderWidget│  │  CheckBoxWidgetState             │  │ │ │
│  │  │  │ TextFieldRender │  │  ToggleWidgetState               │  │ │ │
│  │  │  │ ButtonRender    │  │  ChoicePickerWidgetState         │  │ │ │
│  │  │  │ CheckBoxRender  │  │  SliderWidgetState               │  │ │ │
│  │  │  │ ToggleRender    │  │  ListWidgetState                 │  │ │ │
│  │  │  │ ChoicePicker    │  │  GridWidgetState                 │  │ │ │
│  │  │  │ SliderRender    │  │  TabsWidgetState                 │  │ │ │
│  │  │  │ ListRender      │  │  TextWidgetState                 │  │ │ │
│  │  │  │ GridRender      │  └──────────────────────────────────┘  │ │ │
│  │  │  │ TabsRender      │                                         │ │ │
│  │  │  │ DividerRender   │  交互：hover/focus/press/drag/scroll   │ │ │
│  │  │  │ ImageRender     │                                         │ │ │
│  │  │  │ ContainerRender │  RenderWidgetFactory::create(type)      │ │ │
│  │  │  └─────────────────┘                                         │ │ │
│  │  └──────────────────────────────────────────────────────────────┘ │ │
│  └──────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

### 数据流方向

```
JSON 输入 → processMessage()
  ├─ createSurface         → 新建 Surface
  ├─ surfaceUpdate         → 解析组件树 → 构建 Widget 邻接表
  ├─ dataModelUpdate       → 更新 DataModel（JSON Pointer 路径绑定）
  ├─ beginRendering        → 触发 layout() + setSurface() → 刷新 RenderWidgets
  └─ deleteSurface         → 销毁 Surface

渲染循环（每帧 60fps）：
  layout() → Widget.layoutBounds_ → RenderWidget.setBounds() → paint()
  交互事件 → onMouse*/onKey* → WidgetState 状态更新 → needsRedraw → 下一帧重绘
```

---

## 3. 目录结构

```
jui/
├── CMakeLists.txt                # 顶层构建脚本（C++17 + MSVC + D2D/DWrite 链接）
├── include/jui/                  # === 公共头文件 ===
│   ├── jui.h                     #   统一入口（聚合所有子头文件）
│   ├── core/
│   │   ├── value.h               #   JValue：动态值类型（JSON 解析基础）
│   │   ├── widget.h              #   Widget：纯逻辑控件 + Rect/Size/Point 几何体
│   │   ├── data_model.h          #   DataModel：JSON Pointer 数据绑定
│   │   ├── surface.h             #   Surface：组件树 + 数据模型容器
│   │   └── engine.h              #   JUIEngine：PIMPL 隐藏的主引擎
│   ├── layout/
│   │   └── layout.h              #   LayoutEngine：measure/arrange 布局引擎
│   └── render/
│       ├── render_widget.h       #   RenderWidget 基类 + 13 种渲染控件声明
│       ├── widget_state.h        #   WidgetState 基类 + 8 种状态类
│       └── d2d_renderer.h        #   D2DRenderer：D2D 渲染器 + 剪贴板 + IME
├── src/                          # === 源文件 ===
│   ├── core/
│   │   ├── value.cpp             #   空（头文件即实现）
│   │   ├── widget.cpp            #   空
│   │   ├── data_model.cpp        #   空
│   │   ├── surface.cpp           #   空
│   │   └── engine.cpp            #   引擎核心（消息解析、JSON→Widget 转换）
│   ├── layout/
│   │   └── layout.cpp            #   布局引擎实现
│   └── render/
│       ├── render_widget.cpp     #   所有渲染控件实现（paint/measure/hitTest）
│       └── d2d_renderer.cpp      #   D2D 渲染器实现（render/输入事件/剪贴板/IME）
├── examples/
│   ├── CMakeLists.txt
│   └── hello/main.cpp            #   分页展示所有控件的 demo 程序
├── tests/                        # === 单元测试（Google Test）===
│   ├── test_value.cpp            #   JValue 测试（41 用例）
│   ├── test_widget.cpp           #   Widget/Rect/Size 测试（47 用例）
│   ├── test_data_model.cpp       #   DataModel 测试（21 用例）
│   ├── test_surface.cpp          #   Surface 测试（25 用例）
│   ├── test_engine.cpp           #   Engine 消息解析测试（37 用例）
│   ├── test_render_widget.cpp    #   RenderWidget 测试（34 用例）
│   ├── test_d2d_renderer.cpp     #   D2DRenderer 非渲染测试（19 用例）
│   ├── test_layout.cpp           #   LayoutEngine 测试（27 用例）
│   ├── test_widget_state.cpp     #   WidgetState 状态机测试（68 用例）
│   └── test_list_grid.cpp        #   List/Grid 功能+性能测试（39 用例）
├── thirdparty/
│   └── json.hpp                  #   nlohmann/json v3.12.0（单头文件）
└── docs/                         # === 文档 ===
    ├── ARCHITECTURE.md           #   本文件：框架文档
    └── USER_MANUAL.md            #   使用手册
```

---

## 4. 核心模块

### 4.1 Core 层

核心层负责数据建模，不依赖任何渲染或平台 API。

#### 4.1.1 JValue（`value.h`）

动态类型值，是 JSON 数据绑定的基础。

```
JValue
├── types: Null | String | Number | Boolean | Path | Array | Object
├── 构造: default, nullptr, const char*, string, double, int, bool
├── 工厂: fromPath(), fromArray(), fromObject()
├── 类型判断: isNull(), isString(), isNumber(), isBoolean(), isArray(), isObject()
├── 值访问: asString(), asNumber(), asInt(), asBool(), asArray(), asObject()
├── 对象操作: set(key, val), has(key), get(key)
└── 数组操作: push(val), size()
```

#### 4.1.2 Widget（`widget.h`）

纯逻辑控件，不包含任何渲染状态。

| 属性分类 | 字段/方法 | 说明 |
|---------|-----------|------|
| **基础** | `id`, `type`, `parentId` | 控件标识与类型 |
| **属性** | `properties_: map<string, JValue>` | 通用属性字典（text, fontSize, width 等） |
| **子组件** | `children_: Children{Mode, explicitList}` | 邻接表模型（父子通过 ID 关联） |
| **数据绑定** | `boundValues_: map<string, JValue>` | A2UI BoundValue 绑定 |
| **布局** | `layoutBounds_: Rect` | 经 LayoutEngine 计算后的最终位置尺寸 |
| **状态** | `visible_, enabled_` | 可见性与启用状态 |
| **动作** | `action_` | 交互触发的动作名（如 "click"） |
| **枚举** | `WidgetType` | 15 种控件类型（见下文） |

##### WidgetType 枚举

| 值 | JSON Key | 对应渲染类 | 说明 |
|----|---------|-----------|------|
| `Text` | `"Text"` | `TextRenderWidget` | 文字标签 |
| `Button` | `"Button"` | `ButtonRenderWidget` | 按钮 |
| `TextField` | `"TextField"` | `TextFieldRenderWidget` | 编辑框 |
| `CheckBox` | `"CheckBox"/"Checkbox"` | `CheckBoxRenderWidget` | 复选框 |
| `Toggle` | `"Toggle"` | `ToggleRenderWidget` | 开关 |
| `ChoicePicker` | `"ChoicePicker"/"Choicepicker"` | `ChoicePickerRenderWidget` | 下拉选择器 |
| `Slider` | `"Slider"` | `SliderRenderWidget` | 滑动条 |
| `Grid` | `"Grid"` | `GridRenderWidget` | 表格/网格 |
| `Tabs` | `"Tabs"` | `TabsRenderWidget` | 选项卡 |
| `Image` | `"Image"` | `ImageRenderWidget` | 图片 |
| `Row` | `"Row"` | `ContainerRenderWidget` | 水平布局容器 |
| `Column` | `"Column"` | `ContainerRenderWidget` | 垂直布局容器 |
| `Card` | `"Card"` | `ContainerRenderWidget` | 卡片容器 |
| `List` | `"List"` | `ListRenderWidget` | 虚拟滚动列表 |
| `Divider` | `"Divider"` | `DividerRenderWidget` | 分割线 |
| `Custom` | `"Custom"` | `ContainerRenderWidget` | 自定义 |

#### 4.1.3 DataModel（`data_model.h`）

基于 JSON Pointer（RFC 6901）路径的数据绑定模型。

- `setValue(path, value)` — 设置指定路径的值
- `getValue(path)` — 读取指定路径的值
- `updateContents(basePath, contents)` — 批量更新子树（A2UI dataModelUpdate）
- `clear()` — 清空所有数据
- `allData()` — 返回完整数据树
- `addListener(callback)` — 注册变化通知

#### 4.1.4 Surface（`surface.h`）

UI 表面，管理一个独立界面的完整生命周期。

- `id()` — 表面 ID
- `addWidget() / getWidget() / removeWidget()` — 组件增删查
- `rootWidget() / setRootWidget()` — 根组件
- `dataModel()` — 关联的数据模型
- `setSize() / width() / height()` — 表面尺寸
- `updateComponents()` — 批量更新组件（A2UI surfaceUpdate）
- `needsLayout() / setNeedsLayout()` — 脏标记

#### 4.1.5 JUIEngine（`engine.h`）

使用 **PIMPL 模式** 隐藏实现的引擎入口。

| 接口分组 | 方法 | 说明 |
|---------|------|------|
| **初始化** | `initialize(HWND)` | 初始化 D2D 渲染器 |
| **JSON 输入** | `processMessage(json)` | 处理 A2UI 协议消息（支持 JSONL） |
| **渲染** | `render()` | 执行一帧绘制（在 WM_PAINT 中调用） |
| **鼠标** | `onMouseMove/onMouseDown/onMouseUp` | 鼠标事件 |
| **鼠标滚轮** | `onMouseWheel(delta)` | 滚轮事件 |
| **键盘** | `onKeyDown/onKeyUp/onCharInput` | 键盘与字符输入 |
| **IME** | `onIMEStart/onIMEComposition/onIMEEnd` | 中文输入法 |
| **尺寸** | `onSize(w, h)` | 窗口尺寸变化 |
| **回调** | `setActionCallback(cb)` | 设置交互事件回调 |
| **工具** | `getSurface(id)` | 获取指定表面 |

##### processMessage 支持的消息类型

| A2UI 消息 | JSON 格式 | 作用 |
|-----------|----------|------|
| `createSurface` | `{"createSurface":{"surfaceId":"id"}}` | 创建新 Surface |
| `deleteSurface` | `{"deleteSurface":{"surfaceId":"id"}}` | 销毁 Surface |
| `surfaceUpdate` | `{"surfaceUpdate":{"surfaceId":"id","components":[...]}}` | 更新组件树（v0.8） |
| `updateComponents` | `{"updateComponents":{"surfaceId":"id","components":[...]}}` | 更新组件树（v0.9） |
| `dataModelUpdate` | `{"dataModelUpdate":{"surfaceId":"id","contents":[...]}}` | 更新数据模型 |
| `beginRendering` | `{"beginRendering":{"surfaceId":"id","root":"rootId"}}` | 开始渲染（触发布局） |

---

### 4.2 Layout 层

#### LayoutEngine（`layout.h` / `layout.cpp`）

纯计算层，不依赖 D2D/DWrite。使用 **两遍布局算法**：

1. **测量阶段（measureWidget）**：自底向上递归计算每个控件的自然尺寸
2. **排列阶段（arrangeWidget）**：自顶向下递归分配每个控件的最终位置-尺寸矩形

```
layout(root, containerSize, resolver)
  │
  ├── measureWidget(root, constraint)
  │   ├── 根据 WidgetType 返回各控件默认/估算尺寸
  │   ├── measureRow()：水平排列 → 宽度累加，高度取最大
  │   ├── measureColumn()：垂直排列 → 高度累加，宽度取最大
  │   └── measureCard()：内容 + 16px × 2 内边距
  │
  └── arrangeWidget(root, Rect{0,0,containerSize.w,containerSize.h})
      ├── 设置 widget.layoutBounds_
      ├── arrangeRow()：从左到右分配子控件位置
      ├── arrangeColumn()：从上到下分配子控件位置
      └── arrangeCard()：内缩 16px 后递归 ArrangeColumn
```

**尺寸解析（resolveSize）**：支持 `auto`（-1）、固定像素值（`200.0`）、百分比字符串（`"50%"`）。

---

### 4.3 Render 层

渲染层是项目中规模最大的模块，采用 **状态-视图分离** 设计。

#### 4.3.1 WidgetState（状态层）

每种控件都有对应的 `WidgetState` 子类，封装所有状态逻辑，**完全不涉及 D2D/DWrite 渲染代码**。

```
WidgetState（基类）
├── enabled / hovered / focused（通用交互状态）
├── onChange 通知回调
│
├── TextWidgetState          — 文本、overflow
├── TextFieldWidgetState     — 文本编辑、光标、选区、剪贴板、IME、输入模式
├── ButtonWidgetState        — 视觉状态（Normal→Hovered→Pressed→Disabled）
├── CheckBoxWidgetState      — 勾选状态（Unchecked→Checked→Indeterminate）
├── ToggleWidgetState        — 开关状态（toggled + label）
├── ChoicePickerWidgetState  — 下拉打开/关闭、选项列表、选中索引
├── SliderWidgetState        — 值、范围、步进、拖拽中
├── ListWidgetState          — 虚拟滚动、可见范围、选中、Provider
├── GridWidgetState          — 列定义、行数据、滚动、选中、排序
└── TabsWidgetState          — Tab 列表、activeIndex、命中检测
```

##### TextFieldWidgetState 状态维度

该状态类最为复杂，包含 6 个维度的状态：

| 维度 | 枚举 | 说明 |
|------|------|------|
| `InputMode` | Normal / Password / ReadOnly | 输入模式 |
| `IMEState` | Idle / Composing | IME 合成状态 |
| `DragState` | None / Selecting | 选区拖拽状态 |
| `EditAction` | InsertChar / InsertText / DeleteBack / DeleteForward / Cut / Paste / Clear | 编辑操作 |
| 光标 | `cursorPosition_` | 光标位置 |
| 选区 | `selectionStart_ / selectionEnd_` | 选中范围 |

##### List/Grid 虚拟滚动性能

- `ListWidgetState::visibleStart/visibleEnd` — 仅渲染可见范围（100,000 项中约 20 项）
- `GridWidgetState::visibleRowStart/visibleRowEnd` — 仅渲染可见行（50,000 行中约 12 行）
- `ListRenderWidget::paint` — 遍历 `visibleStart()` 到 `visibleEnd()` 跳过不可见项

#### 4.3.2 RenderWidget（视图层）

```
RenderWidget（基类）
├── bounds_ / setBounds / bounds
├── widget_ / setWidget / widget
├── state_ / setState / state
├── hitTest(x,y) / canFocus / canCopy / canPaste
├── paint(rt, dwrite) — 纯虚函数（D2D 绘制）
├── measure(dwrite) — 纯虚函数（DWrite 测量）
├── onMouseDown / onChar / onKeyDown
├── selectAll / copySelection / cutSelection / pasteText
│
├── TextRenderWidget           — DWrite 文字渲染（fontSize/fontWeight/textColor/textAlign）
├── TextFieldRenderWidget      — D2D 圆角背景 + 三态边框 + 闪烁光标 + 选区高亮
├── ButtonRenderWidget         — D2D 圆角 + 三态填充色（Normal/Hovered/Pressed/Disabled）
├── CheckBoxRenderWidget       — D2D 方框 + ✓ 勾号 + 标签文字
├── ToggleRenderWidget         — D2D 圆角轨道 + 圆形滑块
├── ChoicePickerRenderWidget   — D2D 下拉框 + ▼ 箭头 + 弹出选项列表
├── SliderRenderWidget         — D2D 轨道 + 填充 + 圆形拖拽手柄
├── ListRenderWidget           — D2D 虚拟滚动列表 + 选中高亮 + 滚动条指示器
├── GridRenderWidget           — D2D 固定表头 + 虚拟滚动数据行 + 排序箭头
├── TabsRenderWidget           — D2D Tab 头 + 激活态白底 + 蓝色底部指示条
├── ContainerRenderWidget      — 容器背景（Card: 圆角白底灰边框）
├── DividerRenderWidget        — D2D 1px 水平灰线
└── ImageRenderWidget          — 占位（reserve）
```

#### 4.3.3 D2DRenderer（渲染器）

- **D2D 资源管理**：`createDeviceResources()` / `discardDeviceResources()`
- **帧渲染**：`render()` — 同步 bounds → 绘制所有可见控件
- **输入事件分发**：鼠标/键盘/IME 事件路由到对应 RenderWidget
- **剪贴板**：`copyToClipboard()` / `pasteFromClipboard()`（Windows 原生 API）
- **交互管理**：hover 状态、focus 管理、`SetCapture`/`ReleaseCapture` 拖拽

#### 4.3.4 RenderWidgetFactory

```cpp
static RenderWidgetPtr create(WidgetPtr widget) {
    switch (widget->type()) {
        case Text:         → TextRenderWidget
        case Button:       → ButtonRenderWidget
        case TextField:    → TextFieldRenderWidget
        case CheckBox:     → CheckBoxRenderWidget
        case Toggle:       → ToggleRenderWidget
        case ChoicePicker: → ChoicePickerRenderWidget
        case Slider:       → SliderRenderWidget
        case List:         → ListRenderWidget
        case Grid:         → GridRenderWidget
        case Tabs:         → TabsRenderWidget
        case Row/Column/Card: → ContainerRenderWidget
        case Divider:      → DividerRenderWidget
        case Image:        → ImageRenderWidget
        default:           → nullptr
    }
}
```

---

## 5. 模块间交互流程

### 5.1 JSON 加载 → 渲染就绪

```
processMessage(json)
  │
  ├─ "createSurface" ────────────────────► EngineImpl 新建 Surface 加入 surfaces_ 字典
  │
  ├─ "surfaceUpdate"/"updateComponents" ──► 遍历 components[] 数组
  │                                          ├─ 解析 "component" 对象 → WidgetType
  │                                          ├─ 解析属性 → widget.setProperty(k, v)
  │                                          ├─ 解析 children.explicitList → widget.setChildren()
  │                                          └─ 解析 boundValues → widget.setBoundValue(k, v)
  │                                          surface.addWidget(widget)
  │
  ├─ "dataModelUpdate" ───────────────────► DataModel.updateContents(path, contents)
  │
  └─ "beginRendering" ────────────────────► surface.setRootWidget(rootId)
                                              │
                                              ├── refreshLayout()
                                              │   └── layoutEngine.layout(root, {w,h}, resolver)
                                              │       ├── measureWidget(root)
                                              │       └── arrangeWidget(root)
                                              │
                                              └── renderer.setSurface(surface)
                                                  └── refreshRenderWidgets()
                                                      ├── renderWidgets_.clear()
                                                      └── createRenderTree(root) ← recursive
```

### 5.2 渲染帧循环（每 16ms）

```
WM_TIMER (SetTimer 16ms)
  ↓ InvalidateRect
WM_PAINT
  ↓ BeginPaint
JUIEngine::render()
  ↓
D2DRenderer::render()
  ├── 计算 delta time
  ├── 更新聚焦控件状态（光标闪烁动画）
  ├── BeginDraw → Clear(White)
  │
  ├── for each (对 renderWidgets_.begin() → end() 正向遍历)
  │   ├── rw.setBounds(widget.layoutBounds())      ← 同步布局结果
  │   ├── List/Grid: ls.setViewportSize(bounds)    ← 同步视口尺寸
  │   └── rw.paint(rt, dwriteFactory)               ← D2D 绘制
  │
  ├── EndDraw
  └── needsRedraw_ = false
```

### 5.3 用户交互 → 状态更新

```
WM_LBUTTONDOWN
  ↓
D2DRenderer::onMouseDown(x, y)
  ├── 反向遍历 renderWidgets_（从后往前，找最内层命中控件）
  ├── 清除旧焦点
  ├── 根据命中控件的 WidgetType 分发：
  │   ├── Button    → ButtonWidgetState::press()
  │   ├── TextField → TextFieldWidgetState::mouseDown()
  │   ├── CheckBox  → CheckBoxWidgetState::toggle()
  │   ├── Toggle    → ToggleWidgetState::toggle()
  │   ├── ChoicePicker → ChoicePickerWidgetState::toggle()
  │   ├── Slider    → SliderWidgetState::setDragging(true) + SetCapture
  │   ├── Tabs      → TabsWidgetState::setActiveIndex() → actionCallback_
  │   ├── List (scrollbar) → setScrollOffset + setScrolling(true) + SetCapture
  │   └── List (item)      → ListRenderWidget::onClick → selectIndex
  ├── 设置焦点（canFocus()）
  └── needsRedraw_ = true

WM_MOUSEMOVE
  ↓
  ├── List isScrolling → 更新 scrollOffset
  ├── Slider isDragging → 更新 tempValue
  └── 更新 hover 状态

WM_MOUSEWHEEL
  ↓
  └── List/Grid → onScroll(delta)

WM_LBUTTONUP
  ↓
  ├── ReleaseCapture
  ├── List → setScrolling(false)
  ├── Slider → setDragging(false) → actionCallback_("slider_change")
  └── Button → release() → 如命中则 actionCallback_("click")
```

---

## 6. 技术栈与依赖

| 类别 | 技术/库 | 版本 | 用途 |
|------|---------|------|------|
| **语言** | C++ | 17 | 语言标准 |
| **编译器** | MSVC | Visual Studio 2022 | Windows 编译 |
| **构建系统** | CMake | 3.14+ | 跨平台构建 |
| **图形** | Direct2D | Windows SDK | 2D 矢量渲染 |
| **文字** | DirectWrite | Windows SDK | 字体渲染、文本测量 |
| **JSON** | nlohmann/json | v3.12.0 | A2UI JSON 解析（单头文件） |
| **输入法** | IMM32 | Windows SDK | 中文 IME 输入 |
| **系统** | Win32 API | — | 窗口、剪贴板、鼠标捕获 |
| **测试** | Google Test | v1.14.0 | 单元测试框架（FetchContent 自动下载） |

### Windows SDK 依赖模块

- `d2d1.lib` — Direct2D 图形 API
- `dwrite.lib` — DirectWrite 文字 API
- `windowscodecs.lib` — WIC 图像编解码
- `imm32.lib` — 输入法管理器（仅示例链接）

---

## 7. 构建系统

### CMake 目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `jui` | 静态库 | 核心引擎库 |
| `jui_hello` | 可执行文件 | 分页 Demo 程序（WIN32 GUI） |
| `jui_tests` | 可执行文件 | 单元测试套件（Google Test） |

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `JUI_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `JUI_BUILD_TESTS` | ON | 构建测试套件 |

### Windows 编译命令

```bash
# 配置
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 编译库 + 示例 + 测试
cmake --build build --config Debug

# 仅编译库
cmake --build build --config Debug --target jui

# 运行测试
build\Debug\jui_tests.exe

# 运行示例
build\examples\Debug\jui_hello.exe
```

### MSVC 编译选项

```
/utf-8        — 源文件 UTF-8 编码
/DNOMINMAX    — 禁用 windows.h 的 min/max 宏
```

---

## 8. 测试体系

### 测试文件覆盖

| 测试文件 | 测试套件数 | 用例数 | 覆盖模块 |
|---------|-----------|--------|---------|
| `test_value.cpp` | 1 | 41 | JValue 构造/类型/访问/边界 |
| `test_widget.cpp` | 7 | 47 | Widget/Rect/Size/属性/子组件/绑定 |
| `test_data_model.cpp` | 1 | 21 | DataModel CRUD/批量/路径/通知 |
| `test_surface.cpp` | 2 | 25 | Surface 组件管理/updateComponents |
| `test_engine.cpp` | 1 | 37 | 7 种 A2UI 消息/v0.8+v0.9 兼容/JSONL |
| `test_render_widget.cpp` | 13 | 34 | 基类/工厂/13 种渲染控件接口 |
| `test_d2d_renderer.cpp` | 1 | 19 | parseColor/初始化/防崩溃 |
| `test_layout.cpp` | 1 | 27 | resolveSize/layout/arrange |
| `test_widget_state.cpp` | 8 | 68 | 6 种状态类全覆盖状态机 |
| `test_list_grid.cpp` | 6 | 39 | List/Grid 功能+性能基准 |
| **合计** | **39** | **358** | — |

### 性能测试

- **List 虚拟范围计算**：10 万项 < 1ms
- **List 滚动 1000 次**：< 10ms
- **Grid 大表格滚动**：< 5ms
