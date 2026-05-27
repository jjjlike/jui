#include "jui/render/d2d_renderer.h"
#include <algorithm>

namespace jui {

// ============================================================
// D2DRenderer 实现
// ============================================================

D2DRenderer::D2DRenderer() {}
D2DRenderer::~D2DRenderer() { discardDeviceResources(); }

bool D2DRenderer::initialize(HWND hwnd) {
    hwnd_ = hwnd;

    // 创建 D2D 工厂
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    IID_PPV_ARGS(&d2dFactory_));
    if (FAILED(hr)) return false;

    // 创建 DWrite 工厂
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf()));
    if (FAILED(hr)) return false;

    initialized_ = true;
    return createDeviceResources();
}

bool D2DRenderer::createDeviceResources() {
    if (!d2dFactory_ || !hwnd_) return false;

    RECT rc;
    GetClientRect(hwnd_, &rc);
    width_ = rc.right - rc.left;
    height_ = rc.bottom - rc.top;

    auto size = D2D1::SizeU(width_, height_);
    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        &renderTarget_);
    if (FAILED(hr)) return false;

    // 创建画刷
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &blackBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &grayBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.5f, 0.9f), &blueBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.94f, 0.94f, 0.94f), &lightGrayBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &borderBrush_);

    return true;
}

void D2DRenderer::discardDeviceResources() {
    whiteBrush_.Reset();
    blackBrush_.Reset();
    grayBrush_.Reset();
    blueBrush_.Reset();
    lightGrayBrush_.Reset();
    borderBrush_.Reset();
    renderTarget_.Reset();
}

D2D1_COLOR_F D2DRenderer::parseColor(const std::string& hex, float alpha) {
    D2D1_COLOR_F color = {0, 0, 0, alpha};
    if (hex.empty()) return color;
    std::string h = hex;
    if (h[0] == '#') h = h.substr(1);
    if (h.length() == 6) {
        uint32_t val = 0;
        try { val = static_cast<uint32_t>(std::stoul(h, nullptr, 16)); } catch (...) { return color; }
        color.r = ((val >> 16) & 0xFF) / 255.0f;
        color.g = ((val >> 8) & 0xFF) / 255.0f;
        color.b = (val & 0xFF) / 255.0f;
    }
    return color;
}

void D2DRenderer::setSurface(SurfacePtr surface) {
    surface_ = surface;
    refreshRenderWidgets();
}

RenderWidgetPtr D2DRenderer::createRenderTree(WidgetPtr widget) {
    if (!widget) return nullptr;
    auto rw = RenderWidgetFactory::create(widget);
    if (!rw) return nullptr;
    renderWidgets_[widget->id()] = rw;

    // 递归创建子控件的渲染器
    const auto& children = widget->children();
    if (children.mode == Children::Explicit) {
        for (auto& childId : children.explicitList) {
            if (surface_) {
                auto child = surface_->getWidget(childId);
                if (child) createRenderTree(child);
            }
        }
    }
    return rw;
}

void D2DRenderer::refreshRenderWidgets() {
    renderWidgets_.clear();
    focusedWidget_.reset();
    if (!surface_) return;

    auto root = surface_->rootWidget();
    if (root) createRenderTree(root);
}

void D2DRenderer::render() {
    if (!renderTarget_ || !initialized_) return;

    renderTarget_->BeginDraw();
    renderTarget_->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // 遍历所有渲染控件：先同步布局边界，再绘制
    for (auto& [id, rw] : renderWidgets_) {
        if (rw && rw->widget()->visible()) {
            // 将 Widget 的布局结果同步到 RenderWidget
            rw->setBounds(rw->widget()->layoutBounds());
            rw->paint(renderTarget_.Get(), dwriteFactory_.Get());
        }
    }

    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        discardDeviceResources();
        createDeviceResources();
    }
    needsRedraw_ = false;
}

// ============================================================
// 输入事件处理
// ============================================================

void D2DRenderer::onSize(int w, int h) {
    width_ = w;
    height_ = h;
    if (renderTarget_) {
        renderTarget_->Resize(D2D1::SizeU(w, h));
    }
    needsRedraw_ = true;
}

void D2DRenderer::onMouseMove(int x, int y) {
    for (auto& [id, rw] : renderWidgets_) {
        rw->onMouseMove(static_cast<float>(x), static_cast<float>(y));
    }
}

void D2DRenderer::onMouseDown(int x, int y, int button) {
    // 先取消之前的焦点
    if (focusedWidget_) {
        focusedWidget_->setFocused(false);
    }
    focusedWidget_.reset();

    // 命中测试（倒序遍历，上层控件的 renderer 后创建）
    for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
        auto& rw = it->second;
        if (rw->hitTest(static_cast<float>(x), static_cast<float>(y))) {
            rw->onMouseDown(static_cast<float>(x), static_cast<float>(y));
            if (rw->canFocus()) {
                rw->setFocused(true);
                focusedWidget_ = rw;
            }
            break;
        }
    }
    needsRedraw_ = true;
}

void D2DRenderer::onMouseUp(int x, int y, int button) {
    for (auto& [id, rw] : renderWidgets_) {
        rw->onMouseUp(static_cast<float>(x), static_cast<float>(y));
    }
    needsRedraw_ = true;
}

void D2DRenderer::onCharInput(uint32_t ch) {
    if (focusedWidget_) {
        focusedWidget_->onChar(ch);
        needsRedraw_ = true;
    }
}

void D2DRenderer::onKeyDown(int vk) {
    if (focusedWidget_) {
        focusedWidget_->onKeyDown(vk);
        needsRedraw_ = true;
    }
}

void D2DRenderer::onKeyUp(int vk) {
    if (focusedWidget_) {
        focusedWidget_->onKeyUp(vk);
    }
}

} // namespace jui
