/**
 * @example 10_full_app.cpp
 * @brief 示例10 — 完整应用：多Surface + Tabs + 表单 + 列表 + 数据绑定全功能集成
 * 
 * 【学习目标】
 *   - 多 Surface 共存管理
 *   - 所有核心控件联合使用
 *   - 综合事件处理（鼠标+键盘+滚轮+IME）
 *   - 完整消息循环扩展模式
 * 
 * 【覆盖控件】全部16种控件类型
 * 【难度】★★★★★
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <string>

using namespace jui;
JUIEngine* g_engine = nullptr;
int g_activePage = 0;

// 3个页面的JSON（模拟真实应用）
static const char* PAGE_DASHBOARD = R"({"surfaceUpdate":{"surfaceId":"main","components":[
    {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","title","stats","action"]}}}},
    {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"仪表盘","id":"p0"},{"title":"数据列表","id":"p1"},{"title":"系统设置","id":"p2"}]}}},
    {"id":"d","component":{"Divider":{}}},
    {"id":"title","component":{"Text":{"text":{"literalString":"今日概览"},"fontSize":{"literalNumber":22},"fontWeight":{"literalString":"bold"}}}},
    {"id":"stats","component":{"Row":{"children":{"explicitList":["card1","card2","card3"]}}}},
    {"id":"card1","component":{"Card":{"children":{"explicitList":["c1t","c1v"]}}}},
    {"id":"c1t","component":{"Text":{"text":{"literalString":"用户数"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
    {"id":"c1v","component":{"Text":{"text":{"literalString":"12,580"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}},
    {"id":"card2","component":{"Card":{"children":{"explicitList":["c2t","c2v"]}}}},
    {"id":"c2t","component":{"Text":{"text":{"literalString":"活跃率"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
    {"id":"c2v","component":{"Text":{"text":{"literalString":"87.3%"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#22AA44"}}}},
    {"id":"card3","component":{"Card":{"children":{"explicitList":["c3t","c3v"]}}}},
    {"id":"c3t","component":{"Text":{"text":{"literalString":"待处理"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
    {"id":"c3v","component":{"Text":{"text":{"literalString":"3"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#DD4422"}}}},
    {"id":"action","component":{"Row":{"children":{"explicitList":["btn_refresh","btn_export"]}}}},
    {"id":"btn_refresh","component":{"Button":{"text":{"literalString":"刷新数据"},"action":"refresh"}}},
    {"id":"btn_export","component":{"Button":{"text":{"literalString":"导出报表"}}}}
]}})";

static const char* PAGE_LIST = R"({"surfaceUpdate":{"surfaceId":"main","components":[
    {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","title","list"]}}}},
    {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"仪表盘","id":"p0"},{"title":"数据列表","id":"p1"},{"title":"系统设置","id":"p2"}]}}},
    {"id":"d","component":{"Divider":{}}},
    {"id":"title","component":{"Text":{"text":{"literalString":"List Virtual Scroll"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
    {"id":"list","component":{"List":{"itemCount":{"literalNumber":50000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":450},"height":{"literalNumber":380}}}}
]}})";

static const char* PAGE_SETTINGS = R"({"surfaceUpdate":{"surfaceId":"main","components":[
    {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","title","cb1","cb2","tg1","div","slabel","slider","save"]}}}},
    {"id":"tabs","component":{"Tabs":{"tabs":[{"title":"仪表盘","id":"p0"},{"title":"数据列表","id":"p1"},{"title":"系统设置","id":"p2"}]}}},
    {"id":"d","component":{"Divider":{}}},
    {"id":"title","component":{"Text":{"text":{"literalString":"系统设置"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
    {"id":"cb1","component":{"CheckBox":{"text":{"literalString":"自动保存"},"value":{"literalBoolean":true}}}},
    {"id":"cb2","component":{"CheckBox":{"text":{"literalString":"匿名统计"}}}},
    {"id":"tg1","component":{"Toggle":{"label":{"literalString":"暗黑模式"}}}},
    {"id":"div","component":{"Divider":{}}},
    {"id":"slabel","component":{"Text":{"text":{"literalString":"刷新间隔: 30秒"},"fontSize":{"literalNumber":14}}}},
    {"id":"slider","component":{"Slider":{"min":{"literalNumber":5},"max":{"literalNumber":120},"value":{"literalNumber":30}}}},
    {"id":"save","component":{"Button":{"text":{"literalString":"保存设置"},"action":"save_settings"}}}
]}})";

void switchAppPage(int p) {
    g_activePage = p;
    g_engine->processMessage(R"({"deleteSurface":{"surfaceId":"main"}})");
    g_engine->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    if(p==0) g_engine->processMessage(PAGE_DASHBOARD);
    else if(p==1) g_engine->processMessage(PAGE_LIST);
    else g_engine->processMessage(PAGE_SETTINGS);
    auto s=g_engine->getSurface("main");
    if(s){ auto tw=s->getWidget("tabs"); if(tw)tw->setProperty("activeIndex",JValue(p)); }
    g_engine->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE:
        e = new JUIEngine(); g_engine=e; SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        switchAppPage(0);
        e->setActionCallback([](const std::string&,const std::string& a,const std::string& s,const std::string& c){
            if(a=="tab_switch"){
                if(s=="p0")switchAppPage(0); else if(s=="p1")switchAppPage(1); else if(s=="p2")switchAppPage(2);
            }
            if(a=="slider_change"&&!c.empty()){
                int v=std::stoi(c); auto sur=g_engine->getSurface("main");
                if(sur){ auto lbl=sur->getWidget("slabel"); if(lbl)lbl->setProperty("text",JValue("刷新间隔: "+std::to_string(v)+"秒"));}
            }
            if(a=="refresh") MessageBoxW(nullptr,L"数据已刷新!",L"App",MB_OK);
        });
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDOWN: if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONUP:   if(e)e->onMouseUp(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_MOUSEMOVE:   if(e)e->onMouseMove(GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); return 0;
    case WM_MOUSEWHEEL: { int d=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA; if(e)e->onMouseWheel((float)(-d*40)); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
    case WM_CHAR: if(e)e->onCharInput((uint32_t)wp); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_KEYDOWN: if(e)e->onKeyDown((int)wp); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_IME_STARTCOMPOSITION: e->onIMEStart(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_IME_ENDCOMPOSITION:   e->onIMEEnd(""); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_DESTROY: delete e; PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex10"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex10",L"JUI Example 10: Full Application",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,640,550,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
