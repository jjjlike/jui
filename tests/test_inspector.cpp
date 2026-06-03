/**
 * @file test_inspector.cpp
 * @brief UIInspector 功能验证 — JSON Schema 输出 + 断言模板
 *
 * 覆盖:
 *   - inspect() 返回有效 JSON
 *   - 控件列表完整（id/type/layout/children）
 *   - Layout/Paint 区域一致性
 *   - 控件树父子关系可重建
 *   - 基于 Schema 的通用的断言示例
 */

#include <gtest/gtest.h>
#include "jui/core/engine.h"
#include <json.hpp>
#include <set>
#include <map>

using namespace jui;
using json = nlohmann::json;

class InspectorTest : public ::testing::Test {
protected:
    JUIEngine engine;
    void SetUp() override {
        engine.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
        engine.processMessage(R"({
            "surfaceUpdate":{"surfaceId":"main","components":[
                {"id":"root","component":{"Column":{"children":{"explicitList":["title","btns"]}}}},
                {"id":"title","component":{"Text":{"text":{"literalString":"Hello"},"fontSize":{"literalNumber":16}}}},
                {"id":"btns","component":{"Row":{"children":{"explicitList":["btn1","btn2"]}}}},
                {"id":"btn1","component":{"Button":{"text":{"literalString":"A"},"action":"a"}}},
                {"id":"btn2","component":{"Button":{"text":{"literalString":"B"},"action":"b"}}}
            ]}})");
        engine.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":400,"height":300}})");
    }
};

TEST_F(InspectorTest, Inspect_ReturnsValidJson) {
    std::string jsonStr = engine.inspect("main");
    EXPECT_FALSE(jsonStr.empty());

    json j = json::parse(jsonStr);
    EXPECT_EQ(j["surfaceId"], "main");
    EXPECT_GT(j["timestamp"], 0);
}

TEST_F(InspectorTest, Inspect_AllWidgetsPresent) {
    auto j = json::parse(engine.inspect("main"));

    std::set<std::string> ids;
    for (auto& w : j["widgets"]) ids.insert(w["id"]);

    EXPECT_TRUE(ids.count("root"));
    EXPECT_TRUE(ids.count("title"));
    EXPECT_TRUE(ids.count("btns"));
    EXPECT_TRUE(ids.count("btn1"));
    EXPECT_TRUE(ids.count("btn2"));
    EXPECT_EQ(ids.size(), 5u);
}

TEST_F(InspectorTest, Inspect_WidgetTypesCorrect) {
    auto j = json::parse(engine.inspect("main"));

    std::map<std::string, std::string> types;
    for (auto& w : j["widgets"]) types[w["id"]] = w["type"];

    EXPECT_EQ(types["root"], "Column");
    EXPECT_EQ(types["title"], "Text");
    EXPECT_EQ(types["btns"], "Row");
    EXPECT_EQ(types["btn1"], "Button");
    EXPECT_EQ(types["btn2"], "Button");
}

TEST_F(InspectorTest, Inspect_LayoutBoundsPositive) {
    auto j = json::parse(engine.inspect("main"));

    for (auto& w : j["widgets"]) {
        auto& l = w["layout"];
        EXPECT_GE(l["x"], -1.0);
        EXPECT_GE(l["y"], -1.0);
        EXPECT_GT(l["w"], 0.0) << w["id"] << ": zero width";
        EXPECT_GT(l["h"], 0.0) << w["id"] << ": zero height";
    }
}

TEST_F(InspectorTest, Inspect_PaintMatchTrue) {
    auto j = json::parse(engine.inspect("main"));

    for (auto& w : j["widgets"]) {
        // 无D2D上下文时 paint==layout，paintMatch 应为 true
        EXPECT_TRUE(w["paintMatch"]) << w["id"] << ": paintMatch should be true";
    }
}

TEST_F(InspectorTest, Inspect_ChildrenTreeValid) {
    auto j = json::parse(engine.inspect("main"));

    // Build ID→widget map
    std::map<std::string, json> wmap;
    std::set<std::string> allIds;
    for (auto& w : j["widgets"]) {
        allIds.insert(w["id"].get<std::string>());
        wmap[w["id"]] = w;
    }

    // 每个 child ID 必须在 widget 列表中
    for (auto& w : j["widgets"]) {
        for (auto& c : w["children"]) {
            std::string cid = c;
            EXPECT_TRUE(allIds.count(cid))
                << w["id"] << " references missing child: " << cid;
        }
    }

    // root必须有子控件
    auto& root = wmap["root"];
    EXPECT_GE(root["children"].size(), 2u);

    // btns必须有btn1, btn2
    auto& btns = wmap["btns"];
    EXPECT_EQ(btns["children"].size(), 2u);
}

TEST_F(InspectorTest, Inspect_ViewportPositive) {
    auto j = json::parse(engine.inspect("main"));
    EXPECT_GT(j["viewport"]["w"], 0);
    EXPECT_GT(j["viewport"]["h"], 0);
}

TEST_F(InspectorTest, Inspect_DefaultSurface_CurrentActive) {
    // 不传 surfaceId → 使用当前激活 Surface
    std::string json1 = engine.inspect();
    std::string json2 = engine.inspect("main");

    // 时间戳不同 → 比较时排除 timestamp 字段
    auto j1 = json::parse(json1);
    auto j2 = json::parse(json2);
    EXPECT_EQ(j1["surfaceId"], j2["surfaceId"]);
    EXPECT_EQ(j1["widgets"].size(), j2["widgets"].size());
    EXPECT_EQ(j1["viewport"]["w"], j2["viewport"]["w"]);
}

TEST_F(InspectorTest, Inspect_InvalidSurface_EmptyArray) {
    auto j = json::parse(engine.inspect("nonexistent"));
    EXPECT_EQ(j["surfaceId"], "");
    EXPECT_TRUE(j["widgets"].empty());
}
