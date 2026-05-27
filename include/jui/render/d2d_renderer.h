#pragma once
#include "render_widget.h"
#include "jui/core/surface.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <map>
#include <memory>
#include <functional>

namespace jui {

// ============================================================
// D2DRenderer — Direct2D 渲染器
// 管理 D2D 资源、窗口消息、渲染循环
// 为每个逻辑控件创建对应的 RenderWidget
// ============================================================

using Microsoft::WRL::ComPtr;

class D2DRenderer {
public:
    using ActionCallback = std::function<void(const std::string& surfaceId,
                                               const std::string& action,
                                               const std::string& sourceComponentId,
                                               const JValue& context)>;

    D2DRenderer();
    ~D2DRenderer();

    // 初始化（需传入 HWND）
    bool initialize(HWND hwnd);

    // 关联 Surface
    void setSurface(SurfacePtr surface);

    // 渲染一帧
    void render();

    // 窗口消息处理
    void onSize(int w, int h);
    void onMouseMove(int x, int y);
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);
    void onCharInput(uint32_t ch);
    void onKeyDown(int vk);
    void onKeyUp(int vk);

    // 事件回调
    void setActionCallback(ActionCallback cb) { actionCallback_ = std::move(cb); }

    // 获取尺寸
    int width() const { return width_; }
    int height() const { return height_; }

    // 脏标记
    void invalidate() { needsRedraw_ = true; }

    // 颜色解析（公开，方便测试和外部使用）
    static D2D1_COLOR_F parseColor(const std::string& hex, float alpha = 1.0f);

private:
    // 刷新渲染控件树（从逻辑控件重建）
    void refreshRenderWidgets();

    // 创建渲染控件并递归子控件
    RenderWidgetPtr createRenderTree(WidgetPtr widget);

    // D2D 资源创建
    bool createDeviceResources();
    void discardDeviceResources();

    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;

    // D2D 资源
    ComPtr<ID2D1Factory> d2dFactory_;
    ComPtr<ID2D1HwndRenderTarget> renderTarget_;
    ComPtr<IDWriteFactory> dwriteFactory_;

    // 常用画刷
    ComPtr<ID2D1SolidColorBrush> whiteBrush_;
    ComPtr<ID2D1SolidColorBrush> blackBrush_;
    ComPtr<ID2D1SolidColorBrush> grayBrush_;
    ComPtr<ID2D1SolidColorBrush> blueBrush_;
    ComPtr<ID2D1SolidColorBrush> lightGrayBrush_;
    ComPtr<ID2D1SolidColorBrush> borderBrush_;

    // Surface 和渲染控件
    SurfacePtr surface_;
    std::map<std::string, RenderWidgetPtr> renderWidgets_;
    RenderWidgetPtr focusedWidget_;

    // 回调
    ActionCallback actionCallback_;

    bool initialized_ = false;
    bool needsRedraw_ = true;
};

} // namespace jui
