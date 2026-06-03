/**
 * @example 05_controls_demo.cpp
 * @brief 示例05 — 控件合集：CheckBox复选框 + Toggle开关 + Slider滑动条
 * 
 * 【学习目标】
 *   - CheckBox 的声明与 checked 状态读取
 *   - Toggle 开关控件
 *   - Slider 滑动条 + action 回调联动文字更新
 * 
 * 【覆盖控件】CheckBox, Toggle, Slider, Text, Column
 * 【难度】★★☆☆☆
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;

static const char* UI_JSON = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":[
            "title","cb1","cb2","tg1","txt1","sl1","txt2","sl2"
        ]}}}},
        {"id":"title","component":{"Text":{"text":{"literalString":"控件合集"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        // CheckBox 1: 初始未勾选
        {"id":"cb1","component":{"CheckBox":{"text":{"literalString":"同意用户协议"}}}},
        // CheckBox 2: 初始已勾选
        {"id":"cb2","component":{"CheckBox":{"text":{"literalString":"订阅邮件通知"},"value":{"literalBoolean":true}}}},
        // Toggle 开关: 初始打开
        {"id":"tg1","component":{"Toggle":{"label":{"literalString":"启用通知"},"value":{"literalBoolean":true}}}},
        // Slider 1: 音量（关联文字 r1txt）
        {"id":"txt1","component":{"Text":{"text":{"literalString":"音量: 50%"},"fontSize":{"literalNumber":14}}}},
        {"id":"sl1","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":50}}}},
        // Slider 2: 亮度（关联文字 r2txt）
        {"id":"txt2","component":{"Text":{"text":{"literalString":"亮度: 80%"},"fontSize":{"literalNumber":14}}}},
        {"id":"sl2","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":80}}}}
    ]}})";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        e = new JUIEngine(); SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        e->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        e->processMessage(UI_JSON);
        e->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
        // Slider值变化回调: 更新关联的Text标签
        e->setActionCallback([e](const std::string&,const std::string& a,const std::string& s,const std::string& c){
            if(a=="slider_change"&&!c.empty()){
                int v=std::stoi(c); auto sur=e->getSurface("main");
                if(s=="sl1"&&sur){ auto t=sur->getWidget("txt1"); if(t)t->setProperty("text",JValue("音量: "+std::to_string(v)+"%") );}
                if(s=="sl2"&&sur){ auto t=sur->getWidget("txt2"); if(t)t->setProperty("text",JValue("亮度: "+std::to_string(v)+"%") );}
            }
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:   if(e)e->onMouseUp(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_MOUSEMOVE:   if(e)e->onMouseMove(GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); return 0;
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex05"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex05",L"JUI Example 05: Controls Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,480,480,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
