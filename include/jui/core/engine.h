#pragma once
#include "surface.h"
#include "jui/render/d2d_renderer.h"
#include "jui/layout/layout.h"
#include <string>
#include <map>
#include <memory>
#include <functional>

namespace jui {

// 前向声明（不暴露 nlohmann/json）
struct EngineImpl;

// ============================================================
// JUIEngine — 引擎主入口
// 负责解析 A2UI JSON 消息、管理 Surface、协调渲染
// ============================================================

class JUIEngine {
    friend struct EngineImpl;
public:
    JUIEngine();
    ~JUIEngine();

    // 初始化（绑定窗口）
    bool initialize(HWND hwnd);

    // 处理 A2UI JSON 消息（单条）
    void processMessage(const std::string& jsonStr);

    // 处理 JSONL 流（多条消息，换行分隔）
    void processMessageStream(const std::string& jsonlStream);

    // 渲染当前帧
    void render();

    // 窗口消息处理
    void onSize(int w, int h);
    void onMouseMove(int x, int y);
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);
    void onCharInput(uint32_t ch);
    void onKeyDown(int vk);
    void onKeyUp(int vk);
    void onMouseWheel(float delta);
    void onIMEStart();
    void onIMEComposition(const std::string& str);
    void onIMEEnd(const std::string& result);

    // Action 回调（用户交互 → 服务端）— 使用 JSON 字符串避免暴露依赖
    using ActionCallback = std::function<void(const std::string& surfaceId,
                                               const std::string& action,
                                               const std::string& sourceComponentId,
                                               const std::string& contextJson)>;
    void setActionCallback(ActionCallback cb) {
        actionCallback_ = cb;
        // 同步到 D2DRenderer（适配 JValue → string）
        renderer_.setActionCallback([cb](const std::string& s, const std::string& a,
                                          const std::string& c, const JValue& v) {
            std::string ctx;
            if (v.isNumber()) ctx = std::to_string(static_cast<int>(v.asNumber()));
            else if (v.isString()) ctx = v.asString();
            cb(s, a, c, ctx);
        });
    }

    // Surface 管理
    SurfacePtr getSurface(const std::string& id);
    SurfacePtr createSurface(const std::string& id);

    // 从 JSON 文件加载 UI
    bool loadFromFile(const std::string& filepath);

    // 获取渲染器
    D2DRenderer& renderer() { return renderer_; }

private:
    // 内部实现（隐藏 nlohmann/json 依赖）
    std::unique_ptr<EngineImpl> impl_;
    std::map<std::string, SurfacePtr> surfaces_;
    D2DRenderer renderer_;
    LayoutEngine layoutEngine_;
    ActionCallback actionCallback_;
    std::string currentSurfaceId_;
};

} // namespace jui
