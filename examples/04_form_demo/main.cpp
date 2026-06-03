/**
 * @example 04_form_demo.cpp
 * @brief 示例04 — 表单编辑框：TextField 输入 + 键盘交互 + 中文IME
 *
 * 【学习目标】
 *   - TextField 控件的声明与placeholder用法
 *   - 键盘事件集成（WM_CHAR + WM_KEYDOWN）
 *   - 中文 IME 输入集成（WM_IME_COMPOSITION）
 *
 * 【覆盖控件】TextField, Button, Text, Row, Column
 * 【难度】★★☆☆☆
 *
 * ╔══════════════════════════════════════════════════════╗
 * ║          界面视觉结构（窗口 480×350 px）              ║
 * ╠══════════════════════════════════════════════════════╣
 * ║                                                      ║
 * ║  Column "root" — 垂直表单布局                        ║
 * ║  ┌──────────────────────────────────────────────┐    ║
 * ║  │ Text "title": "Login" (18px Bold)            │    ║
 * ║  │                                              │    ║
 * ║  │ Row "r1" ─ 水平排布                          │    ║
 * ║  │  ┌───────────┬───────────────────────────┐  │    ║
 * ║  │  │ Text "l1" │ TextField "tf1" (200px)   │  │    ║
 * ║  │  │ "Name:"   │ [enter name............]   │  │    ║
 * ║  │  └───────────┴───────────────────────────┘  │    ║
 * ║  │                                              │    ║
 * ║  │ Row "r2" ─ 水平排布                          │    ║
 * ║  │  ┌───────────┬───────────────────────────┐  │    ║
 * ║  │  │ Text "l2" │ TextField "tf2" (200px)   │  │    ║
 * ║  │  │ "Pass:"   │ [enter pass............]   │  │    ║
 * ║  │  └───────────┴───────────────────────────┘  │    ║
 * ║  │                                              │    ║
 * ║  │ ┌──────────────────────────────────────┐    │    ║
 * ║  │ │ Button "btn": "Login" (action=login) │    │    ║
 * ║  │ └──────────────────────────────────────┘    │    ║
 * ║  └──────────────────────────────────────────────┘    ║
 * ║                                                      ║
 * ║  组件层级: root → [title, r1[l1,tf1], r2[l2,tf2], btn]║
 * ║  交互: Ctrl+C/V/X/A 剪贴板, Home/End, IME中文输入      ║
 * ╚══════════════════════════════════════════════════════╝
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>
#include <imm.h>

using namespace jui;

// 界面JSON: 一个简单的登录表单
static const char* UI_JSON = R"({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["title","r1","r2","btn"]}}}},
        {"id":"title","component":{"Text":{"text":{"literalString":"用户 登录"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        {"id":"r1","component":{"Row":{"children":{"explicitList":["l1","tf1"]}}}},
        {"id":"l1","component":{"Text":{"text":{"literalString":"用户名:"},"fontSize":{"literalNumber":14}}}},
        {"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"请输入用户名"},"width":{"literalNumber":200}}}},
        {"id":"r2","component":{"Row":{"children":{"explicitList":["l2","tf2"]}}}},
        {"id":"l2","component":{"Text":{"text":{"literalString":"密码:"},"fontSize":{"literalNumber":14}}}},
        {"id":"tf2","component":{"TextField":{"placeholder":{"literalString":"请输入密码"},"width":{"literalNumber":200}}}},
        {"id":"btn","component":{"Button":{"text":{"literalString":"登录"},"action":"login"}}}
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
        e->setActionCallback([](const std::string&,const std::string& a,const std::string& s,const std::string&){
            if(a=="login") MessageBoxW(nullptr,L"登录按钮被点击!",L"Form Demo",MB_OK);
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    // ---- 鼠标事件 ----
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:   if(e)e->onMouseUp(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_MOUSEMOVE:   if(e)e->onMouseMove(GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); return 0;
    // ---- 键盘事件 ----
    case WM_CHAR:        if(e)e->onCharInput((uint32_t)wp); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_KEYDOWN:     if(e)e->onKeyDown((int)wp); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_KEYUP:       if(e)e->onKeyUp((int)wp); return 0;
    // ---- IME 中文输入 ----
    case WM_IME_STARTCOMPOSITION: e->onIMEStart(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_IME_ENDCOMPOSITION:   e->onIMEEnd(""); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_IME_COMPOSITION: {
        e->onIMEStart();
        HIMC hIMC = ImmGetContext(hwnd);
        if(hIMC){
            if(lp & GCS_RESULTSTR){
                LONG len=ImmGetCompositionStringW(hIMC,GCS_RESULTSTR,nullptr,0);
                if(len>0){ std::wstring ws(len/2,L'\0'); ImmGetCompositionStringW(hIMC,GCS_RESULTSTR,&ws[0],len);
                    int ul=WideCharToMultiByte(CP_UTF8,0,ws.c_str(),-1,nullptr,0,nullptr,nullptr);
                    std::string u(ul-1,'\0'); WideCharToMultiByte(CP_UTF8,0,ws.c_str(),-1,&u[0],ul,nullptr,nullptr);
                    e->onIMEEnd(u); }
            }
            ImmReleaseContext(hwnd,hIMC);
        }
        InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex04"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex04",L"JUI Example 04: Form Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,480,350,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
