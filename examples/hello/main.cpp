#include <jui/jui.h>
#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <string>

using namespace jui;

// ============================================================
// 全局变量
// ============================================================
JUIEngine engine;
const wchar_t* WINDOW_CLASS = L"JUI_HelloWindow";

// ============================================================
// A2UI JSON 示例 UI 描述
// ============================================================

// 步骤1: 创建 Surface
const char* UI_JSON_CREATE = R"(
{"createSurface": {"surfaceId": "main"}}
)";

// 步骤2: 发送组件定义（v0.8 surfaceUpdate 格式）
const char* UI_JSON_COMPONENTS = R"(
{
  "surfaceUpdate": {
    "surfaceId": "main",
    "components": [
      {
        "id": "root",
        "component": {
          "Column": {
            "children": { "explicitList": ["header", "div1", "row1", "row2", "row3", "div2", "actions"] }
          }
        }
      },
      {
        "id": "header",
        "component": {
          "Card": {
            "children": { "explicitList": ["title", "subtitle"] }
          }
        }
      },
      {
        "id": "title",
        "component": {
          "Text": {
            "text": { "literalString": "JUI 引擎演示" },
            "fontSize": { "literalNumber": 20 },
            "fontWeight": { "literalString": "bold" }
          }
        }
      },
      {
        "id": "subtitle",
        "component": {
          "Text": {
            "text": { "literalString": "A2UI 兼容 | D2D 渲染 | 极简轻量 | 逻辑渲染分离" },
            "fontSize": { "literalNumber": 12 }
          }
        }
      },
      { "id": "div1", "component": { "Divider": {} } },
      {
        "id": "row1",
        "component": {
          "Row": {
            "children": { "explicitList": ["lbl-user", "input-user"] }
          }
        }
      },
      {
        "id": "lbl-user",
        "component": {
          "Text": { "text": { "literalString": "用户名：" }, "fontSize": { "literalNumber": 14 } }
        }
      },
      {
        "id": "input-user",
        "component": {
          "TextField": {
            "placeholder": { "literalString": "请输入用户名" },
            "value": { "path": "/form/username" }
          }
        }
      },
      {
        "id": "row2",
        "component": {
          "Row": {
            "children": { "explicitList": ["lbl-pass", "input-pass"] }
          }
        }
      },
      {
        "id": "lbl-pass",
        "component": {
          "Text": { "text": { "literalString": "密  码：" }, "fontSize": { "literalNumber": 14 } }
        }
      },
      {
        "id": "input-pass",
        "component": {
          "TextField": {
            "placeholder": { "literalString": "请输入密码" },
            "value": { "path": "/form/password" }
          }
        }
      },
      {
        "id": "row3",
        "component": {
          "Row": {
            "children": { "explicitList": ["chk-agree"] }
          }
        }
      },
      {
        "id": "chk-agree",
        "component": {
          "CheckBox": {
            "text": { "literalString": "同意用户协议" },
            "value": { "path": "/form/agreed" }
          }
        }
      },
      { "id": "div2", "component": { "Divider": {} } },
      {
        "id": "actions",
        "component": {
          "Row": {
            "children": { "explicitList": ["btn-login", "btn-cancel"] }
          }
        }
      },
      {
        "id": "btn-login",
        "component": {
          "Button": {
            "text": { "literalString": "登录" },
            "action": { "name": "login" }
          }
        }
      },
      {
        "id": "btn-cancel",
        "component": {
          "Button": {
            "text": { "literalString": "取消" },
            "action": { "name": "cancel" }
          }
        }
      }
    ]
  }
}
)";

// 步骤3: 数据模型
const char* UI_JSON_DATA = R"(
{
  "dataModelUpdate": {
    "surfaceId": "main",
    "path": "/form",
    "contents": [
      { "key": "username", "valueString": "" },
      { "key": "password", "valueString": "" },
      { "key": "agreed", "valueBoolean": false }
    ]
  }
}
)";

// 步骤4: 开始渲染
const char* UI_JSON_RENDER = R"(
{"beginRendering": {"surfaceId": "main", "root": "root"}}
)";

// ============================================================
// 窗口过程
// ============================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            engine.initialize(hwnd);

            // 按照 A2UI 协议分步加载
            engine.processMessage(UI_JSON_CREATE);
            engine.processMessage(UI_JSON_COMPONENTS);
            engine.processMessage(UI_JSON_DATA);
            engine.processMessage(UI_JSON_RENDER);

            // 启动定时器驱动光标闪烁动画 (16ms ≈ 60fps)
            SetTimer(hwnd, 1, 16, nullptr);

            // Action 回调（使用 JSON 字符串传递上下文）
            engine.setActionCallback([](const std::string& surfaceId,
                                         const std::string& action,
                                         const std::string& sourceId,
                                         const std::string& contextJson) {
                wchar_t buf[512];
                swprintf_s(buf, L"Action: %hs\nSurface: %hs\nSource: %hs\nContext: %hs",
                    action.c_str(), surfaceId.c_str(), sourceId.c_str(), contextJson.c_str());
                MessageBoxW(nullptr, buf, L"JUI Action", MB_OK);
            });
            return 0;
        }

        case WM_TIMER:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_SIZE: {
            int w = LOWORD(lParam), h = HIWORD(lParam);
            engine.onSize(w, h);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
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

        case WM_CHAR:
            engine.onCharInput(static_cast<uint32_t>(wParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_IME_STARTCOMPOSITION:
            engine.onIMEStart();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_IME_COMPOSITION: {
            engine.onIMEStart();
            if (lParam & GCS_RESULTSTR) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    LONG len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, nullptr, 0);
                    if (len > 0) {
                        std::wstring ws(len / sizeof(wchar_t), L'\0');
                        ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &ws[0], len);
                        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
                        std::string utf8(utf8Len - 1, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);
                        engine.onIMEEnd(utf8);
                    }
                    ImmReleaseContext(hwnd, hIMC);
                }
            } else if (lParam & GCS_COMPSTR) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    LONG len = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);
                    if (len >= 0) {
                        std::wstring ws(len / sizeof(wchar_t), L'\0');
                        ImmGetCompositionStringW(hIMC, GCS_COMPSTR, &ws[0], len);
                        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
                        if (utf8Len > 1) {
                            std::string utf8(utf8Len - 1, '\0');
                            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &utf8[0], utf8Len, nullptr, nullptr);
                            engine.onIMEComposition(utf8);
                        } else {
                            engine.onIMEComposition("");
                        }
                    }
                    ImmReleaseContext(hwnd, hIMC);
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_IME_ENDCOMPOSITION:
            engine.onIMEEnd("");
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_KEYDOWN:
            engine.onKeyDown(static_cast<int>(wParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_KEYUP:
            engine.onKeyUp(static_cast<int>(wParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================
// WinMain
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
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, WINDOW_CLASS, L"JUI - D2D A2UI Demo",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 520,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) { CoUninitialize(); return 1; }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
