/**
 * @example 01_hello_window.cpp
 * @brief 示例01 — 最小启动：引擎初始化、窗口创建、显示单个文字
 *
 * 【学习目标】
 *   - 理解 JUIEngine 的基本初始化流程
 *   - 掌握 Win32 窗口消息循环与 JUI 的集成方式
 *   - 了解 A2UI JSON 协议的四步加载流程
 *
 * 【覆盖控件】Text
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
 * ║  │         ↑ 唯一子组件，无交互                   │    ║
 * ║  └──────────────────────────────────────────────┘    ║
 * ║                                                      ║
 * ║  组件层级: root(Column) → txt(Text)                  ║
 * ║  数据流: JSON → createSurface → surfaceUpdate        ║
 * ║          → beginRendering → layout → paint            ║
 * ╚══════════════════════════════════════════════════════╝
 */

#include <jui/jui.h>
#include <windows.h>

using namespace jui;

// ============================================================
// Step 1: 定义界面 JSON（A2UI 协议）
//   - createSurface: 创建名为 "main" 的UI表面
//   - surfaceUpdate: 声明组件树（Column → Text）
//   - beginRendering: 指定根组件，触发布局计算
// ============================================================
static const char* UI_CREATE = R"({"createSurface":{"surfaceId":"main"}})";

static const char* UI_COMPONENTS = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        // root: Column垂直布局容器，包含一个文字控件
        {"id":"root", "component":{"Column":{"children":{"explicitList":["txt"]}}}},
        // txt: 文字控件，蓝色粗体16号字
        {"id":"txt", "component":{"Text":{
            "text":{"literalString":"Hello, JUI!"},       // 文字内容
            "fontSize":{"literalNumber":16},               // 字号(px)
            "fontWeight":{"literalString":"bold"},          // 粗体
            "textColor":{"literalString":"#2266CC"}         // 蓝色
        }}}
    ]}})";

static const char* UI_RENDER = R"({"beginRendering":{"surfaceId":"main","root":"root"}})";

// ============================================================
// Step 2: 窗口过程 — 绑定引擎事件
// ============================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // 从 GWLP_USERDATA 获取引擎指针
    JUIEngine* engine = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        // Step 2.1: 创建引擎并保存到窗口
        engine = new JUIEngine();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(engine));

        // Step 2.2: 初始化 D2D 渲染器
        engine->initialize(hwnd);

        // Step 2.3: 加载 JSON 界面（A2UI 四步协议）
        engine->processMessage(UI_CREATE);       // 1. 创建Surface
        engine->processMessage(UI_COMPONENTS);    // 2. 声明组件树
        engine->processMessage(UI_RENDER);        // 3. 开始渲染（触发布局）

        // Step 2.4: 启动16ms定时器驱动重绘（约60fps）
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }

    case WM_TIMER:
        // 定时器触发 → 标记窗口需要重绘
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        // Step 2.5: 执行一帧渲染
        if (engine) engine->render();
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        if (engine) engine->onSize(LOWORD(lp), HIWORD(lp));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        delete engine;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================
// Step 3: WinMain — 创建窗口，运行消息循环
// ============================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    // Step 3.1: COM 初始化（D2D 依赖）
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Step 3.2: 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"JUI_Example01";
    RegisterClassExW(&wc);

    // Step 3.3: 创建窗口（400x300 像素）
    HWND hwnd = CreateWindowExW(0, L"JUI_Example01", L"JUI Example 01: Hello Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) { CoUninitialize(); return 1; }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    // Step 3.4: 消息循环
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
