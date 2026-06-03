# JUI 黑盒测试数据采集模块 — 优化设计文档

> **版本**：v1.0  
> **日期**：2026-06-03  
> **范围**：JUI 引擎 v0.2.0 → v0.2.1

---

## 1. 模块概述

### 1.1 目标

为 AI 自动化测试提供标准化的 UI 状态快照能力，使测试代码无需依赖渲染管线即可通过 JSON 契约精确校验界面状态。

### 1.2 设计原则

| 原则 | 说明 |
|------|------|
| **零侵入** | 仅新增 `Engine::inspect()` 一个公开方法，不修改现有渲染/布局代码 |
| **按需激活** | 无测试时零开销，数据采集仅在显式调用时触发 |
| **标准契约** | 输出遵循 JSON Schema，AI 可基于 Schema 预编写通用断言 |
| **高性能** | 单次采集 < 1ms（100 控件规模），不拷贝渲染资源 |

---

## 2. 模块架构

```
┌──────────────────────────────────────────────────┐
│                   JUIEngine                       │
│  ┌────────────────────────────────────────────┐  │
│  │  inspect(surfaceId) → JSON string           │  │  ← 唯一公开接口
│  └──────────────┬─────────────────────────────┘  │
│                 │ 读取（不修改）                    │
│  ┌──────────────▼─────────────────────────────┐  │
│  │         UIInspector (新模块)                │  │
│  │  ┌───────────────────────────────────────┐ │  │
│  │  │ WidgetSnapshotCollector                │ │  │
│  │  │  • 遍历 Surface Widget Tree            │ │  │
│  │  │  • 采集 layoutBounds / properties      │ │  │
│  │  │  • 采集 RenderWidget bounds / state    │ │  │
│  │  └───────────────┬───────────────────────┘ │  │
│  │                  │                          │  │
│  │  ┌───────────────▼───────────────────────┐ │  │
│  │  │ SnapshotSerializer                     │ │  │
│  │  │  • 序列化为 JSON (nlohmann/json)       │ │  │
│  │  │  • 遵循 JSON Schema 契约               │ │  │
│  │  └───────────────────────────────────────┘ │  │
│  └────────────────────────────────────────────┘  │
│                                                  │
│  ┌────────────────────────────────────────────┐  │
│  │  现有模块（不受影响）                         │  │
│  │  Surface → Widget → children.explicitList   │  │
│  │  D2DRenderer → renderWidgets_               │  │
│  │  LayoutEngine → measuredSizes_              │  │
│  └────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

### 2.1 数据流

```
AI Test Code                    JUIEngine
    │                              │
    │  inspect("main")             │
    │─────────────────────────────>│
    │                              │─── UIInspector::collect(surface, renderer)
    │                              │     ├── 遍历 surface->widgets()
    │                              │     ├── 读取 widget->layoutBounds()
    │                              │     ├── 读取 widget->children()
    │                              │     ├── 读取 widget->property(key)
    │                              │     └── 读取 renderer.renderWidgets_[id]
    │                              │         ├── rw->bounds()  ← paint区域
    │                              │         └── rw->state()   ← 交互状态
    │                              │
    │                              │─── SnapshotSerializer::toJson(snapshot)
    │                              │     └── 返回 JSON 字符串
    │  {"surfaceId":"main", ...}   │
    │<─────────────────────────────│
    │                              │
    │  AI 解析 JSON                │
    │  对比断言                     │
```

---

## 3. JSON Schema 定义

### 3.1 顶层结构

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "jui-inspector-snapshot-v1",
  "title": "JUI Inspector Snapshot",
  "type": "object",
  "required": ["surfaceId", "timestamp", "viewport", "widgets"],
  "properties": {
    "surfaceId": {
      "type": "string",
      "description": "Surface ID"
    },
    "timestamp": {
      "type": "integer",
      "description": "Unix毫秒时间戳"
    },
    "viewport": {
      "$ref": "#/$defs/rect",
      "description": "视口尺寸（渲染目标尺寸）"
    },
    "widgets": {
      "type": "array",
      "description": "控件快照列表",
      "items": { "$ref": "#/$defs/widgetSnapshot" }
    }
  },
  "$defs": {
    "rect": {
      "type": "object",
      "required": ["x", "y", "w", "h"],
      "properties": {
        "x": { "type": "number" },
        "y": { "type": "number" },
        "w": { "type": "number" },
        "h": { "type": "number" }
      }
    },
    "widgetSnapshot": {
      "type": "object",
      "required": ["id", "type", "visible", "layout", "children"],
      "properties": {
        "id": {
          "type": "string",
          "description": "控件唯一标识"
        },
        "type": {
          "type": "string",
          "enum": ["Text","Button","TextField","CheckBox","Toggle",
                   "ChoicePicker","Slider","List","Grid","Tabs",
                   "Image","Divider","Row","Column","Card"],
          "description": "控件类型"
        },
        "visible": {
          "type": "boolean",
          "description": "可见性"
        },
        "enabled": {
          "type": "boolean",
          "description": "启用状态"
        },
        "layout": {
          "$ref": "#/$defs/rect",
          "description": "Layout 阶段计算的区域（屏幕坐标系）"
        },
        "paint": {
          "$ref": "#/$defs/rect",
          "description": "Paint 阶段使用的区域（屏幕坐标系）"
        },
        "paintMatch": {
          "type": "boolean",
          "description": "Layout 与 Paint 区域是否一致（<1px 容差）"
        },
        "state": {
          "type": "object",
          "description": "控件运行时状态（类型相关）",
          "properties": {
            "text": { "type": "string" },
            "hovered": { "type": "boolean" },
            "pressed": { "type": "boolean" },
            "focused": { "type": "boolean" },
            "checked": { "type": "boolean" },
            "scrollOffset": { "type": "number" },
            "itemCount": { "type": "integer" },
            "selectedIndex": { "type": "integer" },
            "sliderValue": { "type": "number" }
          }
        },
        "children": {
          "type": "array",
          "items": { "type": "string" },
          "description": "子控件 ID 列表（可据此重建控件树）"
        }
      }
    }
  }
}
```

### 3.2 示例输出

```json
{
  "surfaceId": "main",
  "timestamp": 1748937600000,
  "viewport": {"x": 0, "y": 0, "w": 800, "h": 600},
  "widgets": [
    {
      "id": "root",
      "type": "Column",
      "visible": true,
      "enabled": true,
      "layout": {"x": 0, "y": 0, "w": 800, "h": 600},
      "paint": {"x": 0, "y": 0, "w": 800, "h": 600},
      "paintMatch": true,
      "state": {},
      "children": ["txt", "btn"]
    },
    {
      "id": "txt",
      "type": "Text",
      "visible": true,
      "enabled": true,
      "layout": {"x": 0, "y": 0, "w": 800, "h": 24},
      "paint": {"x": 0, "y": 0, "w": 800, "h": 24},
      "paintMatch": true,
      "state": {"text": "Hello, JUI!"},
      "children": []
    },
    {
      "id": "btn",
      "type": "Button",
      "visible": true,
      "enabled": true,
      "layout": {"x": 0, "y": 24, "w": 80, "h": 32},
      "paint": {"x": 0, "y": 24, "w": 80, "h": 32},
      "paintMatch": true,
      "state": {"text": "点我问候", "hovered": false, "pressed": false},
      "children": []
    }
  ]
}
```

### 3.3 AI 断言示例（基于 Schema）

```cpp
// AI 可基于 Schema 预编写以下通用断言，无需了解 JUI 内部实现：

// 1. 验证所有控件在视口内
for (auto& w : snapshot["widgets"]) {
    auto& l = w["layout"];
    assert(l["x"] >= -1 && l["y"] >= -1);
    assert(l["x"] + l["w"] <= viewport_w + 1);
    assert(l["y"] + l["h"] <= viewport_h + 1);
}

// 2. 验证 Layout=Paint 一致性
for (auto& w : snapshot["widgets"]) {
    assert(w["paintMatch"] == true);
}

// 3. 验证控件树完整性
for (auto& w : snapshot["widgets"]) {
    for (auto& child : w["children"]) {
        assert(widgetMap.contains(child));
    }
}

// 4. 验证必定存在的控件
assert(findWidget("btn")["visible"] == true);
assert(findWidget("txt")["state"]["text"] == "Hello, JUI!");
```

---

## 4. 接口设计

### 4.1 公开接口（最小侵入）

```cpp
// engine.h — 唯一新增方法
class JUIEngine {
public:
    // 获取指定 Surface 的 UI 状态快照（JSON 格式）
    //   surfaceId: 目标 Surface ID，空字符串表示当前激活 Surface
    // 返回: JSON 字符串（符合 jui-inspector-snapshot-v1 Schema）
    // 性能: O(n)，n = 控件数量，100控件 < 0.5ms
    std::string inspect(const std::string& surfaceId = "") const;
};
```

### 4.2 内部模块接口

```cpp
// inspector.h
namespace jui::inspector {

struct RectSnapshot { float x, y, w, h; };

struct WidgetSnapshot {
    std::string id;
    std::string type;
    bool visible;
    bool enabled;
    RectSnapshot layout;
    RectSnapshot paint;
    bool paintMatch;
    nlohmann::json state;      // 类型相关状态
    std::vector<std::string> children;
};

struct SurfaceSnapshot {
    std::string surfaceId;
    int64_t timestamp;
    RectSnapshot viewport;
    std::vector<WidgetSnapshot> widgets;
};

class Collector {
public:
    // 从 Surface + D2DRenderer 采集快照
    SurfaceSnapshot collect(const SurfacePtr& surface,
                            const D2DRenderer& renderer) const;
};

class Serializer {
public:
    // 将快照序列化为 JSON 字符串
    static std::string toJson(const SurfaceSnapshot& snapshot);
    // 将 JSON 字符串反序列化（测试端使用）
    static SurfaceSnapshot fromJson(const std::string& json);
};

} // namespace jui::inspector
```

---

## 5. 性能影响评估

### 5.1 运行时开销

| 场景 | 开销 | 说明 |
|------|------|------|
| **未调用 inspect()** | **0** | 无任何额外代码路径执行 |
| **单次 inspect()** | ~0.3ms/100控件 | Widget树遍历 + layoutBounds读取 + 状态读取 |
| **含 RenderWidget 查询** | +0.2ms/100控件 | D2DRenderer::renderWidgets_ 查找 |
| **JSON序列化** | +0.2ms/100控件 | nlohmann/json stringify |
| **总计** | ~0.7ms/100控件 | 低于渲染帧预算的 5%（16ms 帧） |
| **内存** | ~50KB/100控件 | 临时 JSON 字符串，函数返回后释放 |

### 5.2 编译影响

| 项目 | 影响 |
|------|------|
| 新增文件 | 2 个头文件 + 2 个源文件 |
| CMakeLists.txt | +2 行（源文件） |
| 公开头文件变更 | +1 方法声明（engine.h） |
| 现有代码变更 | 0（不修改任何 .cpp） |

### 5.3 与 DebugLogger 的定位差异

| 特性 | DebugLogger | Inspector |
|------|-------------|-----------|
| 目的 | 开发调试，记录内部流程 | AI 测试，结构化数据 |
| 格式 | 文本日志行 | JSON |
| 触发 | 被动（渲染时写入） | 主动（显式调用） |
| 性能 | 每个控件/帧都有开销 | 仅调用时有开销 |
| Schema | 无 | 标准 JSON Schema |

---

## 6. 实施路径

```
Phase 1: 核心模块（本次实施）
  ├── include/jui/inspector/inspector.h      ← 公开头文件
  ├── src/inspector/inspector.cpp             ← 采集器 + 序列化器
  └── engine.h                                ← +inspect() 方法

Phase 2: 测试用例
  ├── tests/test_inspector.cpp                ← 单元测试
  └── tests/test_blackbox_inspector.cpp       ← 黑盒测试

Phase 3: 文档
  └── docs/INSPECTOR.md                       ← 使用文档
```

---

## 7. 目录结构

```
include/jui/
  inspector/
    inspector.h          ← UIInspector 公开接口
  core/
    engine.h             ← +inspect() 声明

src/
  inspector/
    inspector.cpp        ← Collector + Serializer 实现

tests/
  test_inspector.cpp     ← 单元测试（基于 Schema 断言）
  test_blackbox_inspector.cpp  ← 黑盒测试（进程间 JSON 比对）
```
