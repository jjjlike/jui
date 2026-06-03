/**
 * @example 08_grid_demo.cpp
 * @brief 示例08 — 虚拟表格：50000行x5列 + 固定表头 + 排序切换
 *
 * 【学习目标】
 *   - Grid 控件的列定义（colCount/colTitles/colWidths）
 *   - 表头渲染与排序箭头
 *   - 虚拟行滚动
 *
 * 【覆盖控件】Grid, Text, Column
 * 【难度】★★★☆☆
 *
 * ╔══════════════════════════════════════════════════════╗
 * ║          界面视觉结构（窗口 680×580 px）              ║
 * ╠══════════════════════════════════════════════════════╣
 * ║  Text "title": "Data Grid 50K" (18px Bold)           ║
 * ║  Text "desc":  "Fixed header + virtual rows" (12px)  ║
 * ║                                                      ║
 * ║  ┌──────────┬──────────┬──────────┬──────────┬─────┐ ║
 * ║  │ ID(60)   │ 姓名(100)│ 邮箱(200)│ 电话(120)│城市 │ ║←固定表头
 * ║  ├──────────┼──────────┼──────────┼──────────┼─────┤ ║
 * ║  │ 0        │ R0C1     │ R0C2     │ R0C3     │R0C4 │ ║←数据行
 * ║  │ 1        │ R1C1     │ R1C2     │ R1C3     │R1C4 │ ║  仅渲染
 * ║  │ 2        │ R2C1     │ R2C2     │ R2C3     │R2C4 │ ║  ~12可见行
 * ║  │ ...      │ ...      │ ...      │ ...      │...  │ ║
 * ║  │ 49,999   │ ...      │ ...      │ ...      │...  │ ║  50K行总计
 * ║  └──────────┴──────────┴──────────┴──────────┴─────┘ ║
 * ║       ↑ 滚轮垂直滚动，点击单元格高亮选中 ↑             ║
 * ║                                                      ║
 * ║  组件层级: root → [title, desc, grid]                 ║
 * ║  Grid列定义通过C++ setProperty动态设置                ║
 * ╚══════════════════════════════════════════════════════╝
 */

#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>

using namespace jui;

static const char* UI_JSON = R"J({
    "surfaceUpdate":{"surfaceId":"main","components":[
        {"id":"root","component":{"Column":{"children":{"explicitList":["title","desc","grid"]}}}},
        {"id":"title","component":{"Text":{"text":{"literalString":"Data Grid (50K rows x 5 cols)"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
        {"id":"desc","component":{"Text":{"text":{"literalString":"Fixed header + virtual row scroll"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#999999"}}}},
        {"id":"grid","component":{"Grid":{"width":{"literalNumber":580},"height":{"literalNumber":400}}}}
    ]}})J";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    JUIEngine* e = reinterpret_cast<JUIEngine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        e = new JUIEngine(); SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)e);
        e->initialize(hwnd);
        e->processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        e->processMessage(UI_JSON);
        // 通过代码设置表格列定义和行数（JSON暂不支持复杂嵌套结构）
        auto s = e->getSurface("main");
        if(s){ auto gw=s->getWidget("grid"); if(gw){
            gw->setProperty("colCount",JValue(5));
            gw->setProperty("colTitles",JValue::fromArray({JValue("ID"),JValue("姓名"),JValue("邮箱"),JValue("电话"),JValue("城市")}));
            gw->setProperty("colWidths",JValue::fromArray({JValue(60.0),JValue(100.0),JValue(200.0),JValue(120.0),JValue(100.0)}));
            gw->setProperty("rowCount",JValue(50000));
        }}
        e->processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_TIMER: { InvalidateRect(hwnd, nullptr, FALSE); return 0; }
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd, &ps); if(e)e->render(); EndPaint(hwnd, &ps); return 0; }
    case WM_SIZE: { if(e)e->onSize(LOWORD(lp),HIWORD(lp)); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
    case WM_LBUTTONDOWN: { if(e)e->onMouseDown(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),0); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
    case WM_MOUSEWHEEL: { int d=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA; if(e)e->onMouseWheel((float)(-d*40)); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
    case WM_DESTROY: { delete e; PostQuitMessage(0); return 0; }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW w={}; w.cbSize=sizeof(w); w.lpfnWndProc=WndProc; w.hInstance=hI;
    w.hCursor=LoadCursor(nullptr, IDC_ARROW); w.lpszClassName=L"JUI_Ex08"; RegisterClassExW(&w);
    HWND hw=CreateWindowExW(0,L"JUI_Ex08",L"JUI Example 08: Grid Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,680,580,nullptr,nullptr,hI,nullptr);
    ShowWindow(hw,nS); UpdateWindow(hw);
    MSG m={}; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    CoUninitialize(); return (int)m.wParam;
}
