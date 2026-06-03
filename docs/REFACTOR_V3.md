# JUI 引擎模块化重构设计文档 v3.0

> **目标**：将 5,573 行单体架构解耦为 ~30 个独立模块，每模块 ≤ 300 行  
> **原则**：模块间通过 EventBus / 依赖注入接口交互，零直接依赖  
> **日期**：2026-06-03

---

## 1. 现状诊断

### 1.1 当前问题

| 问题 | 文件 | 行数 | 影响 |
|------|------|------|------|
| 巨型渲染文件 | `render_widget.cpp` | 867 | 14 种控件的 paint/measure/hitTest 混在一起 |
| 渲染+事件耦合 | `d2d_renderer.cpp` | 450 | 帧循环 + 输入处理 + Surface 管理混在一起 |
| 引擎上帝类 | `engine.cpp` | 415 | A2UI 解析 + Layout + DataModel + 组件树全在一个类 |
| 状态类膨胀 | `widget_state.h` | 593 | 8 种状态类无隔离 |
| 无抽象接口 | 全局 | — | 模块间直接 `#include` 对方的头文件 |

### 1.2 依赖图（当前）

```
engine.cpp ──include──> surface.h, widget.h, d2d_renderer.h, layout.h
d2d_renderer.cpp ──include──> render_widget.h, widget_state.h, surface.h
render_widget.cpp ──include──> render_widget.h, widget_state.h, widget.h
layout.cpp ──include──> layout.h, widget.h
```

每个模块直接依赖具体实现，无法替换、无法隔离测试。

---

## 2. 目标架构

### 2.1 核心思想：EventBus + DI 容器

```
┌────────────────────────────────────────────────────────────────┐
│                        EventBus (消息总线)                       │
│  SurfaceCreated | WidgetAdded | LayoutCompleted | PaintBegin    │
│  ActionFired | DataModelChanged | SurfaceDeleted | Resized      │
└────────┬────────┬────────┬────────┬────────┬────────┬──────────┘
         │        │        │        │        │        │
    ┌────▼──┐ ┌──▼───┐ ┌──▼───┐ ┌──▼───┐ ┌──▼───┐ ┌──▼──────┐
    │A2UI   │ │Widget│ │Layout│ │Render │ │Input  │ │Inspector│
    │Parser │ │Tree  │ │Engine│ │Pipeline│ │Handler│ │Module  │
    └───────┘ └──────┘ └──────┘ └───────┘ └───────┘ └─────────┘
         │        │        │        │        │        │
         └────────┴────────┴────┬───┴────────┴────────┘
                                │
                    ┌───────────▼───────────┐
                    │    ServiceLocator     │
                    │  (DI 容器 / 接口注册)  │
                    │  IWidgetTree          │
                    │  IRenderTarget        │
                    │  ILayoutEngine        │
                    │  IInputDispatcher     │
                    └───────────────────────┘
```

### 2.2 模块划分清单

#### 第 0 层：基础类型（零依赖）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M00 | `jvalue` | `core/value.h` | 123 | JSON 值抽象 |
| M01 | `rect_size` | `core/geometry.h` | 40 | Rect/Size 基础结构 |
| M02 | `widget_types` | `core/widget_types.h` | 35 | WidgetType 枚举 + Children 结构 |

#### 第 1 层：接口定义（仅依赖 M0x）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| I00 | `ievent_bus` | `bus/ievent_bus.h` | 60 | EventBus 抽象接口 |
| I01 | `iwidget_tree` | `core/iwidget_tree.h` | 50 | Widget 树查询接口 |
| I02 | `idata_model` | `core/idata_model.h` | 40 | DataModel 读写接口 |
| I03 | `irender_target` | `render/irender_target.h` | 35 | 渲染目标抽象 |
| I04 | `ilayout_engine` | `layout/ilayout_engine.h` | 30 | 布局引擎接口 |
| I05 | `iinput_dispatcher` | `input/iinput_dispatcher.h` | 30 | 输入分发接口 |
| I06 | `iwidget_state` | `render/iwidget_state.h` | 25 | WidgetState 基类 |

#### 第 2 层：EventBus 实现

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M03 | `event_bus` | `bus/event_bus.h` | 120 | 线程安全 EventBus（header-only） |
| M04 | `event_types` | `bus/event_types.h` | 80 | 事件类型枚举 + 事件数据 struct |

#### 第 3 层：核心服务实现（仅依赖 Ixx 接口 + M0x）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M05 | `widget_tree` | `core/widget_tree.h` | 130 | Widget 树存储 + 查询 |
| M06 | `widget_impl` | `core/widget_impl.h` | 150 | Widget 节点（id/type/properties/children/bounds） |
| M07 | `data_model` | `core/data_model_impl.h` | 110 | DataModel 实现（JSON Pointer 路径） |
| M08 | `surface` | `core/surface.h` | 80 | Surface 容器 |

#### 第 4 层：渲染模块（按控件拆分）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M09 | `text_renderer` | `render/text_renderer.h` | 120 | Text 控件 paint/measure |
| M10 | `button_renderer` | `render/button_renderer.h` | 100 | Button 控件 paint/measure |
| M11 | `textfield_renderer` | `render/textfield_renderer.h` | 220 | TextField 控件 paint/measure/IME/光标 |
| M12 | `checkbox_renderer` | `render/checkbox_renderer.h` | 90 | CheckBox 控件 |
| M13 | `toggle_renderer` | `render/toggle_renderer.h` | 70 | Toggle 控件 |
| M14 | `picker_renderer` | `render/picker_renderer.h` | 130 | ChoicePicker 控件 |
| M15 | `slider_renderer` | `render/slider_renderer.h` | 120 | Slider 控件 |
| M16 | `list_renderer` | `render/list_renderer.h` | 180 | List 虚拟滚动 |
| M17 | `grid_renderer` | `render/grid_renderer.h` | 200 | Grid 表格 |
| M18 | `tabs_renderer` | `render/tabs_renderer.h` | 100 | Tabs 渲染 |
| M19 | `container_renderer` | `render/container_renderer.h` | 80 | Row/Column/Card/Divider |
| M20 | `image_renderer` | `render/image_renderer.h` | 70 | Image 占位渲染 |
| M21 | `render_factory` | `render/render_factory.h` | 80 | 控件类型→渲染器映射表 |
| M22 | `d2d_context` | `render/d2d_context.h` | 120 | D2D RT/DWrite/画刷管理 |

#### 第 5 层：控件状态模块（按类型拆分）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M23 | `widget_state_base` | `render/state_base.h` | 40 | WidgetState 基类 |
| M24 | `text_state` | `render/text_state.h` | 30 | Text 状态 |
| M25 | `button_state` | `render/button_state.h` | 50 | Button 状态 |
| M26 | `textfield_state` | `render/textfield_state.h` | 130 | TextField 状态 |
| M27 | `checkbox_state` | `render/checkbox_state.h` | 55 | CheckBox 状态 |
| M28 | `toggle_state` | `render/toggle_state.h` | 35 | Toggle 状态 |
| M29 | `picker_state` | `render/picker_state.h` | 60 | Picker 状态 |
| M30 | `slider_state` | `render/slider_state.h` | 50 | Slider 状态 |
| M31 | `list_state` | `render/list_state.h` | 80 | List 状态 |
| M32 | `grid_state` | `render/grid_state.h` | 90 | Grid 状态 |
| M33 | `tabs_state` | `render/tabs_state.h` | 55 | Tabs 状态 |

#### 第 6 层：布局 + 输入 + 消息

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M34 | `layout_engine` | `layout/layout_engine.h` | 240 | 两遍布局算法 |
| M35 | `input_dispatcher` | `input/input_dispatcher.h` | 160 | 输入事件路由 |
| M36 | `a2ui_parser` | `core/a2ui_parser.h` | 200 | JSON → Widget 树解析 |
| M37 | `rendering_loop` | `render/rendering_loop.h` | 100 | 帧循环（BeginDraw→Paint→EndDraw） |

#### 第 7 层：组合层（DI 装配）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M38 | `engine_facade` | `core/engine_facade.h` | 180 | JUIEngine 外观类，组装所有模块 |
| M39 | `service_locator` | `bus/service_locator.h` | 80 | DI 容器 |

#### 辅助模块（非核心）

| # | 模块名 | 文件 | 行数 | 职责 |
|---|--------|------|------|------|
| M40 | `debug_logger` | 已有 | — | 不变 |
| M41 | `inspector` | 已有 | — | 不变 |
| M42 | `test_driver` | 已有 | — | 不变 |
| M43 | `test_validator` | 已有 | — | 不变 |
| M44 | `test_recorder` | 已有 | — | 不变 |

---

## 3. 统一接口规范

### 3.1 IEventBus — 事件总线

```cpp
namespace jui::bus {

enum class EventType {
    SurfaceCreated, SurfaceDeleted,
    WidgetAdded, WidgetRemoved,
    LayoutCompleted,
    PaintBegin, PaintEnd,
    ActionFired,
    DataModelChanged,
    Resized,
    InputEvent,  // mouse down/up/move, key, wheel
    FocusChanged,
};

struct Event {
    EventType type;
    std::string surfaceId;
    std::string widgetId;
    std::any data;  // 事件负载
};

class IEventBus {
public:
    virtual ~IEventBus() = default;
    using Handler = std::function<void(const Event&)>;
    virtual void subscribe(EventType type, Handler handler) = 0;
    virtual void publish(const Event& event) = 0;
    virtual void publish(EventType type, const std::string& surfaceId = "",
                         const std::string& widgetId = "", std::any data = {}) = 0;
};

} // namespace jui::bus
```

### 3.2 IWidgetTree — Widget 树接口

```cpp
namespace jui::core {

struct IWidgetNode {
    virtual ~IWidgetNode() = default;
    virtual const std::string& id() const = 0;
    virtual WidgetType type() const = 0;
    virtual bool visible() const = 0;
    virtual Rect layoutBounds() const = 0;
    virtual void setLayoutBounds(const Rect&) = 0;
    virtual const std::vector<std::string>& childIds() const = 0;
    virtual JValue property(const std::string& key) const = 0;
    virtual void setProperty(const std::string& key, const JValue& val) = 0;
};

class IWidgetTree {
public:
    virtual ~IWidgetTree() = default;
    virtual IWidgetNode* root() = 0;
    virtual IWidgetNode* get(const std::string& id) = 0;
    virtual void add(std::unique_ptr<IWidgetNode> node) = 0;
    virtual void remove(const std::string& id) = 0;
    virtual void forEach(std::function<void(IWidgetNode&)>) = 0;
    virtual size_t size() const = 0;
};

} // namespace jui::core
```

### 3.3 IRenderTarget — 渲染目标抽象

```cpp
namespace jui::render {

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual void beginDraw() = 0;
    virtual void endDraw() = 0;
    virtual void clear(float r, float g, float b) = 0;
    virtual void drawText(const std::wstring& text, const Rect& rect,
                          float fontSize, const std::string& fontWeight,
                          float r, float g, float b, float a = 1.0f) = 0;
    virtual void drawRect(const Rect& rect, float r, float g, float b,
                          float radius = 0, float a = 1.0f) = 0;
    virtual Size measureText(const std::string& text, float fontSize,
                             const std::string& fontWeight) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

} // namespace jui::render
```

### 3.4 ILayoutEngine — 布局引擎接口

```cpp
namespace jui::layout {

class ILayoutEngine {
public:
    virtual ~ILayoutEngine() = default;
    virtual void layout(core::IWidgetTree& tree, const Size& container) = 0;
    virtual Size measure(core::IWidgetNode& node, const Size& constraint) = 0;
};

} // namespace jui::layout
```

### 3.5 IInputDispatcher — 输入分发接口

```cpp
namespace jui::input {

class IInputDispatcher {
public:
    virtual ~IInputDispatcher() = default;
    virtual void dispatchMouseDown(int x, int y, int button) = 0;
    virtual void dispatchMouseUp(int x, int y, int button) = 0;
    virtual void dispatchMouseMove(int x, int y) = 0;
    virtual void dispatchMouseWheel(float delta) = 0;
    virtual void dispatchKeyDown(int vk) = 0;
    virtual void dispatchKeyUp(int vk) = 0;
    virtual void dispatchChar(uint32_t ch) = 0;
};

} // namespace jui::input
```

### 3.6 IWidgetRenderer — 控件渲染器接口

```cpp
namespace jui::render {

class IWidgetRenderer {
public:
    virtual ~IWidgetRenderer() = default;
    virtual void paint(IRenderTarget& rt, core::IWidgetNode& widget,
                       IWidgetState* state) = 0;
    virtual Size measure(IRenderTarget& rt, core::IWidgetNode& widget) = 0;
    virtual bool hitTest(core::IWidgetNode& widget, float x, float y) = 0;
    virtual bool canFocus() const { return false; }
    virtual WidgetType supportedType() const = 0;
};

} // namespace jui::render
```

---

## 4. 交互流程

### 4.1 应用启动流程

```
WinMain
  │
  ▼
EngineFacade::initialize(hwnd)
  │
  ├─► ServiceLocator::register<IEventBus>(new EventBus)
  ├─► ServiceLocator::register<IRenderTarget>(new D2DRenderTarget(hwnd))
  ├─► ServiceLocator::register<IWidgetTree>(new WidgetTree)
  ├─► ServiceLocator::register<ILayoutEngine>(new LayoutEngine)
  ├─► ServiceLocator::register<IInputDispatcher>(new InputDispatcher)
  ├─► ServiceLocator::register<RenderFactory>(new RenderFactory)
  │
  └─► EventBus::publish(EngineInitialized)
```

### 4.2 JSON 加载流程

```
EngineFacade::processMessage(json)
  │
  ▼
A2UIParser::parse(json)
  │
  ├─► createSurface → EventBus::publish(SurfaceCreated, sid)
  │   └─► WidgetTree::add(root)
  │
  ├─► surfaceUpdate → 遍历 components[]
  │   └─► WidgetTree::add(node) × N
  │       └─► EventBus::publish(WidgetAdded, sid, nodeId)
  │
  └─► beginRendering
      ├─► DataModel::bindAll(WidgetTree)
      ├─► LayoutEngine::layout(WidgetTree, containerSize)
      │   └─► EventBus::publish(LayoutCompleted)
      └─► RenderFactory::buildRenderTree(WidgetTree)
```

### 4.3 渲染帧流程

```
WM_PAINT → EngineFacade::render()
  │
  ▼
RenderingLoop::tick()
  │
  ├─► EventBus::publish(PaintBegin)
  │
  ├─► IRenderTarget::beginDraw()
  ├─► IRenderTarget::clear(white)
  │
  ├─► 遍历 WidgetTree:
  │     RenderFactory::getRenderer(type)
  │       └─► IWidgetRenderer::paint(rt, node, state)
  │
  ├─► IRenderTarget::endDraw()
  │
  └─► EventBus::publish(PaintEnd)
```

### 4.4 输入事件流程

```
WM_LBUTTONDOWN → EngineFacade::onMouseDown(x,y)
  │
  ▼
IInputDispatcher::dispatchMouseDown(x,y,0)
  │
  ├─► 遍历 WidgetTree (z-order 逆序):
  │     RenderFactory::getRenderer(type)
  │       └─► IWidgetRenderer::hitTest(node, x, y)
  │         ├─► hit → 聚焦该 widget
  │         │   └─► EventBus::publish(FocusChanged, sid, nodeId)
  │         └─► miss → 继续下一个
  │
  └─► EventBus::publish(InputEvent, sid, nodeId, MouseDown{x,y})
```

---

## 5. 文件结构（重构后）

```
include/jui/
  core/
    geometry.h           (M01, ~40行) Rect/Size
    widget_types.h       (M02, ~35行) WidgetType + Children
    value.h              (M00, ~123行) JValue (已有)
    iwidget_tree.h       (I01, ~50行) IWidgetNode + IWidgetTree
    idata_model.h        (I02, ~40行) IDataModel
    widget_node.h        (M06, ~150行) WidgetNode 实现
    widget_tree.h        (M05, ~130行) WidgetTree 实现
    data_model_impl.h    (M07, ~110行) DataModel 实现
    surface.h            (M08, ~80行) Surface 容器
    a2ui_parser.h        (M36, ~200行) JSON→Widget解析
    engine_facade.h      (M38, ~180行) JUIEngine 外观
  bus/
    ievent_bus.h         (I00, ~60行) EventBus 接口
    event_types.h        (M04, ~80行) EventType + Event
    event_bus.h          (M03, ~120行) EventBus 实现
    service_locator.h    (M39, ~80行) DI 容器
  render/
    irender_target.h     (I03, ~35行) 渲染目标接口
    iwidget_state.h      (I06, ~25行) WidgetState 接口
    d2d_context.h        (M22, ~120行) D2D RT/DWrite
    state_base.h         (M23, ~40行) WidgetState 基类
    text_state.h         (M24, ~30行)
    button_state.h       (M25, ~50行)
    textfield_state.h    (M26, ~130行)
    checkbox_state.h     (M27, ~55行)
    toggle_state.h       (M28, ~35行)
    picker_state.h       (M29, ~60行)
    slider_state.h       (M30, ~50行)
    list_state.h         (M31, ~80行)
    grid_state.h         (M32, ~90行)
    tabs_state.h         (M33, ~55行)
    text_renderer.h      (M09, ~120行)
    button_renderer.h    (M10, ~100行)
    textfield_renderer.h (M11, ~220行)
    checkbox_renderer.h  (M12, ~90行)
    toggle_renderer.h    (M13, ~70行)
    picker_renderer.h    (M14, ~130行)
    slider_renderer.h    (M15, ~120行)
    list_renderer.h      (M16, ~180行)
    grid_renderer.h      (M17, ~200行)
    tabs_renderer.h      (M18, ~100行)
    container_renderer.h (M19, ~80行)
    image_renderer.h     (M20, ~70行)
    render_factory.h     (M21, ~80行)
    rendering_loop.h     (M37, ~100行)
  layout/
    ilayout_engine.h     (I04, ~30行)
    layout_engine.h      (M34, ~240行)
  input/
    iinput_dispatcher.h  (I05, ~30行)
    input_dispatcher.h   (M35, ~160行)
  inspector/
    inspector.h          (已有)
  test/
    (已有)
  jui.h                  (聚合头文件)
```

---

## 6. 实施路径

### Phase 1：基础设施（无破坏性变更）
1. `geometry.h` — Rect/Size 提取
2. `widget_types.h` — WidgetType + Children
3. `bus/ievent_bus.h` + `bus/event_types.h` + `bus/event_bus.h`
4. `bus/service_locator.h`

### Phase 2：接口定义（纯虚类）
5. `core/iwidget_tree.h`
6. `core/idata_model.h`
7. `render/irender_target.h`
8. `layout/ilayout_engine.h`
9. `input/iinput_dispatcher.h`
10. `render/iwidget_state.h`

### Phase 3：实现层（header-only）
11. 状态类拆分（M23-M33，每个 ≤ 130行）
12. 渲染器拆分（M09-M20，每个 ≤ 220行）
13. 核心服务（M05-M08）
14. 布局/输入/解析（M34-M37）

### Phase 4：组装
15. `render/render_factory.h` — 注册表
16. `core/engine_facade.h` — JUIEngine 重构为纯外观
17. 更新 `jui.h` 聚合头文件

### Phase 5：清理
18. 删除旧文件（`d2d_renderer.cpp/h`, `render_widget.cpp/h`, `widget_state.h`）
19. 更新 CMakeLists.txt
20. 运行全量测试确保 441 测试通过
