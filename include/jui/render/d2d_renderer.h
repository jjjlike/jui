#pragma once
#include "render_widget.h"
#include "jui/core/surface.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <chrono>
#include <map>
#include <memory>
#include <functional>

namespace jui {

using Microsoft::WRL::ComPtr;

// ============================================================
// D2DRenderer — Direct2D 渲染器
// ============================================================

class D2DRenderer {
public:
    using ActionCallback = std::function<void(const std::string& surfaceId,
                                               const std::string& action,
                                               const std::string& sourceComponentId,
                                               const JValue& context)>;

    D2DRenderer();
    ~D2DRenderer();

    bool initialize(HWND hwnd);
    void setSurface(SurfacePtr surface);
    void render();

    // 窗口消息
    void onSize(int w, int h);
    void onMouseMove(int x, int y);
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);
    void onCharInput(uint32_t ch);
    void onKeyDown(int vk);
    void onKeyUp(int vk);
    void onMouseWheel(float delta);

    // IME 中文输入
    void onIMEStart();
    void onIMEComposition(const std::string& str);
    void onIMEEnd(const std::string& result);

    // 事件回调
    void setActionCallback(ActionCallback cb) { actionCallback_ = std::move(cb); }

    int width() const { return width_; }
    int height() const { return height_; }
    void invalidate() { needsRedraw_ = true; }

    // 颜色解析
    static D2D1_COLOR_F parseColor(const std::string& hex, float alpha = 1.0f);

private:
    void refreshRenderWidgets();
    RenderWidgetPtr createRenderTree(WidgetPtr widget);
    bool createDeviceResources();
    void discardDeviceResources();
    void copyToClipboard(const std::string& text);
    std::string pasteFromClipboard();

    HWND hwnd_ = nullptr;
    int width_ = 0, height_ = 0;

    // D2D 资源
    ComPtr<ID2D1Factory> d2dFactory_;
    ComPtr<ID2D1HwndRenderTarget> renderTarget_;
    ComPtr<IDWriteFactory> dwriteFactory_;
    ComPtr<ID2D1SolidColorBrush> whiteBrush_, blackBrush_, grayBrush_;
    ComPtr<ID2D1SolidColorBrush> blueBrush_, lightGrayBrush_, borderBrush_;

    // Surface & 渲染控件
    SurfacePtr surface_;
    std::map<std::string, RenderWidgetPtr> renderWidgets_;
    RenderWidgetPtr focusedWidget_;
    std::string hoveredWidgetId_;

    // 时间（用于光标闪烁 delta）
    std::chrono::steady_clock::time_point lastFrameTime_;

    // 回调
    ActionCallback actionCallback_;

    bool initialized_ = false;
    bool needsRedraw_ = true;
};

} // namespace jui
