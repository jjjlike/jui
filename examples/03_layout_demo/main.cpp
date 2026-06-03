/**
 * @example 03_layout_demo.cpp
 * @brief 示例03 — 布局系统：Row/Column/Card 三层容器演示
 *
 * 【学习目标】
 *   - 理解 Row(水平)、Column(垂直)、Card(卡片) 三种容器的嵌套使用
 *   - 掌握 children.explicitList 邻接表关联方式
 *   - 了解 Divider 分割线的使用
 *
 * 【覆盖控件】Row, Column, Card, Text, Divider
 * 【难度】★★☆☆☆
 *
 * ╔══════════════════════════════════════════════════════════╗
 * ║           界面视觉结构（窗口 520×400 px）                ║
 * ╠══════════════════════════════════════════════════════════╣
 * ║                                                          ║
 * ║  Column "root"                                           ║
 * ║  ┌───────────────────────────────────────────────────┐   ║
 * ║  │ Card "card1" ─ 圆角白底+灰色边框 (内边距16px)    │   ║
 * ║  │  ┌─────────────────────────────────────────┐     │   ║
 * ║  │  │ Text "c1title": "UserInfo" (18px Bold)  │     │   ║
 * ║  │  │                                         │     │   ║
 * ║  │  │ Row "c1row1" ─ 水平排布                 │     │   ║
 * ║  │  │  ┌─────────────┬────────────────────┐   │     │   ║
 * ║  │  │  │ Text "c1l1" │ Text "c1v1"        │   │     │   ║
 * ║  │  │  │ "Name:"     │ "ZhangSan"         │   │     │   ║
 * ║  │  │  └─────────────┴────────────────────┘   │     │   ║
 * ║  │  │                                         │     │   ║
 * ║  │  │ Row "c1row2" ─ 水平排布                 │     │   ║
 * ║  │  │  ┌─────────────┬────────────────────┐   │     │   ║
 * ║  │  │  │ Text "c1l2" │ Text "c1v2"        │   │     │   ║
 * ║  │  │  │ "Role:"     │ "Engineer"         │   │     │   ║
 * ║  │  │  └─────────────┴────────────────────┘   │     │   ║
 * ║  │  └─────────────────────────────────────────┘     │   ║
 * ║  └───────────────────────────────────────────────────┘   ║
 * ║                                                          ║
 * ║  ───────── Divider "div1" (1px 灰色水平线) ────────    ║
 * ║                                                          ║
 * ║  ┌───────────────────────────────────────────────────┐   ║
 * ║  │ Card "card2" ─ 操作面板                           │   ║
 * ║  │  Text "c2title": "Panel" (18px Bold)              │   ║
 * ║  │  Text "c2desc":  "desc text" (12px 灰色)          │   ║
 * ║  └───────────────────────────────────────────────────┘   ║
 * ║                                                          ║
 * ║  组件层级: root → [card1, div1, card2]                   ║
 * ║    card1 → [c1title, c1row1[c1l1,c1v1], c1row2[c1l2,c1v2]]║
 * ║    card2 → [c2title, c2desc]                              ║
 * ╚══════════════════════════════════════════════════════════╝
 */

#include <jui/jui.h>
#include <jui/test/debug_logger.h>
#include <windows.h>
#include <windowsx.h>
#include <fstream>

using namespace jui;

static const char* UI_JSON = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":[
            "card1","div1","card2"
        ]}}}},
        {"id":"card1","component":{"Card":{"children":{"explicitList":[
            "c1title","c1row1","c1row2"
        ]}}}},
        {"id":"c1title","component":{"Text":{"text":{"literalString":"用户 信息"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        {"id":"c1row1","component":{"Row":{"children":{"explicitList":["c1l1","c1v1"]}}}},
        {"id":"c1l1","component":{"Text":{"text":{"literalString":"姓名:"},"fontSize":{"literalNumber":14}}}},
        {"id":"c1v1","component":{"Text":{"text":{"literalString":"张三"},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#333333"}}}},
        {"id":"c1row2","component":{"Row":{"children":{"explicitList":["c1l2","c1v2"]}}}},
        {"id":"c1l2","component":{"Text":{"text":{"literalString":"角色:"},"fontSize":{"literalNumber":14}}}},
        {"id":"c1v2","component":{"Text":{"text":{"literalString":"工程师"},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#333333"}}}},
        {"id":"div1","component":{"Divider":{}}},
        {"id":"card2","component":{"Card":{"children":{"explicitList":[
            "c2title","c2desc"
        ]}}}},
        {"id":"c2title","component":{"Text":{"text":{"literalString":"操作 面板"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        {"id":"c2desc","component":{"Text":{"text":{"literalString":"Card、Row、Column 可任意嵌套组合，构建丰富的布局层次。"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#5555FF"}}}}
    ]}})";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        e = new JUIEngine();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);

        // ──── 黑盒测试：DebugLogger 输出到固定文件名 ────
        jui::test::DebugLogConfig cfg;
        cfg.enabled = true;
        cfg.includeLocation = false;
        static std::ofstream logFile("ex03_blackbox.log", std::ios::trunc);
        cfg.sink = [](const jui::test::LogEntry& entry) {
            static std::ofstream& f = logFile;
            if (f.is_open()) {
                // 紧凑格式方便正则解析
                char prefix = 'D';
                switch (entry.level) {
                    case jui::test::LogLevel::Info: prefix = 'I'; break;
                    case jui::test::LogLevel::Warning: prefix = 'W'; break;
                    case jui::test::LogLevel::Error: prefix = 'E'; break;
                    default: break;
                }
                f << "[" << prefix << "] [" << entry.stage << "] "
                  << entry.message << std::endl;
            }
        };
        jui::test::DebugLogger::instance().setConfig(cfg);

        e->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        e->processMessage(UI_JSON);
        e->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
        e->setActionCallback([](const std::string&,const std::string& a,const std::string& s,const std::string&){
            MessageBoxA(nullptr, ("Action: " + a + "\nSource: " + s).c_str(), "Layout Demo", MB_OK);
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
        if (e) e->render();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
        if (e) e->onMouseDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:
        if (e) e->onMouseUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), 0);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE:
        if (e) e->onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        if (e) e->onSize(w, h);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_DESTROY:
        delete e; PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={};
    w.cbSize=sizeof(w);
    w.style=CS_HREDRAW|CS_VREDRAW;
    w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW);
    w.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    w.lpszClassName=L"JUI_Ex03"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex03",L"JUI Example 03: Layout Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,520,400,nullptr,nullptr,hI,nullptr);
    if (!hw) { CoUninitialize(); return 1; }
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
