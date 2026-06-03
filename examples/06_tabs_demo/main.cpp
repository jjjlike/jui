/**
 * @example 06_tabs_demo.cpp
 * @brief 示例06 — Tabs分页：页面切换 + 动态重建Surface
 * 
 * 【学习目标】
 *   - Tabs 控件的声明与 tab_switch 回调处理
 *   - 页面切换时的 Surface 销毁/重建模式
 *   - 理解 deleteSurface → createSurface → surfaceUpdate → beginRendering 完整流程
 * 
 * 【覆盖控件】Tabs, Text, Button  (3个Tab页面)
 * 【难度】★★★☆☆
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;
JUIEngine* g_engine = nullptr;
int g_page = 0;

// 3个Tab页面JSON
static const char* PAGES[] = {
    R"({"surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","content"]}}}},
        {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"首页","id":"t0"},{"title":"设置","id":"t1"},{"title":"关于","id":"t2"}]}}},
        {"id":"d","component":{"Divider":{}}},
        {"id":"content","component":{"Text":{"text":{"literalString":"欢迎来到首页!"},"fontSize":{"literalNumber":28},"fontWeight":{"literalString":"bold"}}}}
    ]}})",
    R"({"surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","content","btn"]}}}},
        {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"首页","id":"t0"},{"title":"设置","id":"t1"},{"title":"关于","id":"t2"}]}}},
        {"id":"d","component":{"Divider":{}}},
        {"id":"content","component":{"Text":{"text":{"literalString":"设置页面"},"fontSize":{"literalNumber":28},"fontWeight":{"literalString":"bold"}}}},
        {"id":"btn","component":{"Button":{"text":{"literalString":"保存设置"},"action":"save"}}}
    ]}})",
    R"({"surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","content","info"]}}}},
        {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"首页","id":"t0"},{"title":"设置","id":"t1"},{"title":"关于","id":"t2"}]}}},
        {"id":"d","component":{"Divider":{}}},
        {"id":"content","component":{"Text":{"text":{"literalString":"关于 JUI v0.2"},"fontSize":{"literalNumber":28},"fontWeight":{"literalString":"bold"}}}},
        {"id":"info","component":{"Text":{"text":{"literalString":"A2UI兼容的极简C++ UI引擎"},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#666666"}}}}
    ]}})"
};

void switchPage(int p) {
    g_page = p;
    g_engine->processMessage(R"({"deleteSurface":{"surfaceId":"main"}})");
    g_engine->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    g_engine->processMessage(PAGES[p]);
    auto s = g_engine->getSurface("main");
    if(s){ auto tw=s->getWidget("tabs"); if(tw)tw->setProperty("activeIndex",JValue(p)); }
    g_engine->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        e = new JUIEngine(); g_engine=e; SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        switchPage(0);
        e->setActionCallback([](const std::string&,const std::string& a,const std::string& s,const std::string&){
            if(a=="tab_switch"){
                if(s=="t0") switchPage(0); else if(s=="t1") switchPage(1); else if(s=="t2") switchPage(2);
            }
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex06"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex06",L"JUI Example 06: Tabs Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,500,400,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
