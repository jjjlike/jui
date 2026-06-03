/**
 * @file test_debug_logger.cpp
 * @brief 带日志的自动化测试 — 验证控件在不同阶段的区域一致性
 *
 * 测试策略（针对无 D2D 上下文环境）：
 *   1. 开启 DebugLogger，记录 Layout 阶段（Measure + Arrange）所有控件的位置和尺寸
 *   2. 用 Engine API 直接读取 Widget 的 layoutBounds 做交叉验证
 *   3. 验证父子控件包含关系、视口边界、尺寸合理性
 *   4. 验证日志开关、过滤、导出功能
 *   5. 如 D2D 可用，额外验证 Paint 阶段与 Layout 一致性
 *
 * 重要发现：在有 HWND 的真实窗口环境下，TextRenderWidget::paint()
 * 会记录 PaintText 日志（含文本、颜色、绘制区域）。无 D2D 时，
 * render() 因无 renderTarget 提前返回，不产生 Paint 日志。
 */

#include <gtest/gtest.h>
#include "jui/test/test.h"
#include "jui/core/engine.h"
#include <cmath>
#include <algorithm>

using namespace jui;
using namespace jui::test;

// ──────────────────────────────────────────────
// Helper: 从日志中提取 bounds
// ──────────────────────────────────────────────
struct ParsedBounds {
    float x = 0, y = 0, w = 0, h = 0;
    bool valid = false;
};

static ParsedBounds parseBounds(const std::string& msg) {
    ParsedBounds b;
    // 格式: "bounds=(x,y)-(wxh)" 或 "rect=(x,y)-(wxh)"
    // 或 "measured=(w,h)" 等变体
    auto pos = msg.find("=(");
    if (pos == std::string::npos) return b;
    const char* p = msg.c_str() + pos + 2;
    char* end = nullptr;
    b.x = std::strtof(p, &end); if (*end == ',') p = end + 1;
    b.y = std::strtof(p, &end);
    // end 现在指向 ')' 或 '-' 后面的字符
    // 跳过 ")-(" 到达宽高部分
    p = end;
    while (*p && *p != '-') p++;
    if (*p == '-') p++;      // 跳过 '-'
    while (*p && *p != '(') p++;
    if (*p == '(') p++;      // 跳过 '('
    b.w = std::strtof(p, &end);
    if (*end == 'x') p = end + 1;
    else if (*end == ',') {
        // 可能是 "measured=(w,h)" 格式，取 w 和 h
        p = end + 1;
        b.h = std::strtof(p, nullptr);
        b.valid = (b.w > 0.5f && b.h > 0.5f);
        return b;
    }
    b.h = std::strtof(p, nullptr);
    b.valid = (b.w > 0.5f && b.h > 0.5f);
    return b;
}

static bool boundsEqual(const ParsedBounds& a, const ParsedBounds& b, float tol = 0.5f) {
    return std::abs(a.x - b.x) <= tol &&
           std::abs(a.y - b.y) <= tol &&
           std::abs(a.w - b.w) <= tol &&
           std::abs(a.h - b.h) <= tol;
}

// ──────────────────────────────────────────────
// Fixture
// ──────────────────────────────────────────────
class DebugLoggerTest : public ::testing::Test {
protected:
    JUIEngine engine;
    
    void SetUp() override {
        DebugLogConfig cfg;
        cfg.enabled = false;
        cfg.includeLocation = false;
        cfg.sink = [](const LogEntry&) {};
        DebugLogger::instance().setConfig(cfg);
    }
    
    void TearDown() override {
        DebugLogger::instance().enable(false);
        DebugLogger::instance().clearWidgetFilter();
        DebugLogger::instance().clearLogs();
    }

    void loadEx01UI() {
        engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        engine.processMessage(R"({
            "surfaceUpdate":{"surfaceId":"main","components":[
                {"id":"root","component":{"Column":{"children":{"explicitList":["txt"]}}}},
                {"id":"txt","component":{"Text":{"text":{"literalString":"Hello, JUI!"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"2266CC"}}}}
            ]}})");
        engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":800,"height":600}})");
    }

    void loadEx02UI() {
        engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        engine.processMessage(R"({
            "surfaceUpdate":{"surfaceId":"main","components":[
                {"id":"root","component":{"Column":{"children":{"explicitList":["title","btns"]}}}},
                {"id":"title","component":{"Text":{"text":{"literalString":"Click:"},"fontSize":{"literalNumber":16}}}},
                {"id":"btns","component":{"Row":{"children":{"explicitList":["btn1","btn2"]}}}},
                {"id":"btn1","component":{"Button":{"text":{"literalString":"BtnA"}}}},
                {"id":"btn2","component":{"Button":{"text":{"literalString":"Hello"}}}}
            ]}})");
        engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":800,"height":600}})");
    }
};

// ──────────────────────────────────────────────
// 场景 1: Layout 阶段日志 — txt 有 Measure + Arrange
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn1_LayoutLogs_TxtHasMeasureAndArrange) {
    DebugLogger::instance().enable(true);
    DebugLogger::instance().addWidgetFilter("txt");
    
    loadEx01UI();
    
    DebugLogger::instance().enable(false);
    
    auto logs = DebugLogger::instance().getLogs("txt");
    ASSERT_GE(logs.size(), 2u) 
        << "txt should have at least Measure+Arrange entries, got " << logs.size();
    
    // 验证至少有 Measure 和 Arrange 阶段日志
    bool hasMeasure = false, hasArrange = false;
    for (const auto& entry : logs) {
        if (entry.stage == "Measure") hasMeasure = true;
        if (entry.stage == "Arrange") hasArrange = true;
    }
    EXPECT_TRUE(hasMeasure) << "Missing Measure log for txt";
    EXPECT_TRUE(hasArrange) << "Missing Arrange log for txt";
}

// ──────────────────────────────────────────────
// 场景 2: Layout bounds 与 Engine API 交叉验证
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn2_LayoutBounds_CrossCheckWithAPI) {
    DebugLogger::instance().enable(true);
    DebugLogger::instance().addWidgetFilter("txt");
    
    loadEx01UI();
    
    DebugLogger::instance().enable(false);
    
    // 从日志读取 Arrange bounds
    ParsedBounds logBounds;
    for (const auto& entry : DebugLogger::instance().getLogs("txt")) {
        if (entry.stage == "Arrange") {
            logBounds = parseBounds(entry.message);
            break;
        }
    }
    ASSERT_TRUE(logBounds.valid) << "No valid Arrange bounds in log";
    
    // 从 Engine API 直接读取
    auto surface = engine.getSurface("main");
    ASSERT_NE(surface, nullptr);
    auto txt = surface->getWidget("txt");
    ASSERT_NE(txt, nullptr);
    auto& apiBounds = txt->layoutBounds();
    
    // 日志值与 API 值必须一致
    EXPECT_NEAR(logBounds.x, apiBounds.x, 0.5f);
    EXPECT_NEAR(logBounds.y, apiBounds.y, 0.5f);
    EXPECT_NEAR(logBounds.w, apiBounds.w, 0.5f);
    EXPECT_NEAR(logBounds.h, apiBounds.h, 0.5f);
    
    // 最终确认: 在视口内
    EXPECT_GE(logBounds.x, -0.5f);
    EXPECT_GE(logBounds.y, -0.5f);
    EXPECT_LE(logBounds.x + logBounds.w, 800.5f);
    EXPECT_LE(logBounds.y + logBounds.h, 600.5f);
    EXPECT_GT(logBounds.w, 0);
    EXPECT_GT(logBounds.h, 0);
}

// ──────────────────────────────────────────────
// 场景 3: 控件过滤 — txt 白名单工作正常
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn3_WidgetFilter_OnlyTxt) {
    DebugLogger::instance().enable(true);
    DebugLogger::instance().addWidgetFilter("txt");
    
    loadEx01UI();
    
    DebugLogger::instance().enable(false);
    
    // root 被过滤
    auto rootLogs = DebugLogger::instance().getLogs("root");
    EXPECT_EQ(rootLogs.size(), 0u);
    
    // txt 在白名单
    auto txtLogs = DebugLogger::instance().getLogs("txt");
    EXPECT_GT(txtLogs.size(), 0u);
    
    // 清除过滤后，root 重新可见
    DebugLogger::instance().clearWidgetFilter();
    DebugLogger::instance().enable(true);
    engine.processMessage(R"({"createSurface":{"surfaceId":"main2"}})");
    engine.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"main2","components":[
            {"id":"root2","component":{"Column":{"children":{"explicitList":["t2"]}}}},
            {"id":"t2","component":{"Text":{"text":{"literalString":"Hi"}}}}
        ]}})");
    engine.processMessage(R"({"beginRendering":{"surfaceId":"main2","root":"root2","width":400,"height":300}})");
    DebugLogger::instance().enable(false);
    
    auto root2Logs = DebugLogger::instance().getLogs("root2");
    EXPECT_GT(root2Logs.size(), 0u) << "root2 should be logged when filter is cleared";
}

// ──────────────────────────────────────────────
// 场景 4: 全局开关正确控制日志产出
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn4_GlobalSwitch_OnOff) {
    // 不开
    loadEx01UI();
    EXPECT_EQ(DebugLogger::instance().getLogs().size(), 0u);
    DebugLogger::instance().clearLogs();
    
    // 打开后重新触发 layout
    DebugLogger::instance().enable(true);
    engine.processMessage(R"({"createSurface":{"surfaceId":"on"}})");
    engine.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"on","components":[
            {"id":"r","component":{"Column":{"children":{"explicitList":["t"]}}}},
            {"id":"t","component":{"Text":{"text":{"literalString":"X"}}}}
        ]}})");
    engine.processMessage(R"({"beginRendering":{"surfaceId":"on","root":"r","width":400,"height":300}})");
    auto cntOn = DebugLogger::instance().getLogs().size();
    DebugLogger::instance().enable(false);
    EXPECT_GT(cntOn, 0u);
    DebugLogger::instance().clearLogs();
    
    // 关掉后再操作
    engine.processMessage(R"({"createSurface":{"surfaceId":"off"}})");
    engine.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"off","components":[
            {"id":"r2","component":{"Column":{"children":{"explicitList":["t2"]}}}},
            {"id":"t2","component":{"Text":{"text":{"literalString":"Y"}}}}
        ]}})");
    engine.processMessage(R"({"beginRendering":{"surfaceId":"off","root":"r2","width":400,"height":300}})");
    EXPECT_EQ(DebugLogger::instance().getLogs().size(), 0u);
}

// ──────────────────────────────────────────────
// 场景 5: 父子包含关系 — txt 在 root 内部
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn5_ParentChildBounds_Containment) {
    DebugLogger::instance().enable(true);  // 不过滤

    loadEx01UI();

    DebugLogger::instance().enable(false);

    ParsedBounds rootBounds, txtBounds;
    for (const auto& entry : DebugLogger::instance().getLogs()) {
        if (entry.stage == "Arrange") {
            auto b = parseBounds(entry.message);
            if (entry.widgetId == "root" && b.valid) rootBounds = b;
            if (entry.widgetId == "txt" && b.valid) txtBounds = b;
        }
    }

    ASSERT_TRUE(rootBounds.valid) << "root has no valid Arrange bounds";
    ASSERT_TRUE(txtBounds.valid) << "txt has no valid Arrange bounds";

    // txt 在 root 内部
    EXPECT_GE(txtBounds.x, rootBounds.x - 0.5f);
    EXPECT_GE(txtBounds.y, rootBounds.y - 0.5f);
    EXPECT_LE(txtBounds.x + txtBounds.w, rootBounds.x + rootBounds.w + 0.5f);
    EXPECT_LE(txtBounds.y + txtBounds.h, rootBounds.y + rootBounds.h + 0.5f);
}

// ──────────────────────────────────────────────
// 场景 6: 示例02 多个控件区域一致性
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn6_Ex02_MultiWidgetArrangeConsistency) {
    DebugLogger::instance().enable(true);

    loadEx02UI();

    DebugLogger::instance().enable(false);

    std::map<std::string, ParsedBounds> boundsMap;
    for (const auto& entry : DebugLogger::instance().getLogs()) {
        if (entry.stage == "Arrange") {
            auto b = parseBounds(entry.message);
            if (b.valid) boundsMap[entry.widgetId] = b;
        }
    }

    ASSERT_GE(boundsMap.size(), 3u) 
        << "Expected at least 3 widgets with Arrange logs, got " << boundsMap.size();

    // Row 的 btn1 和 btn2 应水平排布（x 递增）
    if (boundsMap.count("btn1") && boundsMap.count("btn2")) {
        EXPECT_LT(boundsMap["btn1"].x + boundsMap["btn1"].w, 
                  boundsMap["btn2"].x + 0.5f)
            << "btn1 and btn2 should not overlap horizontally";
    }

    // title 应在 btns 上方
    if (boundsMap.count("title") && boundsMap.count("btns")) {
        EXPECT_LE(boundsMap["title"].y + boundsMap["title"].h, 
                  boundsMap["btns"].y + 0.5f)
            << "title should be above btns";
    }
}

// ──────────────────────────────────────────────
// 场景 7: 导出日志到字符串
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn7_ExportToString) {
    DebugLogger::instance().enable(true);
    DebugLogger::instance().addWidgetFilter("txt");
    
    loadEx01UI();
    
    auto output = DebugLogger::instance().exportToString();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("txt") != std::string::npos);
    EXPECT_TRUE(output.find("Arrange") != std::string::npos || 
                output.find("Measure") != std::string::npos);
    
    DebugLogger::instance().enable(false);
}

// ──────────────────────────────────────────────
// 场景 8: VerifyBoundsConsistency API
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn8_VerifyBoundsConsistency_API) {
    EXPECT_TRUE(DebugLogger::instance().verifyBoundsConsistency("test",
        10.0f, 20.0f, 100.0f, 30.0f,
        10.0f, 20.0f, 100.0f, 30.0f));

    EXPECT_FALSE(DebugLogger::instance().verifyBoundsConsistency("test",
        10.0f, 20.0f, 100.0f, 30.0f,
        15.0f, 20.0f, 100.0f, 30.0f));
}

// ──────────────────────────────────────────────
// 场景 9: 所有 LogLevel 的过滤正确
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn9_LogLevelFilter) {
    DebugLogConfig cfg;
    cfg.enabled = true;
    cfg.minLevel = LogLevel::Warning;
    cfg.includeLocation = false;
    cfg.sink = [](const LogEntry&) {};
    DebugLogger::instance().setConfig(cfg);

    engine.processMessage(R"({"createSurface":{"surfaceId":"lvl"}})");
    engine.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"lvl","components":[
            {"id":"r","component":{"Column":{"children":{"explicitList":["t"]}}}},
            {"id":"t","component":{"Text":{"text":{"literalString":"Hi"}}}}
        ]}})");
    engine.processMessage(R"({"beginRendering":{"surfaceId":"lvl","root":"r","width":100,"height":50}})");

    auto allLogs = DebugLogger::instance().getLogs();
    // minLevel=Warning 应将 Debug 级别日志过滤掉
    for (const auto& entry : allLogs) {
        EXPECT_GE(static_cast<int>(entry.level), static_cast<int>(LogLevel::Warning))
            << "Debug/Info log should be filtered when minLevel=Warning: " << entry.message;
    }

    DebugLogger::instance().enable(false);
}

// ──────────────────────────────────────────────
// 场景 10: 清除日志
// ──────────────────────────────────────────────
TEST_F(DebugLoggerTest, Scn10_ClearLogs) {
    DebugLogger::instance().enable(true);
    loadEx01UI();
    EXPECT_GT(DebugLogger::instance().getLogs().size(), 0u);
    
    DebugLogger::instance().clearLogs();
    EXPECT_EQ(DebugLogger::instance().getLogs().size(), 0u);
    
    DebugLogger::instance().enable(false);
}
