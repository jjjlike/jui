#include "jui/render/d2d_renderer.h"
#include <algorithm>
#include <chrono>

namespace jui {

// ============================================================
// D2DRenderer 实现
// ============================================================

D2DRenderer::D2DRenderer() {
    lastFrameTime_ = std::chrono::steady_clock::now();
}

D2DRenderer::~D2DRenderer() { discardDeviceResources(); }

bool D2DRenderer::initialize(HWND hwnd) {
    hwnd_ = hwnd;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    IID_PPV_ARGS(&d2dFactory_));
    if (FAILED(hr)) return false;

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

    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &blackBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &grayBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.5f, 0.9f), &blueBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.94f, 0.94f, 0.94f), &lightGrayBrush_);
    renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &borderBrush_);

    return true;
}

void D2DRenderer::discardDeviceResources() {
    whiteBrush_.Reset(); blackBrush_.Reset(); grayBrush_.Reset();
    blueBrush_.Reset(); lightGrayBrush_.Reset(); borderBrush_.Reset();
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

// ============================================================
// render — 绘制+更新所有控件
// ============================================================

void D2DRenderer::render() {
    if (!renderTarget_ || !initialized_) return;

    // 计算 delta time（用于光标闪烁等动画）
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;
    // 限制最大 dt 防止跳帧异常
    if (dt > 0.1f) dt = 0.016f;

    // 更新所有控件的动画状态（光标闪烁等）
    for (auto& [id, rw] : renderWidgets_) {
        if (rw) rw->update(dt);
    }

    renderTarget_->BeginDraw();
    renderTarget_->Clear(D2D1::ColorF(D2D1::ColorF::White));

    for (auto& [id, rw] : renderWidgets_) {
        if (rw && rw->widget()->visible()) {
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
// 输入事件 — 含 hover、IME、剪贴板、右键菜单
// ============================================================

void D2DRenderer::onSize(int w, int h) {
    width_ = w; height_ = h;
    if (renderTarget_) renderTarget_->Resize(D2D1::SizeU(w, h));
    needsRedraw_ = true;
}

void D2DRenderer::onMouseMove(int x, int y) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    // 清除所有 hover 状态
    for (auto& [id, rw] : renderWidgets_) rw->setHovered(false);

    // 命中测试设置 hover
    for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
        if (it->second->hitTest(fx, fy)) {
            it->second->setHovered(true);
            break;
        }
    }

    for (auto& [id, rw] : renderWidgets_) rw->onMouseMove(fx, fy);

    if (!hoveredWidgetId_.empty()) {
        auto it = renderWidgets_.find(hoveredWidgetId_);
        if (it != renderWidgets_.end()) needsRedraw_ = true;
    }
}

void D2DRenderer::onMouseDown(int x, int y, int button) {
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);

    // 右键菜单
    if (button == 1) { // 右键
        for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
            if (it->second->hitTest(fx, fy) && it->second->onContextMenu(fx, fy)) {
                float cx = fx;
                float cy = fy;
                // 通知外部显示上下文菜单
                if (actionCallback_) {
                    JValue ctx = JValue::fromObject(
                        {{"type", JValue("contextmenu")},
                         {"sourceComponentId", JValue(it->second->widget()->id())},
                         {"x", JValue(static_cast<double>(cx))},
                         {"y", JValue(static_cast<double>(cy))}}
                    );
                }
                needsRedraw_ = true;
                return;
            }
        }
    }

    // 左键/中键: 正常处理
    if (focusedWidget_) {
        focusedWidget_->setFocused(false);
    }
    focusedWidget_.reset();

    for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
        auto& rw = it->second;
        if (rw->hitTest(fx, fy)) {
            rw->onMouseDown(fx, fy);
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
        // Ctrl+C / Ctrl+V / Ctrl+X / Ctrl+A 剪贴板操作
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        if (ctrl && vk == 'C') {
            if (focusedWidget_->canCopy()) {
                std::string sel = focusedWidget_->copySelection();
                copyToClipboard(sel);
            }
            return;
        }
        if (ctrl && vk == 'V') {
            if (focusedWidget_->canPaste()) {
                std::string txt = pasteFromClipboard();
                focusedWidget_->pasteText(txt);
            }
            needsRedraw_ = true;
            return;
        }
        if (ctrl && vk == 'X') {
            if (focusedWidget_->canCopy()) {
                std::string sel = focusedWidget_->copySelection();
                copyToClipboard(sel);
                focusedWidget_->cutSelection();
            }
            needsRedraw_ = true;
            return;
        }
        if (ctrl && vk == 'A') {
            focusedWidget_->selectAll();
            needsRedraw_ = true;
            return;
        }

        focusedWidget_->onKeyDown(vk);
        needsRedraw_ = true;
    }
}

void D2DRenderer::onKeyUp(int vk) {
    if (focusedWidget_) {
        focusedWidget_->onKeyUp(vk);
    }
}

// ============================================================
// 剪贴板辅助
// ============================================================

void D2DRenderer::copyToClipboard(const std::string& text) {
    if (text.empty() || !OpenClipboard(hwnd_)) return;
    EmptyClipboard();
    std::wstring wtext(text.begin(), text.end());
    size_t size = (wtext.length() + 1) * sizeof(wchar_t);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hGlobal) {
        memcpy(GlobalLock(hGlobal), wtext.c_str(), size);
        GlobalUnlock(hGlobal);
        SetClipboardData(CF_UNICODETEXT, hGlobal);
    }
    CloseClipboard();
}

std::string D2DRenderer::pasteFromClipboard() {
    if (!OpenClipboard(hwnd_)) return "";
    std::string result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* wstr = static_cast<wchar_t*>(GlobalLock(hData));
        if (wstr) {
            int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                std::vector<char> buf(len);
                WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf.data(), len, nullptr, nullptr);
                result = buf.data();
            }
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

} // namespace jui
