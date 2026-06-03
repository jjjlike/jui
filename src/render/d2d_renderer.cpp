#include "jui/render/d2d_renderer.h"
#include "jui/render/iwidget_renderer.h"
#include "jui/render/d2d_adapter.h"
#include "jui/test/debug_logger.h"
#include <algorithm>
#include <chrono>

namespace jui {

D2DRenderer::D2DRenderer() {
    lastFrameTime_ = std::chrono::steady_clock::now();
}

D2DRenderer::~D2DRenderer() { discardDeviceResources(); }

bool D2DRenderer::initialize(HWND hwnd) {
    hwnd_ = hwnd;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf()));
    if (FAILED(hr)) return false;

    initialized_ = true;
    return createDeviceResources();
}

bool D2DRenderer::createDeviceResources() {
    if (!d2dFactory_ || !hwnd_) return false;
    RECT rc; GetClientRect(hwnd_, &rc);
    width_ = rc.right - rc.left; height_ = rc.bottom - rc.top;

    auto size = D2D1::SizeU(width_, height_);
    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd_, size), &renderTarget_);
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
    D2D1_COLOR_F c = {0,0,0,alpha};
    if (hex.empty() || hex.length() < 6) return c;
    std::string h = (hex[0]=='#') ? hex.substr(1) : hex;
    try {
        uint32_t v = static_cast<uint32_t>(std::stoul(h.substr(0,6), nullptr, 16));
        c.r=((v>>16)&0xFF)/255.0f; c.g=((v>>8)&0xFF)/255.0f; c.b=(v&0xFF)/255.0f;
    } catch(...) {}
    return c;
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
// render — 帧更新 + 绘制
// ============================================================

void D2DRenderer::render() {
    if (!initialized_) return;
    if (!renderTarget_) {
        // 渲染目标不存在——可能在 WM_CREATE 时窗口尺寸为 0×0 导致创建失败
        // onSize 会在 WM_SIZE 时用有效尺寸重新创建
        JUI_WARN_LOG("", "Render", "render() skipped: renderTarget_ is null (width=%d height=%d)", width_, height_);
        return;
    }

    // delta time
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;
    if (dt > 0.1f) dt = 0.016f;

    // 更新光标闪烁 (TextFieldWidgetState::updateCaret)
    if (focusedWidget_ && focusedWidget_->state()) {
        auto* st = focusedWidget_->state();
        auto* tfs = dynamic_cast<TextFieldWidgetState*>(st);
        if (tfs) tfs->updateCaret(dt);
    }

    renderTarget_->BeginDraw();
    renderTarget_->Clear(D2D1::ColorF(D2D1::ColorF::White));

    for (auto& [id, rw] : renderWidgets_) {
        if (rw && rw->widget()->visible()) {
            rw->setBounds(rw->widget()->layoutBounds());

            // 同步 viewport 给 List 和 Grid（虚拟滚动依赖）
            if (rw->widget()->type() == WidgetType::List) {
                auto* ls = dynamic_cast<ListWidgetState*>(rw->state());
                if (ls) ls->setViewportSize(rw->bounds().w, rw->bounds().h);
            }
            if (rw->widget()->type() == WidgetType::Grid) {
                auto* gs = dynamic_cast<GridWidgetState*>(rw->state());
                if (gs) gs->setViewportSize(rw->bounds().w, rw->bounds().h);
            }

            // 调试日志：Paint 阶段 bounds
            JUI_DEBUG_LOG_IF(id, "Paint", jui::test::DebugLogger::instance().isEnabled(),
                "PAINT[Screen] id=%s type=%d bounds=(%.0f,%.0f)-(%.0fx%.0f) visible=1",
                id.c_str(), static_cast<int>(rw->widget()->type()),
                rw->bounds().x, rw->bounds().y, rw->bounds().w, rw->bounds().h);

            rw->paint(renderTarget_.Get(), dwriteFactory_.Get());

            // ──── 自定义渲染器叠加层（如 TestRenderer）────
            if (customRenderer_) {
                auto* cr = customRenderer_->get(rw->widget()->type());
                if (cr) {
                    // 提取文本和状态
                    std::string text;
                    auto tv = rw->widget()->property("text");
                    if (tv.isString()) text = tv.asString();
                    bool enabled = rw->widget()->enabled();
                    bool hovered = rw->state() ? rw->state()->hovered() : false;
                    bool pressed = false;
                    if (auto* bs = dynamic_cast<ButtonWidgetState*>(rw->state()))
                        pressed = bs->visualState() == ButtonWidgetState::VisualState::Pressed;

                    // 创建临时 IRenderTarget 适配器
                    render::D2DRenderTargetAdapter adapter(renderTarget_.Get(), dwriteFactory_.Get());
                    cr->paint(adapter, rw->bounds(), text, enabled, hovered, pressed, 0);
                }
            }
        }
    }

    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) { discardDeviceResources(); createDeviceResources(); }
    needsRedraw_ = false;
}

// ============================================================
// 输入事件
// ============================================================

void D2DRenderer::onSize(int w, int h) {
    width_ = w; height_ = h;
    if (renderTarget_) {
        renderTarget_->Resize(D2D1::SizeU(w, h));
    } else {
        // 渲染目标首次创建失败（如 WM_CREATE 时窗口尺寸为 0×0），
        // 在 WM_SIZE 取得有效尺寸后重新创建
        if (w > 0 && h > 0) {
            createDeviceResources();
        }
    }
    needsRedraw_ = true;
}

void D2DRenderer::onMouseMove(int x, int y) {
    float fx = static_cast<float>(x), fy = static_cast<float>(y);

    // List/Grid 滚动条拖拽中
    for (auto& [id, rw] : renderWidgets_) {
        if (rw->widget()->type() == WidgetType::List) {
            auto* ls = dynamic_cast<ListWidgetState*>(rw->state());
            if (ls && ls->isScrolling()) {
                float sbH = rw->bounds().h * rw->bounds().h / (ls->itemCount() * ls->itemHeight());
                sbH = std::max(20.0f, sbH);
                float ratio = (fy - rw->bounds().y - sbH/2) / (rw->bounds().h - sbH);
                ratio = std::clamp(ratio, 0.0f, 1.0f);
                ls->setScrollOffset(ratio * ls->maxScroll());
                needsRedraw_ = true;
                return;
            }
        }
    }
    // Slider 拖拽中：更新临时值
    for (auto& [id, rw] : renderWidgets_) {
        auto* ss = dynamic_cast<SliderWidgetState*>(rw->state());
        if (ss && ss->isDragging()) {
            float ratio = std::clamp((fx - rw->bounds().x - 8) / (rw->bounds().w - 16), 0.0f, 1.0f);
            float val = ss->minValue() + ratio * (ss->maxValue() - ss->minValue());
            ss->setTempValue(val);
            needsRedraw_ = true;
            return;
        }
    }

    // 清除所有 hover
    for (auto& [id, rw] : renderWidgets_)
        if (rw->state()) rw->state()->setHovered(false);

    // 命中最内层可聚焦控件
    for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
        if (it->second->hitTest(fx, fy) && it->second->canFocus()) {
            if (it->second->state()) it->second->state()->setHovered(true);
            break;
        }
    }
}

void D2DRenderer::onMouseDown(int x, int y, int button) {
    float fx = static_cast<float>(x), fy = static_cast<float>(y);

    if (focusedWidget_ && focusedWidget_->state()) {
        focusedWidget_->state()->setFocused(false);
    }
    focusedWidget_.reset();

    // 穿透非聚焦容器找可聚焦控件
    for (auto it = renderWidgets_.rbegin(); it != renderWidgets_.rend(); ++it) {
        auto& rw = it->second;
        if (!rw->hitTest(fx, fy)) continue;

        // 按钮状态
        if (rw->widget()->type() == WidgetType::Button) {
            auto* bs = dynamic_cast<ButtonWidgetState*>(rw->state());
            if (bs) bs->press();
        }

        // 编辑框状态
        if (rw->widget()->type() == WidgetType::TextField) {
            auto* tfs = dynamic_cast<TextFieldWidgetState*>(rw->state());
            if (tfs) tfs->mouseDown((GetKeyState(VK_CONTROL) & 0x8000) != 0);
        }

        // 复选框状态
        if (rw->widget()->type() == WidgetType::CheckBox) {
            auto* cks = dynamic_cast<CheckBoxWidgetState*>(rw->state());
            if (cks) cks->toggle();
        }
        // 开关状态
        if (rw->widget()->type() == WidgetType::Toggle) {
            auto* ts = dynamic_cast<ToggleWidgetState*>(rw->state());
            if (ts) ts->toggle();
        }
        // 下拉选择器
        if (rw->widget()->type() == WidgetType::ChoicePicker) {
            auto* cp = dynamic_cast<ChoicePickerWidgetState*>(rw->state());
            if (cp) cp->toggle(); // 简单的开关逻辑
        }
        // 滑动条拖拽
        if (rw->widget()->type() == WidgetType::Slider) {
            auto* ss = dynamic_cast<SliderWidgetState*>(rw->state());
            if (ss) { ss->setDragging(true); SetCapture(hwnd_); }
        }
        // Tabs 切换
        if (rw->widget()->type() == WidgetType::Tabs) {
            auto* ts = dynamic_cast<TabsWidgetState*>(rw->state());
            if (ts) {
                float relY = fy - rw->bounds().y;
                if (relY <= ts->headerHeight()) {
                    int idx = ts->hitTabHeader(fx - rw->bounds().x, rw->bounds().w);
                    if (idx >= 0) {
                        ts->setActiveIndex(idx);
                        if (actionCallback_ && idx < ts->tabCount()) {
                            std::string tabId = ts->tab(idx).componentId;
                            actionCallback_(surface_?surface_->id():"", "tab_switch", tabId, JValue());
                            needsRedraw_ = true;
                            return;
                        }
                    }
                }
            }
        }
        // List 滚动条点击/拖拽
        if (rw->widget()->type() == WidgetType::List) {
            auto* ls = dynamic_cast<ListWidgetState*>(rw->state());
            if (ls && fx > rw->bounds().x + rw->bounds().w - 10) {
                // 点击在滚动条区域：跳转到对应位置并启动拖拽
                float sbH = rw->bounds().h * rw->bounds().h / (ls->itemCount() * ls->itemHeight());
                sbH = std::max(20.0f, sbH);
                float ratio = (fy - rw->bounds().y - sbH/2) / (rw->bounds().h - sbH);
                ratio = std::clamp(ratio, 0.0f, 1.0f);
                ls->setScrollOffset(ratio * ls->maxScroll());
                ls->setScrolling(true); // 标记拖拽中，使 onMouseMove 能继续更新
                SetCapture(hwnd_);
                needsRedraw_ = true;
                return; // 不设焦点，防止干扰
            } else if (ls) {
                // 点击在列表项区域：选中该项
                auto* lrw = dynamic_cast<ListRenderWidget*>(rw.get());
                if (lrw) lrw->onClick(fx, fy);
            }
        }

        if (rw->canFocus()) {
            if (rw->state()) rw->state()->setFocused(true);
            focusedWidget_ = rw;
            break;
        }
    }
    needsRedraw_ = true;
}

void D2DRenderer::onMouseUp(int x, int y, int button) {
    float fx = static_cast<float>(x), fy = static_cast<float>(y);
    ReleaseCapture();

    for (auto& [id, rw] : renderWidgets_) {
        // List 滚动结束
        auto* ls = dynamic_cast<ListWidgetState*>(rw->state());
        if (ls) ls->setScrolling(false);
        // Slider 拖拽提交
        auto* ss = dynamic_cast<SliderWidgetState*>(rw->state());
        if (ss && ss->isDragging()) {
            ss->setDragging(false); // commitTemp inside
            ReleaseCapture();
            // 触发回调通知值变化
            if (actionCallback_) {
                std::string src = rw->widget()->id();
                float val = ss->value();
                JValue ctx = JValue(static_cast<double>(val));
                actionCallback_(surface_ ? surface_->id() : "", "slider_change", src, ctx);
            }
            needsRedraw_ = true;
        }

        auto* bs = dynamic_cast<ButtonWidgetState*>(rw->state());
        if (bs) {
            bool wasPressed = (bs->visualState() == ButtonWidgetState::VisualState::Pressed);
            bs->release();
            if (wasPressed && rw->hitTest(fx, fy) && actionCallback_) {
                std::string act = rw->widget()->action();
                std::string src = rw->widget()->id();
                actionCallback_(surface_ ? surface_->id() : "", act, src, JValue());
            }
        }
    }
    needsRedraw_ = true;
}

void D2DRenderer::onCharInput(uint32_t ch) {
    if (!focusedWidget_) return;
    auto* tf = dynamic_cast<TextFieldRenderWidget*>(focusedWidget_.get());
    if (tf) {
        tf->onChar(ch);
        needsRedraw_ = true;
    }
}

void D2DRenderer::onKeyDown(int vk) {
    if (!focusedWidget_) return;

    // Ctrl+C/V/X/A 剪贴板
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrl) {
        if (vk == 'C' && focusedWidget_->canCopy()) {
            copyToClipboard(focusedWidget_->copySelection()); return;
        }
        if (vk == 'V' && focusedWidget_->canPaste()) {
            focusedWidget_->pasteText(pasteFromClipboard());
            needsRedraw_ = true; return;
        }
        if (vk == 'X' && focusedWidget_->canCopy()) {
            copyToClipboard(focusedWidget_->copySelection());
            focusedWidget_->cutSelection();
            needsRedraw_ = true; return;
        }
        if (vk == 'A') { focusedWidget_->selectAll(); needsRedraw_ = true; return; }
    }

    auto* tf = dynamic_cast<TextFieldRenderWidget*>(focusedWidget_.get());
    if (tf) {
        tf->onKeyDown(vk);
        needsRedraw_ = true;
    }
}

void D2DRenderer::onKeyUp(int vk) {
    (void)vk;
}

// ---- 鼠标滚轮 ----
void D2DRenderer::onMouseWheel(float delta) {
    for (auto& [id, rw] : renderWidgets_) {
        if (rw->widget()->type() == WidgetType::List) {
            auto* lrw = dynamic_cast<ListRenderWidget*>(rw.get());
            if (lrw) lrw->onScroll(delta);
        }
        if (rw->widget()->type() == WidgetType::Grid) {
            auto* grw = dynamic_cast<GridRenderWidget*>(rw.get());
            if (grw) grw->onScrollY(delta);
        }
    }
    needsRedraw_ = true;
}

// ---- IME 中文输入 ----
void D2DRenderer::onIMEStart() {
    if (!focusedWidget_) return;
    auto* tf = dynamic_cast<TextFieldRenderWidget*>(focusedWidget_.get());
    if (tf) { tf->imeStart(); needsRedraw_ = true; }
}

void D2DRenderer::onIMEComposition(const std::string& str) {
    if (!focusedWidget_) return;
    auto* tf = dynamic_cast<TextFieldRenderWidget*>(focusedWidget_.get());
    if (tf) { tf->imeComposition(str); needsRedraw_ = true; }
}

void D2DRenderer::onIMEEnd(const std::string& result) {
    if (!focusedWidget_) return;
    auto* tf = dynamic_cast<TextFieldRenderWidget*>(focusedWidget_.get());
    if (tf) { tf->imeEnd(result); needsRedraw_ = true; }
}

// ============================================================
// 剪贴板
// ============================================================
void D2DRenderer::copyToClipboard(const std::string& text) {
    if (text.empty() || !OpenClipboard(hwnd_)) return;
    EmptyClipboard();
    std::wstring w(text.begin(), text.end());
    size_t size = (w.length() + 1) * sizeof(wchar_t);
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, size);
    if (h) { memcpy(GlobalLock(h), w.c_str(), size); GlobalUnlock(h); SetClipboardData(CF_UNICODETEXT, h); }
    CloseClipboard();
}

std::string D2DRenderer::pasteFromClipboard() {
    if (!OpenClipboard(hwnd_)) return "";
    std::string r;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        wchar_t* ws = static_cast<wchar_t*>(GlobalLock(h));
        if (ws) {
            int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) { std::vector<char> buf(len); WideCharToMultiByte(CP_UTF8, 0, ws, -1, buf.data(), len, nullptr, nullptr); r = buf.data(); }
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return r;
}

} // namespace jui
