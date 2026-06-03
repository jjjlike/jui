/**
 * @example 07_list_demo.cpp
 * @brief 示例07 — 虚拟滚动列表：10万项 + 滚动条 + 鼠标滚轮 + 点击选中
 * 
 * 【学习目标】
 *   - List 控件的虚拟滚动机制（仅渲染可见项）
 *   - itemCount/itemHeight 属性配置
 *   - WM_MOUSEWHEEL 滚轮集成
 * 
 * 【覆盖控件】List, Text, Column, Tabs
 * 【难度】★★★☆☆
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;

static const char* UI_JSON = R"JSON({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["title","list"]}}}},
        {"id":"title","component":{"Text":{"text":{"literalString":"Virtual Scroll List (100K items)"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        {"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":400},"height":{"literalNumber":420}}}}
    ]}})JSON";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        e = new JUIEngine(); SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        e->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        e->processMessage(UI_JSON);
        e->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:   if(e)e->onMouseUp(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_MOUSEMOVE:   if(e)e->onMouseMove(GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); return 0;
    case WM_MOUSEWHEEL:
        // 滚轮增量：WHEEL_DELTA = 120，转为每次滚动40px
        { int d=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA; if(e)e->onMouseWheel((float)(-d*40)); }
        InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr,IDC_ARROW); w.lpszClassName=L"JUI_Ex07"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex07",L"JUI Example 07: List Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,520,580,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
