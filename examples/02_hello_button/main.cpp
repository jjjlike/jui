/**
 * @example 02_hello_button.cpp
 * @brief 示例02 — 第一个交互：按钮点击 + action 回调
 *
 * 【学习目标】
 *   - 理解 action 回调机制
 *   - 掌握 Button 控件的 JSON 声明方式
 *   - 了解 Row 水平布局容器
 *
 * 【覆盖控件】Button, Text, Row
 * 【难度】★☆☆☆☆
 *
 * ╔══════════════════════════════════════════════════════════╗
 * ║             界面视觉结构（窗口 450×300 px）              ║
 * ╠══════════════════════════════════════════════════════════╣
 * ║                                                          ║
 * ║  ┌──────────────────────────────────────────────────┐    ║
 * ║  │  Column "root"  ─ 垂直布局根容器                  │    ║
 * ║  │                                                    │    ║
 * ║  │  ┌────────────────────────────────────────────┐   │    ║
 * ║  │  │  Text "title"                              │   │    ║
 * ║  │  │  "点击下方按钮:"  (16px, 黑色)             │   │    ║
 * ║  │  └────────────────────────────────────────────┘   │    ║
 * ║  │       ↑ 16px 间距（自动内边距）                    │    ║
 * ║  │  ┌────────────────────────────────────────────┐   │    ║
 * ║  │  │  Row "btns"  ─ 水平布局容器                │   │    ║
 * ║  │  │                                              │   │    ║
 * ║  │  │  ┌─────────┐ ┌─────────────┐ ┌─────────┐  │   │    ║
 * ║  │  │  │ Button   │ │ Button      │ │ Button   │  │   │    ║
 * ║  │  │  │ "btn1"   │ │ "btn2"      │ │ "btn3"   │  │   │    ║
 * ║  │  │  │ "按钮A"  │ │ "点我问候"  │ │ "点我告别"│  │   │    ║
 * ║  │  │  │ ¬action  │ │ action=hello│ │ action=bye│  │   │    ║
 * ║  │  │  │ 纯展示   │ │ → 弹窗问候 │ │ → 弹窗告别│  │   │    ║
 * ║  │  │  └─────────┘ └─────────────┘ └─────────┘  │   │    ║
 * ║  │  └────────────────────────────────────────────┘   │    ║
 * ║  └──────────────────────────────────────────────────┘    ║
 * ║                                                          ║
 * ║  【交互行为】                                            ║
 * ║  · 按钮A: 点击无回调（action="" 或未设置）               ║
 * ║  · 按钮B: 点击弹出 MessageBox "你好! 来自: btn2"         ║
 * ║  · 按钮C: 点击弹出 MessageBox "再见! 来自: btn3"         ║
 * ║  · 鼠标掠过按钮: Hover 变亮蓝色 (Normal→Hovered→Pressed) ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * ──────────────────────────────────────────────────────────
 *  组件层级树（邻接表模型）:
 * ──────────────────────────────────────────────────────────
 *  root (Column)
 *   ├── title (Text)     ← id 引用: "title"
 *   └── btns  (Row)      ← id 引用: "btns"
 *        ├── btn1 (Button)  ← id 引用: "btn1"
 *        ├── btn2 (Button)  ← id 引用: "btn2"  [action="hello"]
 *        └── btn3 (Button)  ← id 引用: "btn3"  [action="bye"]
 *
 * ──────────────────────────────────────────────────────────
 *  交互数据流:
 * ──────────────────────────────────────────────────────────
 *  用户点击 btn2
 *    → D2DRenderer::onMouseUp() 命中 btn2
 *    → actionCallback_("main", "hello", "btn2", "")
 *    → onAction() 匹配 action=="hello"
 *    → MessageBox("你好! 来自: btn2")
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>
#include <string>

using namespace jui;

// 引擎指针（全局，供回调使用）
JUIEngine* g_engine = nullptr;

// ============================================================
// Step 1: 界面JSON — 声明组件树与视觉结构
//
//   空间排布示意（Y轴自上而下，X轴自左向右）：
//
//   y=0  ┌─────────────────────────────────────┐
//        │ title: "点击下方按钮:" (16px)        │  ← Column 子项 #1
//   y~24 ├─────────────────────────────────────┤
//        │ btns: [btn1] [btn2] [btn3]          │  ← Column 子项 #2
//        │   ← 水平均匀排列 (Row布局) →         │     Row 内子项水平排布
//   y~56 └─────────────────────────────────────┘
//
//   每个按钮自适应文字宽度 (min 70px)，按钮间自动间距
// ============================================================
static const char* UI_JSON = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["title","btn"]}}}},
        {"id":"title","component":{"Text":{"text":{"literalString":"点击下方按钮:"},"fontSize":{"literalNumber":16}}}},
        {"id":"btn","component":{"Button":{"text":{"literalString":"点我问候"},"action":"hello"}}}
    ]}})";

// ============================================================
// Step 2: Action 回调 — 处理按钮点击
//   参数:
//     surfaceId — 触发回调的表面ID
//     action    — Button 的 action 属性值
//     sourceId  — 触发按钮的ID
//     context   — 附加上下文（一般为空）
// ============================================================
void onAction(const std::string& /*surfaceId*/, const std::string& action,
              const std::string& sourceId, const std::string& /*context*/) {
    if (action == "hello") {
        MessageBoxA(nullptr, ("你好! 来自: " + sourceId).c_str(), "JUI Example 02", MB_OK);
    }
}

// ============================================================
// Step 3: 窗口过程
// ============================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* engine = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        engine = new JUIEngine(); g_engine = engine;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(engine));
        engine->initialize(hwnd);

        // A2UI 四步加载
        engine->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        engine->processMessage(UI_JSON);
        engine->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");

        // Step 3.1: 注册回调 — 所有交互事件在此处理
        engine->setActionCallback(onAction);

        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
        if (engine) engine->render();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:   // 鼠标左键按下
        if (engine) engine->onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:     // 鼠标左键释放（触发点击回调）
        if (engine) engine->onMouseUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE:     // 鼠标移动（更新hover状态）
        if (engine) engine->onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;
    case WM_SIZE:
        if (engine) engine->onSize(LOWORD(lp), HIWORD(lp));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DESTROY:
        delete engine; PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"JUI_Example02";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, L"JUI_Example02", L"JUI Example 02: Button Interaction",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 450, 300,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) { CoUninitialize(); return 1; }
    ShowWindow(hwnd, nShow); UpdateWindow(hwnd);
    MSG msg = {}; while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
