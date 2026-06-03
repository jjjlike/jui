/**
 * @file inspector.h
 * @brief JUI 黑盒测试数据采集模块 — 零侵入 UI 状态快照
 *
 * 提供标准 JSON Schema 格式的 UI 状态数据，供 AI 自动化测试使用。
 *
 * 用法:
 *   std::string json = engine.inspect("main");
 *   // 返回 JSON 字符串，符合 jui-inspector-snapshot-v1 Schema
 *
 * Schema: docs/INSPECTOR_DESIGN.md
 */
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// 前向声明（避免引入全量依赖）
namespace jui {
    class Surface;
    class D2DRenderer;
    using SurfacePtr = std::shared_ptr<Surface>;
}

namespace jui {
namespace inspector {

// ──────────────────────────────────────────────
// 数据模型
// ──────────────────────────────────────────────

struct RectSnapshot {
    float x = 0, y = 0, w = 0, h = 0;
};

/**
 * @brief 单个控件的运行时快照
 */
struct WidgetSnapshot {
    std::string id;                    ///< 控件唯一标识
    std::string type;                  ///< "Text" / "Button" / "Column" ...
    bool        visible = true;        ///< 可见性
    bool        enabled = true;        ///< 启用状态
    RectSnapshot layout;               ///< Layout 阶段区域（屏幕坐标系）
    RectSnapshot paint;                ///< Paint 阶段区域（屏幕坐标系）
    bool        paintMatch = true;     ///< Layout 与 Paint 一致
    std::string stateJson;             ///< 运行时状态（JSON 子对象）
    std::vector<std::string> children; ///< 子控件 ID 列表
};

/**
 * @brief Surface 的完整运行时快照
 */
struct SurfaceSnapshot {
    std::string surfaceId;
    int64_t     timestamp = 0;
    RectSnapshot viewport;             ///< 渲染目标尺寸
    std::vector<WidgetSnapshot> widgets;
};

// ──────────────────────────────────────────────
// Collector — 数据采集器
// ──────────────────────────────────────────────

class Collector {
public:
    /**
     * @brief 从 Surface + Renderer 采集完整快照
     * @param surface 目标 Surface
     * @param renderer D2D 渲染器（用于获取 RenderWidget bounds/state）
     * @return Surface 快照
     */
    static SurfaceSnapshot collect(
        const SurfacePtr& surface,
        const D2DRenderer& renderer);
};

// ──────────────────────────────────────────────
// Serializer — JSON 序列化/反序列化
// ──────────────────────────────────────────────

class Serializer {
public:
    /**
     * @brief 将快照序列化为 JSON 字符串
     * @param snapshot Surface 快照
     * @return JSON 字符串（符合 jui-inspector-snapshot-v1 Schema）
     */
    static std::string toJson(const SurfaceSnapshot& snapshot);

    /**
     * @brief 从 JSON 字符串反序列化快照（供测试端消费）
     * @param json JSON 字符串
     * @return Surface 快照，解析失败返回空快照（surfaceId 为空）
     */
    static SurfaceSnapshot fromJson(const std::string& json);
};

} // namespace inspector
} // namespace jui
