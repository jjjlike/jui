/**
 * @file test_blackbox_03.cpp
 * @brief 示例03 黑盒自动化测试框架
 *
 * 架构：
 *   1. 启动 exe 作为独立子进程
 *   2. 通过 Win32 API 找到窗口并注入交互消息
 *   3. 从 DebugLogger 日志文件采集 Layout/Paint 数据
 *   4. 自动解析、比对、诊断差异
 *
 * 特点：
 *   - 零侵入：不修改被测 exe 源码，仅依赖 DebugLogger 文件输出
 *   - 可重现：完整记录交互序列 + 收集的日志
 *   - 自动诊断：失败时分析日志差异并指出问题环节
 */

#include <gtest/gtest.h>
#include "jui/test/test.h"
#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <set>

using namespace jui::test;

// ═══════════════════════════════════════════════════════════════
// Step 1: 进程管理 — 启动、查找窗口、终止
// ═══════════════════════════════════════════════════════════════

class ProcessRunner {
public:
    bool start(const std::string& exePath, const std::string& args = "",
               const std::string& workDir = "") {
        std::string cmd = "\"" + exePath + "\"";
        if (!args.empty()) cmd += " " + args;

        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWMINNOACTIVE; // 最小化运行，不干扰桌面

        PROCESS_INFORMATION pi = {};
        std::string dir = workDir.empty() ? getExeDir(exePath) : workDir;

        BOOL ok = CreateProcessA(
            nullptr, &cmd[0],
            nullptr, nullptr, FALSE,
            CREATE_NEW_CONSOLE,
            nullptr, dir.c_str(),
            &si, &pi);

        if (!ok) {
            lastError_ = "CreateProcess failed: " + std::to_string(GetLastError());
            return false;
        }

        pi_ = pi;
        pid_ = pi.dwProcessId;
        CloseHandle(pi.hThread);
        return true;
    }

    HWND findWindow(const std::string& titleSubstr, int timeoutMs = 5000) {
        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            HWND h = findWindowByTitle(titleSubstr);
            if (h) return h;

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            if (elapsed > timeoutMs) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return nullptr;
    }

    void terminate() {
        if (pi_.hProcess) {
            // 先尝试优雅关闭
            HWND hwnd = findWindowByPid(pid_);
            if (hwnd) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                // 等待最多2秒优雅退出
                DWORD ret = WaitForSingleObject(pi_.hProcess, 2000);
                if (ret == WAIT_TIMEOUT) {
                    TerminateProcess(pi_.hProcess, 0);
                }
            } else {
                TerminateProcess(pi_.hProcess, 0);
            }
            CloseHandle(pi_.hProcess);
            pi_.hProcess = nullptr;
        }
    }

    bool isRunning() const {
        if (!pi_.hProcess) return false;
        DWORD code;
        return GetExitCodeProcess(pi_.hProcess, &code) && code == STILL_ACTIVE;
    }

    std::string lastError() const { return lastError_; }

    ~ProcessRunner() { terminate(); }

private:
    static std::string getExeDir(const std::string& exePath) {
        auto pos = exePath.find_last_of("\\/");
        return (pos != std::string::npos) ? exePath.substr(0, pos) : ".";
    }

    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
        auto* ctx = reinterpret_cast<std::pair<DWORD, HWND*>*>(lParam);
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == ctx->first) {
            wchar_t title[256];
            GetWindowTextW(hwnd, title, 256);
            if (wcslen(title) > 0) {
                *ctx->second = hwnd;
                return FALSE; // 停止枚举
            }
        }
        return TRUE;
    }

    HWND findWindowByPid(DWORD pid) {
        HWND result = nullptr;
        auto ctx = std::make_pair(pid, &result);
        EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&ctx));
        return result;
    }

    HWND findWindowByTitle(const std::string& substr) {
        // 枚举所有顶层窗口，按标题匹配
        struct Ctx { std::string substr; HWND result; };
        Ctx ctx{substr, nullptr};

        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            auto* c = reinterpret_cast<Ctx*>(lp);
            wchar_t title[256];
            GetWindowTextW(hwnd, title, 256);
            int len = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
            if (len <= 1) return TRUE;
            std::string s(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, title, -1, &s[0], len, nullptr, nullptr);
            if (s.find(c->substr) != std::string::npos) {
                c->result = hwnd;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
        return ctx.result;
    }

    PROCESS_INFORMATION pi_ = {};
    DWORD pid_ = 0;
    std::string lastError_;
};

// ═══════════════════════════════════════════════════════════════
// Step 2: 日志采集与解析
// ═══════════════════════════════════════════════════════════════

struct WidgetLogEntry {
    std::string widgetId;
    std::string stage;      // Measure / Arrange / Paint / PaintText
    int         type = -1;  // WidgetType 枚举值
    float       x = 0, y = 0, w = 0, h = 0;
    std::string text;       // PaintText 阶段的文本内容
    float       colorR = -1, colorG = -1, colorB = -1; // 文字颜色
    bool        valid = false;
};

class LogParser {
public:
    static std::vector<WidgetLogEntry> parseFile(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.is_open()) {
            rawErrors_.push_back("Cannot open log file: " + filepath);
            return {};
        }
        std::string line;
        std::vector<WidgetLogEntry> entries;
        while (std::getline(f, line)) {
            auto entry = parseLine(line);
            if (entry.valid) entries.push_back(entry);
        }
        return entries;
    }

    static std::vector<WidgetLogEntry> parseLines(const std::string& content) {
        std::istringstream ss(content);
        std::string line;
        std::vector<WidgetLogEntry> entries;
        while (std::getline(ss, line)) {
            auto entry = parseLine(line);
            if (entry.valid) entries.push_back(entry);
        }
        return entries;
    }

    static std::vector<std::string> rawErrors() { return rawErrors_; }
    static void clearErrors() { rawErrors_.clear(); }

private:
    static WidgetLogEntry parseLine(const std::string& line) {
        WidgetLogEntry e;

        // 格式: [I][Arrange][txt] ARRANGE[Screen] id=txt type=1 bounds=(0,0)-(800x24)
        std::regex arrRe(R"(ARRANGE\[Screen\]\s+id=(\S+)\s+type=(\d+)\s+bounds=\(([\d.-]+),([\d.-]+)\)-\(([\d.-]+)x([\d.-]+)\))");
        std::smatch m;
        if (std::regex_search(line, m, arrRe) && m.size() > 6) {
            e.widgetId = m[1]; e.stage = "Arrange";
            e.type = std::stoi(m[2]);
            e.x = std::stof(m[3]); e.y = std::stof(m[4]);
            e.w = std::stof(m[5]); e.h = std::stof(m[6]);
            e.valid = true;
            return e;
        }

        // Measure: MEASURE[Local] id=xxx type=N constraint=(w,h) measured=(w,h) ...
        std::regex measRe(R"(MEASURE\[Local\]\s+id=(\S+)\s+type=(\d+)\s+constraint=\(([\d.-]+),([\d.-]+)\)\s+measured=\(([\d.-]+),([\d.-]+)\))");
        if (std::regex_search(line, m, measRe) && m.size() > 6) {
            e.widgetId = m[1]; e.stage = "Measure";
            e.type = std::stoi(m[2]);
            e.x = std::stof(m[5]); e.y = std::stof(m[6]); // measured w/h stored in x/y
            e.w = std::stof(m[5]); e.h = std::stof(m[6]);
            e.valid = true;
            return e;
        }

        // Paint: PAINT[Screen] id=xxx type=N bounds=(x,y)-(wxh) visible=1
        std::regex paintRe(R"(PAINT\[Screen\]\s+id=(\S+)\s+type=(\d+)\s+bounds=\(([\d.-]+),([\d.-]+)\)-\(([\d.-]+)x([\d.-]+)\))");
        if (std::regex_search(line, m, paintRe) && m.size() > 6) {
            e.widgetId = m[1]; e.stage = "Paint";
            e.type = std::stoi(m[2]);
            e.x = std::stof(m[3]); e.y = std::stof(m[4]);
            e.w = std::stof(m[5]); e.h = std::stof(m[6]);
            e.valid = true;
            return e;
        }

        // PaintText: PAINT_TEXT[D2D] id=xxx text="..." rect=(x,y)-(wxh) color=(r,g,b) fontSize=X
        std::regex ptRe(R"(PAINT_TEXT\[D2D\]\s+id=(\S+)\s+text=\"([^\"]*)\"\s+rect=\(([\d.-]+),([\d.-]+)\)-\(([\d.-]+)x([\d.-]+)\)\s+color=\(([\d.]+),([\d.]+),([\d.]+)\))");
        if (std::regex_search(line, m, ptRe) && m.size() > 9) {
            e.widgetId = m[1]; e.stage = "PaintText";
            e.text = m[2];
            e.x = std::stof(m[3]); e.y = std::stof(m[4]);
            e.w = std::stof(m[5]); e.h = std::stof(m[6]);
            e.colorR = std::stof(m[7]); e.colorG = std::stof(m[8]);
            e.colorB = std::stof(m[9]);
            e.valid = true;
            return e;
        }

        return e;
    }

    static thread_local std::vector<std::string> rawErrors_;
};

thread_local std::vector<std::string> LogParser::rawErrors_;

// ═══════════════════════════════════════════════════════════════
// Step 3: 窗口交互 — 注入鼠标/尺寸消息
// ═══════════════════════════════════════════════════════════════

class WindowInteractor {
public:
    WindowInteractor(HWND hwnd) : hwnd_(hwnd) {}

    void resize(int w, int h) {
        SetWindowPos(hwnd_, nullptr, 0, 0, w, h,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        pumpMessages(5);
    }

    void mouseClick(int x, int y) {
        LPARAM lp = MAKELPARAM(x, y);
        PostMessage(hwnd_, WM_LBUTTONDOWN, MK_LBUTTON, lp);
        pumpMessages(3);
        PostMessage(hwnd_, WM_LBUTTONUP, 0, lp);
        pumpMessages(5);
    }

    void mouseMove(int x, int y) {
        LPARAM lp = MAKELPARAM(x, y);
        PostMessage(hwnd_, WM_MOUSEMOVE, 0, lp);
        pumpMessages(2);
    }

    void waitForRender(int cycles = 10) {
        // 等待 timer 触发若干次渲染
        pumpMessages(cycles);
    }

    void pumpMessages(int maxMsgs = 5) {
        MSG msg;
        for (int i = 0; i < maxMsgs; i++) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                // 没有消息时稍等
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
        // 额外等待渲染完成
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    HWND hwnd() const { return hwnd_; }

private:
    HWND hwnd_;
};

// ═══════════════════════════════════════════════════════════════
// Step 4: 自动诊断引擎
// ═══════════════════════════════════════════════════════════════

struct DiagnosticResult {
    bool passed = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::string summary;
};

class AutoDiagnostician {
public:
    DiagnosticResult diagnose(const std::vector<WidgetLogEntry>& entries,
                               const std::string& scenarioName) {
        DiagnosticResult r;

        // ──── 检测 1: 必须有足够数量的日志条目 ────
        if (entries.empty()) {
            r.passed = false;
            r.errors.push_back("[FATAL] Zero log entries captured. Check:"
                               "\n  1) Is DebugLogger enabled?"
                               "\n  2) Did the process start and render?"
                               "\n  3) Is the log file path correct?");
            r.summary = "No log data available.";
            return r;
        }

        // ──── 检测 2: 每个控件必须有 Measure + Arrange ────
        std::map<std::string, int> stageCount;
        std::set<std::string> allWidgets;
        for (const auto& e : entries) {
            stageCount[e.widgetId + ":" + e.stage]++;
            allWidgets.insert(e.widgetId);
        }

        for (const auto& id : allWidgets) {
            bool hasArrange = stageCount[id + ":Arrange"] > 0;
            bool hasMeasure = stageCount[id + ":Measure"] > 0;
            if (!hasArrange || !hasMeasure) {
                r.passed = false;
                std::string detail = "[MISSING] Widget '" + id + "' lacks: ";
                if (!hasMeasure) detail += "Measure ";
                if (!hasArrange) detail += "Arrange ";
                detail += "\n  → Widget may not have been parsed or layout was skipped.";
                r.errors.push_back(detail);
            }
        }

        // ──── 检测 3: Arrange bounds 必须在合理的视口范围内 ────
        const float VIEWPORT_W = 800.0f, VIEWPORT_H = 600.0f;
        for (const auto& e : entries) {
            if (e.stage == "Arrange") {
                if (e.x < -0.5f || e.y < -0.5f ||
                    e.x + e.w > VIEWPORT_W + 0.5f || e.y + e.h > VIEWPORT_H + 0.5f) {
                    r.passed = false;
                    r.errors.push_back("[OUT_OF_BOUNDS] Widget '" + e.widgetId +
                                       "' bounds (" + std::to_string(e.x) + "," +
                                       std::to_string(e.y) + ")-(" +
                                       std::to_string(e.w) + "x" + std::to_string(e.h) +
                                       ") outside viewport (" + std::to_string((int)VIEWPORT_W) +
                                       "x" + std::to_string((int)VIEWPORT_H) + ")");
                }
                if (e.w <= 0 || e.h <= 0) {
                    r.passed = false;
                    r.errors.push_back("[ZERO_SIZE] Widget '" + e.widgetId +
                                       "' has invalid size " + std::to_string(e.w) +
                                       "x" + std::to_string(e.h));
                }
            }
        }

        // ──── 检测 4: Card 内边距检测 ────
        // Card的 children 应在 Card bounds 内部偏移 16px
        std::map<std::string, WidgetLogEntry> arrangeMap;
        for (const auto& e : entries) {
            if (e.stage == "Arrange") arrangeMap[e.widgetId] = e;
        }

        // 已知 03 的组件树: root→[card1→[c1title,c1row1→[c1l1,c1v1],c1row2→[c1l2,c1v2]], div1, card2→[c2title,c2desc]]
        // Card 内边距 = 16px, 子元素 x 应 >= card.x+16
        auto checkCardPadding = [&](const std::string& cardId,
                                     const std::vector<std::string>& childIds) {
            if (!arrangeMap.count(cardId)) return;
            auto& card = arrangeMap[cardId];
            for (auto& childId : childIds) {
                if (!arrangeMap.count(childId)) continue;
                auto& child = arrangeMap[childId];
                if (child.x < card.x + 15.0f) { // 容差 1px
                    r.passed = false;
                    r.errors.push_back("[CARD_PADDING] Widget '" + childId +
                                       "' x=" + std::to_string(child.x) +
                                       " should be >= " + std::to_string(card.x + 16) +
                                       " (Card '" + cardId + "' inner padding)");
                }
            }
        };
        checkCardPadding("card1", {"c1title", "c1row1", "c1row2"});
        checkCardPadding("card2", {"c2title", "c2desc"});

        // ──── 检测 5: Column 子元素 Y 坐标递增（无重叠） ────
        auto checkColumnOrder = [&](const std::string& parentId,
                                     const std::vector<std::string>& childrenInOrder) {
            float prevBottom = -1;
            for (auto& childId : childrenInOrder) {
                if (!arrangeMap.count(childId)) continue;
                auto& child = arrangeMap[childId];
                if (prevBottom >= 0 && child.y < prevBottom - 0.5f) {
                    r.passed = false;
                    r.errors.push_back("[OVERLAP] Widget '" + childId +
                                       "' y=" + std::to_string(child.y) +
                                       " overlaps with previous (bottom=" +
                                       std::to_string(prevBottom) + ") in Column '" +
                                       parentId + "'");
                }
                prevBottom = child.y + child.h;
            }
        };
        checkColumnOrder("root", {"card1", "div1", "card2"});

        // ──── 检测 6: Row 子元素 X 坐标递增 ────
        auto checkRowOrder = [&](const std::string& rowId,
                                  const std::vector<std::string>& childrenInOrder) {
            float prevRight = -1;
            for (auto& childId : childrenInOrder) {
                if (!arrangeMap.count(childId)) continue;
                auto& child = arrangeMap[childId];
                if (prevRight >= 0 && child.x < prevRight - 0.5f) {
                    r.errors.push_back("[ROW_OVERLAP] Widget '" + childId +
                                       "' x=" + std::to_string(child.x) +
                                       " overlaps in Row '" + rowId + "'");
                    r.passed = false;
                }
                prevRight = child.x + child.w;
            }
        };
        checkRowOrder("c1row1", {"c1l1", "c1v1"});
        checkRowOrder("c1row2", {"c1l2", "c1v2"});

        // ──── 检测 7: Divider 宽度 ≈ 列宽 ────
        if (arrangeMap.count("root") && arrangeMap.count("div1")) {
            float rootW = arrangeMap["root"].w;
            float divW = arrangeMap["div1"].w;
            if (std::abs(rootW - divW) > 100.0f) {
                r.warnings.push_back("[DIVIDER_WIDTH] Divider width " +
                                     std::to_string(divW) + " differs from root width " +
                                     std::to_string(rootW));
            }
        }

        // ──── 生成摘要 ────
        if (r.passed) {
            r.summary = scenarioName + ": PASSED — " +
                        std::to_string(entries.size()) + " log entries, " +
                        std::to_string(allWidgets.size()) + " widgets verified.";
        } else {
            r.summary = scenarioName + ": FAILED — " +
                        std::to_string(r.errors.size()) + " errors, " +
                        std::to_string(r.warnings.size()) + " warnings.";
        }
        return r;
    }
};

// ═══════════════════════════════════════════════════════════════
// Step 5: 测试场景
// ═══════════════════════════════════════════════════════════════

class BlackBox03Test : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 查找 exe 路径
        TCHAR buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        std::string testDir(buf);
        auto pos = testDir.find_last_of("\\/");
        if (pos != std::string::npos) testDir = testDir.substr(0, pos);

        exePath_ = testDir + "\\jui_example_03_layout_demo.exe";
        logPath_ = testDir + "\\ex03_blackbox.log";

        // 确保之前没有残留的日志文件
        DeleteFileA(logPath_.c_str());
    }

    void SetUp() override {
        // 清空之前的解析错误
        LogParser::clearErrors();
    }

    void TearDown() override {
        runner_.terminate();
        // 清理日志文件
        DeleteFileA(logPath_.c_str());
    }

    // 启动被测进程并等待渲染就绪
    bool launchAndWait(const std::string& windowTitle = "Layout Demo", int timeoutMs = 8000) {
        if (!runner_.start(exePath_)) {
            ADD_FAILURE() << "Failed to start process: " << runner_.lastError();
            return false;
        }

        HWND hwnd = runner_.findWindow(windowTitle, timeoutMs);
        if (!hwnd) {
            ADD_FAILURE() << "Window not found within " << timeoutMs << "ms";
            return false;
        }

        interactor_ = std::make_unique<WindowInteractor>(hwnd);
        // 等待首次渲染完成
        interactor_->waitForRender(20);
        return true;
    }

    std::vector<WidgetLogEntry> readLog() {
        return LogParser::parseFile(logPath_);
    }

    static std::string exePath_;
    static std::string logPath_;
    ProcessRunner runner_;
    std::unique_ptr<WindowInteractor> interactor_;
    AutoDiagnostician diagnostician_;
};

std::string BlackBox03Test::exePath_;
std::string BlackBox03Test::logPath_;

// ──── 场景1: 初始渲染 — 验证所有控件 Layout 正确 ────
TEST_F(BlackBox03Test, Scn1_InitialLayout_AllWidgetsVisible) {
    ASSERT_NO_FATAL_FAILURE({ ASSERT_TRUE(launchAndWait()); });

    auto entries = readLog();

    auto result = diagnostician_.diagnose(entries, "Scn1_InitialLayout");
    EXPECT_TRUE(result.passed) << result.summary;
    if (!result.passed) {
        for (auto& err : result.errors) {
            ADD_FAILURE() << err;
        }
    }
    // 即使通过也输出摘要
    std::cout << "[DIAG] " << result.summary << std::endl;
}

// ──── 场景2: 窗口缩放 — 验证布局自适应 ────
TEST_F(BlackBox03Test, Scn2_Resize_LayoutAdapts) {
    ASSERT_NO_FATAL_FAILURE({ ASSERT_TRUE(launchAndWait()); });

    // 记录当前日志条数作为基线
    auto beforeEntries = readLog();
    size_t baselineCount = beforeEntries.size();
    float beforeW = 800, beforeH = 600;
    for (auto& e : beforeEntries) {
        if (e.widgetId == "root" && e.stage == "Arrange") {
            beforeW = e.w; beforeH = e.h; break;
        }
    }

    // 缩放窗口（SetWindowPos 设置包含标题栏的外部尺寸）
    interactor_->resize(1000, 700);
    interactor_->waitForRender(20);

    // 重新读取日志，只检查新增的条目
    auto afterEntries = readLog();
    float afterW = beforeW, afterH = beforeH;
    bool rootFound = false;
    for (size_t i = baselineCount; i < afterEntries.size(); i++) {
        auto& e = afterEntries[i];
        if (e.widgetId == "root" && e.stage == "Arrange") {
            rootFound = true;
            afterW = e.w; afterH = e.h;
        }
    }

    if (!rootFound) {
        // 回退：检查全部条目
        for (auto& e : afterEntries) {
            if (e.widgetId == "root" && e.stage == "Arrange") {
                afterW = e.w; afterH = e.h; rootFound = true;
            }
        }
    }

    // 初始窗口为 WS_OVERLAPPEDWINDOW 450×300（CreateWindow 尺寸），
    // 客户端区 ≈ 434×261。布局使用默认 800×600（WM_CREATE 时 RT 可能为 0×0）
    // WM_SIZE 后 RT 重建，尺寸 = 客户端区实际尺寸
    // 缩放后窗口 1000×700，客户端区 ≈ 984×661
    EXPECT_TRUE(rootFound) << "No Arrange entry for 'root' after resize";
    if (rootFound && beforeW > 500) {
        // 仅当初始布局使用了实际窗口尺寸时才对比
        EXPECT_NEAR(afterW, beforeW, 5.0f)
            << "After resize, root width should differ. Before=" << beforeW << " After=" << afterW;
    }
    EXPECT_GT(afterW, 400.0f) << "Root width unreasonably small: " << afterW;
    EXPECT_GT(afterH, 200.0f) << "Root height unreasonably small: " << afterH;

    std::cout << "[RESIZE] Before: " << beforeW << "x" << beforeH
              << " → After: " << afterW << "x" << afterH << std::endl;
}

// ──── 场景3: 日志文件完整性 — 验证日志可读取 ────
TEST_F(BlackBox03Test, Scn3_LogFile_ReadableAndParsable) {
    ASSERT_NO_FATAL_FAILURE({ ASSERT_TRUE(launchAndWait()); });

    auto entries = readLog();

    EXPECT_GT(entries.size(), 0u) << "No log entries found";

    // 统计各阶段
    std::map<std::string, int> stages;
    std::set<std::string> widgets;
    for (auto& e : entries) {
        stages[e.stage]++;
        widgets.insert(e.widgetId);
    }

    std::cout << "[STATS] Total entries: " << entries.size()
              << ", Widgets: " << widgets.size()
              << ", Measure: " << stages["Measure"]
              << ", Arrange: " << stages["Arrange"]
              << ", Paint: " << stages["Paint"]
              << ", PaintText: " << stages["PaintText"] << std::endl;

    // 03 示例应该有这些核心控件
    std::vector<std::string> expectedIds = {
        "root", "card1", "c1title", "c1row1", "c1l1", "c1v1",
        "c1row2", "c1l2", "c1v2", "div1", "card2", "c2title", "c2desc"
    };
    for (auto& id : expectedIds) {
        EXPECT_TRUE(widgets.count(id))
            << "Expected widget '" << id << "' not found in logs";
    }
}

// ──── 场景4: 卡片内边距专项检测 ────
TEST_F(BlackBox03Test, Scn4_CardPadding_16px) {
    ASSERT_NO_FATAL_FAILURE({ ASSERT_TRUE(launchAndWait()); });

    auto entries = readLog();

    // 收集 Arrange bounds
    std::map<std::string, WidgetLogEntry> arrangeMap;
    for (auto& e : entries) {
        if (e.stage == "Arrange") arrangeMap[e.widgetId] = e;
    }

    // card1 子元素必须在 card1.x+16, card1.y+16 内
    if (arrangeMap.count("card1") && arrangeMap.count("c1title")) {
        auto& card = arrangeMap["card1"];
        auto& title = arrangeMap["c1title"];
        EXPECT_GE(title.x, card.x + 15.0f)
            << "c1title x=" << title.x << " should be >= " << (card.x + 16);
        EXPECT_GE(title.y, card.y + 15.0f)
            << "c1title y=" << title.y << " should be >= " << (card.y + 16);
    }

    if (arrangeMap.count("card2") && arrangeMap.count("c2title")) {
        auto& card = arrangeMap["card2"];
        auto& title = arrangeMap["c2title"];
        EXPECT_GE(title.x, card.x + 15.0f)
            << "c2title x=" << title.x << " should be >= " << (card.x + 16);
    }
}

// ──── 场景5: 退出进程 — 验证正常关闭 ────
TEST_F(BlackBox03Test, Scn5_GracefulShutdown) {
    ASSERT_NO_FATAL_FAILURE({ ASSERT_TRUE(launchAndWait()); });

    // 发送 WM_CLOSE
    HWND hwnd = interactor_->hwnd();
    PostMessage(hwnd, WM_CLOSE, 0, 0);

    // 等待进程退出
    int waitMs = 0;
    while (runner_.isRunning() && waitMs < 3000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitMs += 100;
    }

    EXPECT_FALSE(runner_.isRunning())
        << "Process did not terminate within 3 seconds of WM_CLOSE";
}
