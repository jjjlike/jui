/**
 * @example 09_binding_demo.cpp
 * @brief 示例09 — 数据绑定：DataModel JSON Pointer 路径绑定 + 动态更新
 * 
 * 【学习目标】
 *   - DataModel setValue/getValue API
 *   - dataModelUpdate 批量数据注入
 *   - change 回调监听数据变化
 * 
 * 【覆盖控件】Text (含 path 绑定), Button, Column
 * 【难度】★★★☆☆
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;

// 组件声明：txt_title 使用 path 绑定 "/user/name"
static const char* UI_JSON = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["title","btn1","btn2","btn3"]}}}},
        {"id":"title","component":{"Text":{"text":{"path":"/user/name"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"}}}},
        {"id":"btn1","component":{"Button":{"text":{"literalString":"设为 Alice"},"action":"alice"}}},
        {"id":"btn2","component":{"Button":{"text":{"literalString":"设为 Bob"},"action":"bob"}}},
        {"id":"btn3","component":{"Button":{"text":{"literalString":"设为 Charlie"},"action":"charlie"}}}
    ]}})";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        e = new JUIEngine(); SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        e->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        e->processMessage(UI_JSON);
        // 设置初始数据
        e->processMessage(R"({"dataModelUpdate":{"surfaceId":"main","contents":[
            {"path":"/user","contents":[{"key":"name","valueString":"Guest"}]}
        ]}})");
        e->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
        // 按钮点击：更新DataModel中的用户名
        e->setActionCallback([e](const std::string&,const std::string& a,const std::string&,const std::string&){
            std::string name;
            if(a=="alice") name="Alice";
            else if(a=="bob") name="Bob";
            else if(a=="charlie") name="Charlie";
            else return;
            auto s=e->getSurface("main");
            if(s){ s->dataModel().setValue("/user/name",JValue(name)); }
            // 通知重绘
            InvalidateRect(FindWindowW(L"JUI_Ex09",nullptr),nullptr,FALSE);
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:   if(e)e->onMouseUp(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex09"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex09",L"JUI Example 09: Data Binding",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,480,350,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
