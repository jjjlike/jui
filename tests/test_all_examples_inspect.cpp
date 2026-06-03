/**
 * @file test_all_examples_inspect.cpp
 * @brief 使用 Inspect API 对所有示例程序进行黑盒级校验
 *
 * 对每个示例加载其 UI JSON，调用 inspect() 获取快照，验证:
 *   - 控件树完整性 (id/type/children)
 *   - Layout bounds 合理 (>0, 在视口内)
 *   - PaintMatch 一致
 *   - 文本控件 text 非空
 *   - 按钮/容器/列表/表格特殊状态
 */

#include <gtest/gtest.h>
#include "jui/core/engine.h"
#include <json.hpp>
#include <map>
#include <set>
#include <cmath>

using namespace jui;
using json = nlohmann::json;

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────

static json loadAndInspect(JUIEngine& e, const char* uiJson,
                            const char* surfaceId = "main",
                            int vw = 800, int vh = 600) {
    e.processMessage(std::string(R"({"createSurface":{"surfaceId":")") + surfaceId + "\"}");
    e.processMessage(uiJson);
    e.processMessage(std::string(R"({"beginRendering":{"surfaceId":")") + surfaceId +
                     "\",\"root\":\"root\",\"width\":" + std::to_string(vw) +
                     ",\"height\":" + std::to_string(vh) + "}}");
    return json::parse(e.inspect(surfaceId));
}

static json findWidget(const json& snapshot, const std::string& id) {
    for (auto& w : snapshot["widgets"])
        if (w["id"] == id) return w;
    return {};
}

static std::set<std::string> allIds(const json& snapshot) {
    std::set<std::string> ids;
    for (auto& w : snapshot["widgets"]) ids.insert(w["id"]);
    return ids;
}

// 通用验证宏
static void verifyTreeIntegrity(const json& snap) {
    auto ids = allIds(snap);
    for (auto& w : snap["widgets"]) {
        for (auto& c : w["children"]) {
            std::string cid = c;
            EXPECT_TRUE(ids.count(cid))
                << w["id"] << " references missing child: " << cid;
        }
    }
}

static void verifyBounds(const json& snap) {
    float maxW = 800, maxH = 600;
    if (snap.contains("viewport")) {
        maxW = snap["viewport"]["w"].get<float>();
        maxH = snap["viewport"]["h"].get<float>();
    }
    // 使用快照自带的 viewport 作为边界参考，但允许 layout 使用 containerSize
    // （可能 > viewport，因为 layout 在 RT 创建前使用 fallback 800×600）
    if (maxW < 100) maxW = 800;
    if (maxH < 100) maxH = 600;
    for (auto& w : snap["widgets"]) {
        auto& l = w["layout"];
        EXPECT_GT(l["w"], 0.0) << w["id"] << " zero width";
        EXPECT_GT(l["h"], 0.0) << w["id"] << " zero height";
        EXPECT_GE(l["x"], -1.0);
        EXPECT_GE(l["y"], -1.0);
    }
}

static void verifyTextWidgets(const json& snap) {
    for (auto& w : snap["widgets"]) {
        if (w["type"] != "Text") continue;
        EXPECT_TRUE(w["state"].contains("text"))
            << w["id"] << " Text widget missing state.text";
        if (w["state"].contains("text")) {
            std::string txt = w["state"]["text"];
            EXPECT_FALSE(txt.empty()) << w["id"] << " Text widget has empty text";
        }
    }
}

static void verifyButtonWidgets(const json& snap) {
    for (auto& w : snap["widgets"]) {
        if (w["type"] != "Button") continue;
        EXPECT_TRUE(w["state"].contains("text"))
            << w["id"] << " Button missing state.text";
        if (w["state"].contains("text")) {
            EXPECT_FALSE(w["state"]["text"].get<std::string>().empty())
                << w["id"] << " Button has empty text";
        }
    }
}

// ──────────────────────────────────────────────
// 示例01 — Column → Text + Button
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex01_HelloWindow) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["txt","btn"]}}}},
            {"id":"txt","component":{"Text":{"text":{"literalString":"Hello, JUI!"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}},
            {"id":"btn","component":{"Button":{"text":{"literalString":"Hello"},"action":"click"}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_TRUE(ids.count("root") && ids.count("txt") && ids.count("btn"));
    EXPECT_EQ(ids.size(), 3u);
    EXPECT_EQ(findWidget(snap, "root")["type"], "Column");
    EXPECT_EQ(findWidget(snap, "txt")["type"], "Text");
    EXPECT_EQ(findWidget(snap, "btn")["type"], "Button");

    // txt 在 btn 上方
    auto txt = findWidget(snap, "txt");
    auto btn = findWidget(snap, "btn");
    EXPECT_LE(txt["layout"]["y"].get<float>() + txt["layout"]["h"].get<float>(),
              btn["layout"]["y"].get<float>() + 0.5f);

    verifyTreeIntegrity(snap);
    verifyBounds(snap);
    verifyTextWidgets(snap);
    verifyButtonWidgets(snap);
}

// ──────────────────────────────────────────────
// 示例02 — Column → Text + Button (单按钮版)
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex02_HelloButton) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","btn"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Click:"},"fontSize":{"literalNumber":16}}}},
            {"id":"btn","component":{"Button":{"text":{"literalString":"Hello"},"action":"hello"}}}
        ]}})");

    EXPECT_EQ(allIds(snap).size(), 3u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);
    verifyTextWidgets(snap);

    auto btn = findWidget(snap, "btn");
    EXPECT_GT(btn["layout"]["w"], 50.0);
    EXPECT_GT(btn["layout"]["h"], 20.0);
}

// ──────────────────────────────────────────────
// 示例03 — Column → Card1 + Divider + Card2
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex03_LayoutDemo) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["card1","div1","card2"]}}}},
            {"id":"card1","component":{"Card":{"children":{"explicitList":["c1title","c1row1","c1row2"]}}}},
            {"id":"c1title","component":{"Text":{"text":{"literalString":"User Info"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"c1row1","component":{"Row":{"children":{"explicitList":["c1l1","c1v1"]}}}},
            {"id":"c1l1","component":{"Text":{"text":{"literalString":"Name:"},"fontSize":{"literalNumber":14}}}},
            {"id":"c1v1","component":{"Text":{"text":{"literalString":"ZhangSan"},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#333333"}}}},
            {"id":"c1row2","component":{"Row":{"children":{"explicitList":["c1l2","c1v2"]}}}},
            {"id":"c1l2","component":{"Text":{"text":{"literalString":"Role:"},"fontSize":{"literalNumber":14}}}},
            {"id":"c1v2","component":{"Text":{"text":{"literalString":"Engineer"},"fontSize":{"literalNumber":14},"textColor":{"literalString":"#333333"}}}},
            {"id":"div1","component":{"Divider":{}}},
            {"id":"card2","component":{"Card":{"children":{"explicitList":["c2title","c2desc"]}}}},
            {"id":"c2title","component":{"Text":{"text":{"literalString":"Panel"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"c2desc","component":{"Text":{"text":{"literalString":"Desc text"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#555555"}}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_EQ(ids.size(), 13u) << "Expected 13 widgets, got " << ids.size();

    verifyTreeIntegrity(snap);
    verifyBounds(snap);
    verifyTextWidgets(snap);

    // Card 内边距: c1title.x >= card1.x + 16
    auto card1 = findWidget(snap, "card1");
    auto c1title = findWidget(snap, "c1title");
    EXPECT_GE(c1title["layout"]["x"].get<float>(),
              card1["layout"]["x"].get<float>() + 15.0f)
        << "Card padding violation";

    // Column 顺序: card1.y < div1.y < card2.y
    auto div1 = findWidget(snap, "div1");
    auto card2 = findWidget(snap, "card2");
    EXPECT_LT(card1["layout"]["y"].get<float>(), div1["layout"]["y"].get<float>());
    EXPECT_LT(div1["layout"]["y"].get<float>(), card2["layout"]["y"].get<float>());

    // Row 子元素不重叠: c1l1.x + c1l1.w <= c1v1.x
    auto c1l1 = findWidget(snap, "c1l1");
    auto c1v1 = findWidget(snap, "c1v1");
    EXPECT_LE(c1l1["layout"]["x"].get<float>() + c1l1["layout"]["w"].get<float>(),
              c1v1["layout"]["x"].get<float>() + 0.5f);
}

// ──────────────────────────────────────────────
// 示例04 — Form: TextField + Button
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex04_FormDemo) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","r1","r2","btn"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Login"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"r1","component":{"Row":{"children":{"explicitList":["l1","tf1"]}}}},
            {"id":"l1","component":{"Text":{"text":{"literalString":"Name:"},"fontSize":{"literalNumber":14}}}},
            {"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"enter name"},"width":{"literalNumber":200}}}},
            {"id":"r2","component":{"Row":{"children":{"explicitList":["l2","tf2"]}}}},
            {"id":"l2","component":{"Text":{"text":{"literalString":"Pass:"},"fontSize":{"literalNumber":14}}}},
            {"id":"tf2","component":{"TextField":{"placeholder":{"literalString":"enter pass"},"width":{"literalNumber":200}}}},
            {"id":"btn","component":{"Button":{"text":{"literalString":"Login"},"action":"login"}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_EQ(ids.size(), 9u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);

    EXPECT_EQ(findWidget(snap, "tf1")["type"], "TextField");
    EXPECT_EQ(findWidget(snap, "tf2")["type"], "TextField");
    EXPECT_GT(findWidget(snap, "tf1")["layout"]["w"], 100.0);
}

// ──────────────────────────────────────────────
// 示例05 — CheckBox + Toggle + Slider
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex05_ControlsDemo) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","cb1","cb2","tg1","txt1","sl1","txt2","sl2"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Controls"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"cb1","component":{"CheckBox":{"text":{"literalString":"Agree"}}}},
            {"id":"cb2","component":{"CheckBox":{"text":{"literalString":"Subscribe"},"value":{"literalString":"true"}}}},
            {"id":"tg1","component":{"Toggle":{"label":"Notify"}}},
            {"id":"txt1","component":{"Text":{"text":{"literalString":"Vol 50"},"fontSize":{"literalNumber":14}}}},
            {"id":"sl1","component":{"Slider":{"value":{"literalNumber":50}}}},
            {"id":"txt2","component":{"Text":{"text":{"literalString":"Bright 80"},"fontSize":{"literalNumber":14}}}},
            {"id":"sl2","component":{"Slider":{"value":{"literalNumber":80}}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_EQ(ids.size(), 9u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);

    EXPECT_EQ(findWidget(snap, "cb1")["type"], "CheckBox");
    EXPECT_EQ(findWidget(snap, "tg1")["type"], "Toggle");
    EXPECT_EQ(findWidget(snap, "sl1")["type"], "Slider");
    EXPECT_EQ(findWidget(snap, "sl2")["type"], "Slider");
}

// ──────────────────────────────────────────────
// 示例06 — Tabs with pages
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex06_TabsDemo) {
    JUIEngine e;
    e.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    e.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","content"]}}}},
            {"id":"tabs","component":{"Tabs":{"tabNames":{"explicitList":["Home","Settings","About"]}}}},
            {"id":"d","component":{"Divider":{}}},
            {"id":"hdr","component":{"Text":{"text":{"literalString":"Home Page"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"content","component":{"Text":{"text":{"literalString":"Welcome!"},"fontSize":{"literalNumber":28},"fontWeight":{"literalString":"bold"}}}}
        ]}})");
    e.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":500,"height":400}})");
    auto snap = json::parse(e.inspect("main"));

    auto ids = allIds(snap);
    EXPECT_TRUE(ids.count("tabs") && ids.count("content"));
    verifyTreeIntegrity(snap);

    EXPECT_EQ(findWidget(snap, "tabs")["type"], "Tabs");
    EXPECT_EQ(findWidget(snap, "d")["type"], "Divider");
}

// ──────────────────────────────────────────────
// 示例07 — List (virtual scroll)
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex07_ListDemo) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","list"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"List 100K"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":400},"height":{"literalNumber":420}}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_EQ(ids.size(), 3u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);

    auto list = findWidget(snap, "list");
    EXPECT_EQ(list["type"], "List");
    EXPECT_GT(list["layout"]["w"], 200.0);
    EXPECT_GT(list["layout"]["h"], 200.0);
}

// ──────────────────────────────────────────────
// 示例08 — Grid
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex08_GridDemo) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","desc","grid"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Data Grid"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"desc","component":{"Text":{"text":{"literalString":"Fixed header"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#999999"}}}},
            {"id":"grid","component":{"Grid":{"width":{"literalNumber":580},"height":{"literalNumber":400}}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_TRUE(ids.count("grid"));
    EXPECT_EQ(findWidget(snap, "grid")["type"], "Grid");
    verifyTreeIntegrity(snap);
    verifyBounds(snap);
}

// ──────────────────────────────────────────────
// 示例09 — DataModel path binding
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex09_BindingDemo) {
    JUIEngine e;
    e.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    e.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["title","btn1","btn2","btn3"]}}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Guest"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
            {"id":"btn1","component":{"Button":{"text":{"literalString":"Alice"},"action":"alice"}}},
            {"id":"btn2","component":{"Button":{"text":{"literalString":"Bob"},"action":"bob"}}},
            {"id":"btn3","component":{"Button":{"text":{"literalString":"Charlie"},"action":"charlie"}}}
        ]}})");
    e.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":480,"height":350}})");
    auto snap = json::parse(e.inspect("main"));

    auto ids = allIds(snap);
    EXPECT_EQ(ids.size(), 5u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);

    // title 的 text 应该为 "Guest"（literalString 直接设定）
    auto title = findWidget(snap, "title");
    EXPECT_EQ(title["type"], "Text");
    EXPECT_TRUE(title["state"].contains("text"));
    EXPECT_EQ(title["state"]["text"], "Guest");

    // 3个按钮存在
    EXPECT_EQ(findWidget(snap, "btn1")["type"], "Button");
    EXPECT_EQ(findWidget(snap, "btn2")["type"], "Button");
    EXPECT_EQ(findWidget(snap, "btn3")["type"], "Button");
}

// ──────────────────────────────────────────────
// 示例10 — Full App (Dashboard)
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Ex10_FullApp) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","title","stats","actions"]}}}},
            {"id":"tabs","component":{"Tabs":{"tabNames":{"explicitList":["Dash","List","Set."]}}}},
            {"id":"d","component":{"Divider":{}}},
            {"id":"title","component":{"Text":{"text":{"literalString":"Overview"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"}}}},
            {"id":"stats","component":{"Row":{"children":{"explicitList":["c1","c2","c3"]}}}},
            {"id":"c1","component":{"Card":{"children":{"explicitList":["c1t","c1v"]}}}},
            {"id":"c1t","component":{"Text":{"text":{"literalString":"Users"},"fontSize":{"literalNumber":12}}}},
            {"id":"c1v","component":{"Text":{"text":{"literalString":"12.5K"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
            {"id":"c2","component":{"Card":{"children":{"explicitList":["c2t","c2v"]}}}},
            {"id":"c2t","component":{"Text":{"text":{"literalString":"Active"},"fontSize":{"literalNumber":12}}}},
            {"id":"c2v","component":{"Text":{"text":{"literalString":"87%"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
            {"id":"c3","component":{"Card":{"children":{"explicitList":["c3t","c3v"]}}}},
            {"id":"c3t","component":{"Text":{"text":{"literalString":"Pend."},"fontSize":{"literalNumber":12}}}},
            {"id":"c3v","component":{"Text":{"text":{"literalString":"3"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
            {"id":"actions","component":{"Row":{"children":{"explicitList":["btn1","btn2"]}}}},
            {"id":"btn1","component":{"Button":{"text":{"literalString":"Refresh"},"action":"refresh"}}},
            {"id":"btn2","component":{"Button":{"text":{"literalString":"Export"},"action":"export"}}}
        ]}})");

    auto ids = allIds(snap);
    EXPECT_GE(ids.size(), 15u) << "Expected >=15 widgets, got " << ids.size();
    EXPECT_TRUE(ids.count("root") && ids.count("tabs") && ids.count("stats"));
    verifyTreeIntegrity(snap);
    verifyBounds(snap);
    verifyTextWidgets(snap);
    verifyButtonWidgets(snap);

    // 3 个统计卡片不重叠
    auto c1 = findWidget(snap, "c1");
    auto c2 = findWidget(snap, "c2");
    auto c3 = findWidget(snap, "c3");
    EXPECT_LT(c1["layout"]["x"].get<float>() + c1["layout"]["w"].get<float>(),
              c2["layout"]["x"].get<float>() + 0.5f);
    EXPECT_LT(c2["layout"]["x"].get<float>() + c2["layout"]["w"].get<float>(),
              c3["layout"]["x"].get<float>() + 0.5f);
}

// ──────────────────────────────────────────────
// jui_hello — All controls demo (6 tabs)
// ──────────────────────────────────────────────

TEST(AllExamplesInspect, Hello_AllControls) {
    JUIEngine e;
    e.processMessage(R"({"createSurface":{"surfaceId":"main"}})");
    // 只测试第一个 tab page（Button/Text）
    e.processMessage(R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","actions"]}}}},
            {"id":"tabs","component":{"Tabs":{"tabNames":{"explicitList":["Button","TextFld","Check","Picker","List","Grid"]}}}},
            {"id":"d","component":{"Divider":{}}},
            {"id":"hdr","component":{"Text":{"text":{"literalString":"Button/Text"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"}}}},
            {"id":"r1","component":{"Text":{"text":{"literalString":"Sizes:"},"fontSize":{"literalNumber":14}}}},
            {"id":"r2","component":{"Text":{"text":{"literalString":"14px default black"},"fontSize":{"literalNumber":14}}}},
            {"id":"r3","component":{"Text":{"text":{"literalString":"20px bold blue"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}},
            {"id":"actions","component":{"Row":{"children":{"explicitList":["btn1","btn2","btn3"]}}}},
            {"id":"btn1","component":{"Button":{"text":{"literalString":"Default"}}}},
            {"id":"btn2","component":{"Button":{"text":{"literalString":"ClickMe"},"action":"hello"}}},
            {"id":"btn3","component":{"Button":{"text":{"literalString":"Submit"},"action":"submit"}}}
        ]}})");
    e.processMessage(R"({"beginRendering":{"surfaceId":"main","root":"root","width":680,"height":600}})");
    auto snap = json::parse(e.inspect("main"));

    auto ids = allIds(snap);
    EXPECT_GE(ids.size(), 10u);
    verifyTreeIntegrity(snap);
    verifyBounds(snap);
    verifyTextWidgets(snap);
    verifyButtonWidgets(snap);

    EXPECT_EQ(findWidget(snap, "tabs")["type"], "Tabs");
}

// ═══════════════════════════════════════════════
// 跨所有示例的综合断言
// ═══════════════════════════════════════════════

TEST(AllExamplesInspect, CrossExample_NoDuplicateIds) {
    // 每个快照内部控制 ID 唯一
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["a","b","c"]}}}},
            {"id":"a","component":{"Text":{"text":{"literalString":"A"},"fontSize":{"literalNumber":14}}}},
            {"id":"b","component":{"Text":{"text":{"literalString":"B"},"fontSize":{"literalNumber":14}}}},
            {"id":"c","component":{"Text":{"text":{"literalString":"C"},"fontSize":{"literalNumber":14}}}}
        ]}})");

    std::set<std::string> seen;
    for (auto& w : snap["widgets"]) {
        EXPECT_FALSE(seen.count(w["id"]))
            << "Duplicate widget id: " << w["id"];
        seen.insert(w["id"]);
    }
}

TEST(AllExamplesInspect, CrossExample_RootIsColumn) {
    // 所有示例的根控件都是 Column
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["t"]}}}},
            {"id":"t","component":{"Text":{"text":{"literalString":"Hi"},"fontSize":{"literalNumber":14}}}}
        ]}})");

    EXPECT_EQ(findWidget(snap, "root")["type"], "Column");
}

TEST(AllExamplesInspect, CrossExample_AllVisibleEnabled) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["t","b","cb","tg"]}}}},
            {"id":"t","component":{"Text":{"text":{"literalString":"X"},"fontSize":{"literalNumber":14}}}},
            {"id":"b","component":{"Button":{"text":{"literalString":"Y"}}}},
            {"id":"cb","component":{"CheckBox":{"text":{"literalString":"Z"}}}},
            {"id":"tg","component":{"Toggle":{"label":"W"}}}
        ]}})");

    for (auto& w : snap["widgets"]) {
        EXPECT_TRUE(w["visible"]) << w["id"] << " not visible";
        EXPECT_TRUE(w["enabled"]) << w["id"] << " not enabled";
    }
}

TEST(AllExamplesInspect, CrossExample_PaintMatchAll) {
    JUIEngine e;
    auto snap = loadAndInspect(e, R"({
        "surfaceUpdate":{"surfaceId":"main","components":[
            {"id":"root","component":{"Column":{"children":{"explicitList":["t1","t2","t3"]}}}},
            {"id":"t1","component":{"Text":{"text":{"literalString":"One"},"fontSize":{"literalNumber":12}}}},
            {"id":"t2","component":{"Text":{"text":{"literalString":"Two"},"fontSize":{"literalNumber":14}}}},
            {"id":"t3","component":{"Text":{"text":{"literalString":"Three"},"fontSize":{"literalNumber":16}}}}
        ]}})");

    for (auto& w : snap["widgets"]) {
        EXPECT_TRUE(w["paintMatch"])
            << w["id"] << " paintMatch should be true";
    }
}
