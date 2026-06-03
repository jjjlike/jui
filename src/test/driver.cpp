/**
 * @file driver.cpp
 * @brief AiTestDriver 实现 — 无头测试环境下驱动UI引擎
 */
#include "jui/test/driver.h"
#include "jui/test/validator.h"
#include <json.hpp>
#include <windows.h>
#include <atomic>
#include <sstream>

namespace jui::test {

using json = nlohmann::json;

// ============================================================
// 内部实现
// ============================================================
struct AiTestDriver::Impl {
    JUIEngine engine;
    std::string activeSurface;
    std::vector<std::string> actionLog;
    std::string lastActionName;
    std::string lastActionSrc;
    HWND hwnd = nullptr;
    bool initialized = false;
    int width = 800, height = 600;

    // 帮助方法：找到控件的中心坐标
    Point widgetCenter(const std::string& id) {
        auto s = engine.getSurface(activeSurface);
        if (!s) return {400, 300};
        auto w = s->getWidget(id);
        if (!w) return {400, 300};
        auto& b = w->layoutBounds();
        return {b.x + b.w / 2, b.y + b.h / 2};
    }
};

// ============================================================
// 构造/析构
// ============================================================
AiTestDriver::AiTestDriver() : impl_(std::make_unique<Impl>()) {}
AiTestDriver::~AiTestDriver() {
    if (impl_->hwnd) DestroyWindow(impl_->hwnd);
}

bool AiTestDriver::initialize() {
    if (impl_->initialized) return true;

    // 创建隐藏窗口用于内部渲染
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"JUI_TestDriver_Window";
    static bool registered = false;
    if (!registered) { RegisterClassExW(&wc); registered = true; }

    impl_->hwnd = CreateWindowExW(0, L"JUI_TestDriver_Window", L"",
        WS_POPUP, 0, 0, impl_->width, impl_->height,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!impl_->hwnd) return false;
    impl_->engine.initialize(impl_->hwnd);
    impl_->initialized = true;
    return true;
}

// ---- 加载 ----
void AiTestDriver::loadUI(const std::string& surfaceId, const std::string& componentsJson) {
    impl_->activeSurface = surfaceId;
    impl_->engine.processMessage(R"({"createSurface":{"surfaceId":")" + surfaceId + R"("}})");
    impl_->engine.processMessage(componentsJson);
    // 找到根组件ID（第一个组件通常是root）
    json j = json::parse(componentsJson);
    std::string rootId = "root";
    if (j.contains("surfaceUpdate") && j["surfaceUpdate"].contains("components")) {
        auto& comps = j["surfaceUpdate"]["components"];
        if (comps.is_array() && !comps.empty()) {
            rootId = comps[0]["id"].get<std::string>();
        }
    }
    impl_->engine.processMessage(R"({"beginRendering":{"surfaceId":")" + surfaceId + R"(","root":")" + rootId + R"("}})");
    impl_->engine.render();
}

void AiTestDriver::loadJSONL(const std::string& jsonl) {
    std::istringstream ss(jsonl);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line[0] != '#') {
            impl_->engine.processMessage(line);
        }
    }
    impl_->engine.render();
}

// ---- 交互 ----
void AiTestDriver::click(const std::string& widgetId) {
    auto pt = impl_->widgetCenter(widgetId);
    impl_->engine.onMouseDown(static_cast<int>(pt.x), static_cast<int>(pt.y), 0);
    impl_->engine.onMouseUp(static_cast<int>(pt.x), static_cast<int>(pt.y), 0);
    impl_->engine.render();
}

void AiTestDriver::mouseDown(const std::string& widgetId) {
    auto pt = impl_->widgetCenter(widgetId);
    impl_->engine.onMouseDown(static_cast<int>(pt.x), static_cast<int>(pt.y), 0);
}

void AiTestDriver::mouseUp(const std::string& widgetId) {
    auto pt = impl_->widgetCenter(widgetId);
    impl_->engine.onMouseUp(static_cast<int>(pt.x), static_cast<int>(pt.y), 0);
    impl_->engine.render();
}

void AiTestDriver::typeText(const std::string& widgetId, const std::string& text) {
    // 先点击获取焦点
    click(widgetId);
    // 逐个字符输入
    for (unsigned char c : text) {
        // 只处理 ASCII，中文用 UTF-8 编码输入
        if (c < 0x80) {
            impl_->engine.onCharInput(static_cast<uint32_t>(c));
        }
    }
    // 对于非ASCII（如中文），需要逐字节通过IME输入
    for (size_t i = 0; i < text.length(); ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c >= 0x80) {
            // UTF-8 多字节序列：找到完整的 code point
            size_t len = 1;
            if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            std::string utf8Char = text.substr(i, len);
            // 使用 IME 模拟输入多字节字符
            impl_->engine.onIMEStart();
            impl_->engine.onIMEEnd(utf8Char);
            i += len;
        } else {
            i += 1;
        }
    }
    impl_->engine.render();
}

void AiTestDriver::pressKey(int vkCode) {
    impl_->engine.onKeyDown(vkCode);
    impl_->engine.onKeyUp(vkCode);
    impl_->engine.render();
}

void AiTestDriver::scroll(const std::string& widgetId, float delta) {
    (void)widgetId;
    impl_->engine.onMouseWheel(delta);
    impl_->engine.render();
}

void AiTestDriver::clickTab(const std::string& tabsId, int index) {
    auto pt = impl_->widgetCenter(tabsId);
    // Tab 头的宽度 = 总宽度 / 标签数量，计算目标 Tab 的 X 位置
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (s) {
        auto w = s->getWidget(tabsId);
        if (w) {
            auto& b = w->layoutBounds();
            float tabW = b.w / 6.0f; // 假设 6 个 Tab
            float tabX = b.x + tabW * index + tabW / 2;
            impl_->engine.onMouseDown(static_cast<int>(tabX), static_cast<int>(b.y + 16), 0);
        }
    }
    impl_->engine.render();
}

void AiTestDriver::dragSlider(const std::string& sliderId, float targetValue) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return;
    auto w = s->getWidget(sliderId);
    if (!w) return;
    auto& b = w->layoutBounds();
    // 计算目标值对应的X坐标：ratio = (value - min) / (max - min)
    float ratio = std::clamp((targetValue - 0) / (100 - 0), 0.0f, 1.0f); // 默认min=0, max=100
    float targetX = b.x + 8 + (b.w - 16) * ratio;
    float midY = b.y + b.h / 2;
    // 完整拖拽序列：按下 → 移动 → 释放
    impl_->engine.onMouseDown(static_cast<int>(targetX), static_cast<int>(midY), 0);
    impl_->engine.render();
    impl_->engine.onMouseMove(static_cast<int>(targetX), static_cast<int>(midY));
    impl_->engine.render();
    impl_->engine.onMouseUp(static_cast<int>(targetX), static_cast<int>(midY), 0);
    impl_->engine.render();
}

void AiTestDriver::injectEvent(const SimulatedEvent& event) {
    switch (event.type) {
    case InputEventType::MouseDown:
        impl_->engine.onMouseDown(static_cast<int>(event.x), static_cast<int>(event.y), event.button);
        break;
    case InputEventType::MouseUp:
        impl_->engine.onMouseUp(static_cast<int>(event.x), static_cast<int>(event.y), event.button);
        break;
    case InputEventType::MouseMove:
        impl_->engine.onMouseMove(static_cast<int>(event.x), static_cast<int>(event.y));
        break;
    case InputEventType::MouseWheel:
        impl_->engine.onMouseWheel(event.wheelDelta);
        break;
    case InputEventType::KeyDown:
        impl_->engine.onKeyDown(event.keyCode);
        break;
    case InputEventType::KeyUp:
        impl_->engine.onKeyUp(event.keyCode);
        break;
    case InputEventType::CharInput:
        impl_->engine.onCharInput(event.charCode);
        break;
    default: break;
    }
    impl_->engine.render();
}

// ---- 状态读取 ----
std::string AiTestDriver::widgetText(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return "";
    auto w = s->getWidget(widgetId);
    if (!w) return "";
    auto tv = w->property("text");
    return tv.isString() ? tv.asString() : "";
}

bool AiTestDriver::widgetVisible(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return false;
    auto w = s->getWidget(widgetId);
    return w ? w->visible() : false;
}

bool AiTestDriver::widgetEnabled(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return false;
    auto w = s->getWidget(widgetId);
    return w ? w->enabled() : false;
}

bool AiTestDriver::widgetChecked(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return false;
    auto w = s->getWidget(widgetId);
    if (!w) return false;
    auto v = w->property("value");
    return v.isBoolean() ? v.asBool() : false;
}

bool AiTestDriver::widgetFocused(const std::string& widgetId) {
    // 通过检查 widget是否有焦点标记
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return false;
    auto w = s->getWidget(widgetId);
    if (!w) return false;
    return w->property("_focused").isBoolean() && w->property("_focused").asBool();
}

float AiTestDriver::sliderValue(const std::string& sliderId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return 0;
    auto w = s->getWidget(sliderId);
    if (!w) return 0;
    auto v = w->property("value");
    return v.isNumber() ? static_cast<float>(v.asNumber()) : 0;
}

int AiTestDriver::listSelectedIndex(const std::string& listId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return -1;
    auto w = s->getWidget(listId);
    if (!w) return -1;
    auto v = w->property("_selectedIndex");
    return v.isNumber() ? v.asInt() : -1;
}

int AiTestDriver::tabsActiveIndex(const std::string& tabsId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return -1;
    auto w = s->getWidget(tabsId);
    if (!w) return -1;
    auto v = w->property("activeIndex");
    return v.isNumber() ? v.asInt() : 0;
}

Rect AiTestDriver::widgetBounds(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return {0,0,0,0};
    auto w = s->getWidget(widgetId);
    return w ? w->layoutBounds() : Rect{0,0,0,0};
}

bool AiTestDriver::widgetExists(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return false;
    return s->getWidget(widgetId) != nullptr;
}

WidgetType AiTestDriver::widgetType(const std::string& widgetId) {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return WidgetType::Custom;
    auto w = s->getWidget(widgetId);
    return w ? w->type() : WidgetType::Custom;
}

// ---- 表面 ----
void AiTestDriver::setActiveSurface(const std::string& surfaceId) {
    impl_->activeSurface = surfaceId;
}

std::vector<std::string> AiTestDriver::allWidgetIds() {
    auto s = impl_->engine.getSurface(impl_->activeSurface);
    if (!s) return {};
    std::vector<std::string> ids;
    for (auto& [id, w] : s->widgets()) ids.push_back(id);
    return ids;
}

// ---- 回调 ----
std::vector<std::string> AiTestDriver::capturedActions() const {
    return impl_->actionLog;
}

std::string AiTestDriver::lastAction() const {
    return impl_->lastActionName;
}

std::string AiTestDriver::lastActionSource() const {
    return impl_->lastActionSrc;
}

// ---- 布局验证 ----
bool AiTestDriver::boundsOverlap(const std::string& a, const std::string& b) {
    Rect ra = widgetBounds(a), rb = widgetBounds(b);
    return !(ra.x + ra.w <= rb.x || rb.x + rb.w <= ra.x ||
             ra.y + ra.h <= rb.y || rb.y + rb.h <= ra.y);
}

bool AiTestDriver::boundsContains(const std::string& parent, const std::string& child) {
    Rect rp = widgetBounds(parent), rc = widgetBounds(child);
    return rc.x >= rp.x && rc.y >= rp.y &&
           rc.x + rc.w <= rp.x + rp.w &&
           rc.y + rc.h <= rp.y + rp.h;
}

} // namespace jui::test
