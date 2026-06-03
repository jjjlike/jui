/**
 * @example 01_hello_window.cpp
 * @brief 示例01 — 最小启动：引擎初始化、窗口创建、显示文字 + 交互按钮
 *
 * 【学习目标】
 *   - 理解 JUIEngine 的基本初始化流程
 *   - 掌握 Win32 窗口消息循环与 JUI 的集成方式
 *   - 了解 A2UI JSON 协议的四步加载流程
 *   - 掌握 Button 控件的声明与 action 回调绑定
 *
 * 【覆盖控件】Text, Button, Column
 * 【难度】★☆☆☆☆
 *
 * ╔══════════════════════════════════════════════════════╗
 * ║          界面视觉结构（窗口 400×300 px）              ║
 * ╠══════════════════════════════════════════════════════╣
 * ║                                                      ║
 * ║  ┌──────────────────────────────────────────────┐    ║
 * ║  │  Column "root" — 垂直布局根容器              │    ║
 * ║  │                                              │    ║
 * ║  │    ┌────────────────────────────────────┐    │    ║
 * ║  │    │  Text "txt"                        │    │    ║
 * ║  │    │  "Hello, JUI!" (16px, Bold, 蓝色) │    │    ║
 * ║  │    └────────────────────────────────────┘    │    ║
 * ║  │                                              │    ║
 * ║  │    ┌──────────────┐                         │    ║
 * ║  │    │ Button "btn" │ 点击弹出 MessageBox     │    ║
 * ║  │    │ "点我问候"   │ ← action="click"        │    ║
 * ║  │    └──────────────┘                         │    ║
 * ║  │         ↑ 交互验证：渲染+事件完整链路          │    ║
 * ║  └──────────────────────────────────────────────┘    ║
 * ║                                                      ║
 * ║  组件层级: root(Column) → txt(Text), btn(Button)     ║
 * ║  数据流: JSON → createSurface → surfaceUpdate        ║
 * ║          → beginRendering → layout → paint            ║
 * ║  交互: WM_LBUTTONUP → onMouseDown → actionCallback   ║
 * ║        → MessageBox("Hello, World!")                 ║
 * ╚══════════════════════════════════════════════════════╝
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;

// ──────────────────────────────────────────────
// Step 1: 定义界面 JSON（A2UI 协议）
//   - createSurface: 创建名为 "main" 的UI表面
//   - surfaceUpdate: 声明组件树（Column → Text + Button）
//   - beginRendering: 指定根组件，触发布局计算
//
//   空间排布示意：
//   y=0   ┌──────────────────────────────────┐
//         │ txt: "Hello, JUI!" (16px 蓝粗体) │  ← Column #1
//   y=24  ├──────────────────────────────────┤
//         │ btn: "点我问候" (70px×32px)       │  ← Column #2
//   y=56  └──────────────────────────────────┘
// ──────────────────────────────────────────────
static const char* UI_CREATE     = R"({"createSurface":{"surfaceId":"main"}})";
static const char* UI_COMPONENTS = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["txt","btn"]}}}},
        {"id":"txt","component":{"Text":{
            "text":{"literalString":"Hello, JUI!"},
            "fontSize":{"literalNumber":16},
            "fontWeight":{"literalString":"bold"},
            "textColor":{"literalString":"#2266CC"}
        }}},
        {"id":"btn","component":{"Button":{
            "text":{"literalString":"点我问候"},
            "action":"click"
        }}}
    ]}})";
static const char* UI_RENDER     = R"({"beginRendering":{"surfaceId":"main","root":"root"}})";

// ──────────────────────────────────────────────
// Step 2: Action 回调 — 处理按钮点击
// ──────────────────────────────────────────────
void onAction(const std::string& /*surfaceId*/, const std::string& action,
              const std::string& sourceId, const std::string& /*context*/) {
    if (action == "click") {
        MessageBoxA(nullptr, ("Hello, World!\n\n来自: " + sourceId).c_str(),
                    "JUI Example 01", MB_OK);
    }
}

// ──────────────────────────────────────────────
// Step 3: 窗口过程 — 绑定引擎事件
//
// 关键：必须完整处理以下消息以确保交互可见：
//   - WM_LBUTTONDOWN / WM_LBUTTONUP → 按钮点击
//   - WM_MOUSEMOVE → Hover 状态
//   - WM_SIZE → 窗口尺寸变化时重建/缩放渲染目标
// ──────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* engine = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        engine = new JUIEngine();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(engine));

        // 初始化 D2D 渲染器（WC_CREATE 时窗口尺寸可能为0，D2D RT 可能创建失败）
        engine->initialize(hwnd);

        // 加载 JSON 界面（A2UI 四步协议）
        engine->processMessage(UI_CREATE);      // 1. 创建 Surface
        engine->processMessage(UI_COMPONENTS);   // 2. 声明组件树
        engine->processMessage(UI_RENDER);       // 3. 布局 + 渲染树

        // 注册 action 回调 — 所有交互事件统一入口
        engine->setActionCallback(onAction);

        // 启动 16ms 定时器驱动重绘（≈60fps）
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (engine) engine->render();
        EndPaint(hwnd, &ps);
        return 0;
    }

    // ──── 鼠标事件（交互核心）────
    case WM_LBUTTONDOWN:
        if (engine) engine->onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:
        if (engine) engine->onMouseUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE:
        if (engine) engine->onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;

    // ──── 窗口尺寸变化 — 缩放或重建 D2D 渲染目标 ────
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        if (engine) engine->onSize(w, h);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY:
        delete engine;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ──────────────────────────────────────────────
// Step 4: WinMain — 创建窗口，运行消息循环
// ──────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"JUI_Example01";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"JUI_Example01", L"JUI Example 01: Hello Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) { CoUninitialize(); return 1; }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
