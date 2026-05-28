/**
 * jui_hello — Tabs 控件分页展示所有控件
 */
#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <string>
#include <sstream>

using namespace jui;
JUIEngine engine;
int g_page = 0;

// ============================================================
// JSON 页面定义
// ============================================================
static const char* PAGE_JSONS[] = {
    // 0: Button/Text
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","actions"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"文字与按钮控件"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"不同字号的文字："},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#666"}}}},
{"id":"r2","component":{"Text":{"text":{"literalString":"14px 默认黑色 | 20px 粗体蓝色 | 12px 灰色小字"},"fontSize":{"literalNumber":14}}}},
{"id":"r3","component":{"Text":{"text":{"literalString":"20px 粗体蓝色"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}},
{"id":"actions","component":{"Row":{"children":{"explicitList":["btnA","btnB","btnC"]}}}},
{"id":"btnA","component":{"Button":{"text":{"literalString":"默认"}}}},
{"id":"btnB","component":{"Button":{"text":{"literalString":"长按钮文字"},"action":"click"}}},
{"id":"btnC","component":{"Button":{"text":{"literalString":"提交"}}}}
]}})JSON",

    // 1: TextField
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"编辑框控件"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"r1","component":{"Row":{"children":{"explicitList":["lbl1","tf1"]}}}},
{"id":"lbl1","component":{"Text":{"text":{"literalString":"用户名："},"fontSize":{"literalNumber":14}}}},
{"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"请输入用户名"},"width":{"literalNumber":180}}}},
{"id":"r2","component":{"Row":{"children":{"explicitList":["lbl2","tf2"]}}}},
{"id":"lbl2","component":{"Text":{"text":{"literalString":"邮箱："},"fontSize":{"literalNumber":14}}}},
{"id":"tf2","component":{"TextField":{"placeholder":{"literalString":"user@test.com"},"width":{"literalNumber":220}}}},
{"id":"r3","component":{"Text":{"text":{"literalString":"Ctrl+C/V/X/A 剪贴板, Home/End 跳转, 支持中文"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#999"}}}}
]}})JSON",

    // 2: Check/Toggle
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","r4","r5"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"多选与开关控件"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"复选框："},"fontSize":{"literalNumber":14}}}},
{"id":"r2","component":{"CheckBox":{"text":{"literalString":"同意用户协议"}}}},
{"id":"r3","component":{"CheckBox":{"text":{"literalString":"订阅邮件通知"}}}},
{"id":"r4","component":{"CheckBox":{"text":{"literalString":"记住登录状态"}}}},
{"id":"r5","component":{"Toggle":{"label":{"literalString":"启用通知"}}}}
]}})JSON",

    // 3: Picker/Slider
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","r4"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"滑动条"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"音量 50%"},"fontSize":{"literalNumber":14}}}},
{"id":"r2","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":50}}}},
{"id":"r3","component":{"Text":{"text":{"literalString":"亮度 80%"},"fontSize":{"literalNumber":14}}}},
{"id":"r4","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":80}}}}
]}})JSON",

    // 4: List
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","list"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"虚拟滚动列表 (100,000 项)"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":420},"height":{"literalNumber":380}}}}
]}})JSON",

    // 5: Grid
    R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","grid"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},{"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},{"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"表格 (50,000 行 x 5 列)"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
{"id":"grid","component":{"Grid":{"width":{"literalNumber":520},"height":{"literalNumber":380}}}}
]}})JSON"
};

static void switchPage(int p) {
    g_page = p;
    engine.processMessage(R"({"deleteSurface":{"surfaceId":"main"}})");
    engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    engine.processMessage(PAGE_JSONS[p]);

    auto sur = engine.getSurface("main");
    if (sur) {
        // 设置当前 Tab 高亮
        auto tabsW = sur->getWidget("tabs");
        if (tabsW) tabsW->setProperty("activeIndex", JValue(p));

        // Grid 页面: 设置列和数据
        if (p == 5) {
            auto gw = sur->getWidget("grid");
            if (gw) {
                gw->setProperty("colCount", JValue(5));
                gw->setProperty("colTitles", JValue::fromArray({
                    JValue("ID"), JValue("姓名"), JValue("邮箱"),
                    JValue("电话"), JValue("城市")
                }));
                gw->setProperty("colWidths", JValue::fromArray({
                    JValue(60.0), JValue(100.0), JValue(200.0),
                    JValue(120.0), JValue(100.0)
                }));
                gw->setProperty("rowCount", JValue(50000));
            }
        }
    }

    engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root"}})");
}

// ============================================================
// 窗口过程
// ============================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            engine.initialize(hwnd);
            switchPage(0);
            SetTimer(hwnd, 1, 16, nullptr);

            engine.setActionCallback([](const std::string& surf, const std::string& action,
                                         const std::string& src, const std::string& ctx) {
                // Tabs 切换
                if (action == "tab_switch") {
                    if (src == "tbtn") switchPage(0);
                    else if (src == "ttf") switchPage(1);
                    else if (src == "tck") switchPage(2);
                    else if (src == "tpk") switchPage(3);
                    else if (src == "tls") switchPage(4);
                    else if (src == "tgd") switchPage(5);
                }
                // Slider 值变化 → 更新文字标签
                if (action == "slider_change") {
                    auto sur = engine.getSurface("main");
                    if (sur && !ctx.empty()) {
                        int val = static_cast<int>(std::stod(ctx));
                        if (src == "r2") {
                            auto r1 = sur->getWidget("r1");
                            if (r1) r1->setProperty("text", JValue("音量 " + std::to_string(val) + "%"));
                        }
                        if (src == "r4") {
                            auto r3 = sur->getWidget("r3");
                            if (r3) r3->setProperty("text", JValue("亮度 " + std::to_string(val) + "%"));
                        }
                    }
                }
                // 按钮点击
                if (action == "click")
                    MessageBoxW(nullptr, L"Button clicked!", L"JUI", MB_OK);
            });
            return 0;
        }
        case WM_TIMER:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_SIZE:
            engine.onSize(LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
            engine.render();
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
            engine.onMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_LBUTTONUP:
            engine.onMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_MOUSEMOVE:
            engine.onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            engine.onMouseWheel(static_cast<float>(-delta * 40));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_CHAR:
            engine.onCharInput(static_cast<uint32_t>(wParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_IME_STARTCOMPOSITION: engine.onIMEStart(); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case WM_IME_COMPOSITION: {
            engine.onIMEStart();
            if (lParam & GCS_RESULTSTR) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    LONG len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
                    if (len > 0) {
                        std::wstring ws(len/sizeof(wchar_t), L'\0');
                        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &ws[0], len);
                        int ul = WideCharToMultiByte(CP_UTF8,0,ws.c_str(),-1,nullptr,0,nullptr,nullptr);
                        std::string utf8(ul-1,'\0');
                        WideCharToMultiByte(CP_UTF8,0,ws.c_str(),-1,&utf8[0],ul,nullptr,nullptr);
                        engine.onIMEEnd(utf8);
                    }
                    ImmReleaseContext(hwnd, hIMC);
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_IME_ENDCOMPOSITION: engine.onIMEEnd(""); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case WM_KEYDOWN:
            engine.onKeyDown(static_cast<int>(wParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_KEYUP: engine.onKeyUp(static_cast<int>(wParam)); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"JUI_HelloWindow";
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(0, L"JUI_HelloWindow", L"JUI - Controls Demo",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 680, 600,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) { CoUninitialize(); return 1; }
    ShowWindow(hwnd, nCmdShow); UpdateWindow(hwnd);
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
