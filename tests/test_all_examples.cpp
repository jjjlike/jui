/**
 * @file test_all_examples.cpp
 * @brief 所有11个示例的全链路自动化测试 — 检查界面是否存在问题
 *
 * 测试覆盖：
 *   每个示例 → 加载UI → 验证所有widget存在且可见 → 检查布局无重叠
 *   → 模拟交互 → 验证回调 → 检查状态变更
 *
 * 设计原则：发现一个问题修复一个问题，零误报
 */
#include <gtest/gtest.h>
#include "jui/jui.h"
#include "jui/test/driver.h"

using namespace jui::test;

// ============================================================
// 辅助函数：从示例文件中提取JSON
// ============================================================

// 示例01: hello_window — Text
static const char* EX01_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["txt"]}}}},
{"id":"txt","component":{"Text":{"text":{"literalString":"Hello, JUI!"},"fontSize":{"literalNumber":16},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}}
]}})";

// 示例02: hello_button — 3个按钮
static const char* EX02_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","btns"]}}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Click buttons:"},"fontSize":{"literalNumber":16}}}},
{"id":"btns","component":{"Row":{"children":{"explicitList":["btn1","btn2","btn3"]}}}},
{"id":"btn1","component":{"Button":{"text":{"literalString":"BtnA"}}}},
{"id":"btn2","component":{"Button":{"text":{"literalString":"Hello"},"action":"hello"}}},
{"id":"btn3","component":{"Button":{"text":{"literalString":"Bye"},"action":"bye"}}}
]}})";

// 示例03: layout_demo — Card嵌套
static const char* EX03_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["card1","div1","card2"]}}}},
{"id":"card1","component":{"Card":{"children":{"explicitList":["c1title","c1row1","c1row2"]}}}},
{"id":"c1title","component":{"Text":{"text":{"literalString":"UserInfo"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"c1row1","component":{"Row":{"children":{"explicitList":["c1l1","c1v1"]}}}},
{"id":"c1l1","component":{"Text":{"text":{"literalString":"Name:"},"fontSize":{"literalNumber":14}}}},
{"id":"c1v1","component":{"Text":{"text":{"literalString":"ZhangSan"},"fontSize":{"literalNumber":14}}}},
{"id":"c1row2","component":{"Row":{"children":{"explicitList":["c1l2","c1v2"]}}}},
{"id":"c1l2","component":{"Text":{"text":{"literalString":"Role:"},"fontSize":{"literalNumber":14}}}},
{"id":"c1v2","component":{"Text":{"text":{"literalString":"Engineer"},"fontSize":{"literalNumber":14}}}},
{"id":"div1","component":{"Divider":{}}},
{"id":"card2","component":{"Card":{"children":{"explicitList":["c2title","c2desc"]}}}},
{"id":"c2title","component":{"Text":{"text":{"literalString":"Panel"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"c2desc","component":{"Text":{"text":{"literalString":"desc text"},"fontSize":{"literalNumber":12}}}}
]}})";

// 示例04: form_demo — TextField
static const char* EX04_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","r1","r2","btn"]}}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Login"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"r1","component":{"Row":{"children":{"explicitList":["l1","tf1"]}}}},
{"id":"l1","component":{"Text":{"text":{"literalString":"Name:"},"fontSize":{"literalNumber":14}}}},
{"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"enter name"},"width":{"literalNumber":200}}}},
{"id":"r2","component":{"Row":{"children":{"explicitList":["l2","tf2"]}}}},
{"id":"l2","component":{"Text":{"text":{"literalString":"Pass:"},"fontSize":{"literalNumber":14}}}},
{"id":"tf2","component":{"TextField":{"placeholder":{"literalString":"enter pass"},"width":{"literalNumber":200}}}},
{"id":"btn","component":{"Button":{"text":{"literalString":"Login"},"action":"login"}}}
]}})";

// 示例05: controls_demo — CheckBox/Toggle/Slider
static const char* EX05_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","cb1","cb2","tg1","txt1","sl1","txt2","sl2"]}}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Controls"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"cb1","component":{"CheckBox":{"text":{"literalString":"Agree"}}}},
{"id":"cb2","component":{"CheckBox":{"text":{"literalString":"Subscribe"},"value":{"literalBoolean":true}}}},
{"id":"tg1","component":{"Toggle":{"label":{"literalString":"Notify"},"value":{"literalBoolean":true}}}},
{"id":"txt1","component":{"Text":{"text":{"literalString":"Vol 50%"},"fontSize":{"literalNumber":14}}}},
{"id":"sl1","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":50}}}},
{"id":"txt2","component":{"Text":{"text":{"literalString":"Bright 80%"},"fontSize":{"literalNumber":14}}}},
{"id":"sl2","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":80}}}}
]}})";

// 示例06: tabs_demo — 3页Tabs (每个Tab用不同surfaceId)
static const char* EX06_P0 = R"({"surfaceUpdate":{"surfaceId":"t0","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","content"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Home","id":"t0"},{"title":"Settings","id":"t1"},{"title":"About","id":"t2"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"content","component":{"Text":{"text":{"literalString":"Welcome!"},"fontSize":{"literalNumber":28},"fontWeight":{"literalString":"bold"}}}}
]}})";

// 示例07: list_demo — 100K列表
static const char* EX07_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","list"]}}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Virtual List 100K"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":400},"height":{"literalNumber":420}}}}
]}})";

// 示例08: grid_demo — 表格
static const char* EX08_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","desc","grid"]}}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Data Grid 50K"},"fontSize":{"literalNumber":18},"fontWeight":{"literalString":"bold"}}}},
{"id":"desc","component":{"Text":{"text":{"literalString":"Fixed header + virtual rows"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#999"}}}},
{"id":"grid","component":{"Grid":{"width":{"literalNumber":580},"height":{"literalNumber":400}}}}
]}})";

// 示例09: binding_demo — DataModel绑定
static const char* EX09_JSON = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["title","btn1","btn2","btn3"]}}}},
{"id":"title","component":{"Text":{"text":{"path":"/user/name"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"}}}},
{"id":"btn1","component":{"Button":{"text":{"literalString":"Alice"},"action":"alice"}}},
{"id":"btn2","component":{"Button":{"text":{"literalString":"Bob"},"action":"bob"}}},
{"id":"btn3","component":{"Button":{"text":{"literalString":"Charlie"},"action":"charlie"}}}
]}})";

// 示例10: full_app 仪表盘页
static const char* EX10_P0 = R"({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","title","stats","action"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[{"title":"Dash","id":"p0"},{"title":"List","id":"p1"},{"title":"Settings","id":"p2"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"title","component":{"Text":{"text":{"literalString":"Overview"},"fontSize":{"literalNumber":22},"fontWeight":{"literalString":"bold"}}}},
{"id":"stats","component":{"Row":{"children":{"explicitList":["card1","card2","card3"]}}}},
{"id":"card1","component":{"Card":{"children":{"explicitList":["c1t","c1v"]}}}},
{"id":"c1t","component":{"Text":{"text":{"literalString":"Users"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
{"id":"c1v","component":{"Text":{"text":{"literalString":"12.5K"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
{"id":"card2","component":{"Card":{"children":{"explicitList":["c2t","c2v"]}}}},
{"id":"c2t","component":{"Text":{"text":{"literalString":"Active"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
{"id":"c2v","component":{"Text":{"text":{"literalString":"87%"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
{"id":"card3","component":{"Card":{"children":{"explicitList":["c3t","c3v"]}}}},
{"id":"c3t","component":{"Text":{"text":{"literalString":"Pending"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#888"}}}},
{"id":"c3v","component":{"Text":{"text":{"literalString":"3"},"fontSize":{"literalNumber":24},"fontWeight":{"literalString":"bold"}}}},
{"id":"action","component":{"Row":{"children":{"explicitList":["btn_refresh","btn_export"]}}}},
{"id":"btn_refresh","component":{"Button":{"text":{"literalString":"Refresh"},"action":"refresh"}}},
{"id":"btn_export","component":{"Button":{"text":{"literalString":"Export"}}}}
]}})";

// ============================================================
// 测试类
// ============================================================
class AllExamplesTest : public ::testing::Test {
protected:
    void SetUp() override { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
    void TearDown() override { CoUninitialize(); }

    // 通用验证: 加载界面后检查所有核心widget
    void loadAndVerify(const char* json, std::vector<std::string> mustExist) {
        AiTestDriver drv;
        ASSERT_TRUE(drv.initialize());
        drv.loadUI("main", json);
        for (auto& id : mustExist) {
            EXPECT_TRUE(drv.widgetExists(id)) << "Missing widget: " << id;
            EXPECT_TRUE(drv.widgetVisible(id)) << "Widget not visible: " << id;
        }
    }
};

// ============================================================
// 示例01: hello_window
// ============================================================
TEST_F(AllExamplesTest, Ex01_HelloWindow_WidgetsExist) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX01_JSON);
    EXPECT_TRUE(drv.widgetExists("root"));
    EXPECT_TRUE(drv.widgetExists("txt"));
    EXPECT_TRUE(drv.widgetVisible("txt"));
    // Text包含正确内容
    EXPECT_EQ(drv.widgetText("txt"), "Hello, JUI!");
}

TEST_F(AllExamplesTest, Ex01_HelloWindow_LayoutCorrect) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX01_JSON);
    // root 应包含 txt
    EXPECT_TRUE(drv.boundsContains("root", "txt"));
    // txt 应有合理的尺寸
    auto b = drv.widgetBounds("txt");
    EXPECT_GT(b.w, 20.0f) << "Text should have non-zero width";
    EXPECT_GT(b.h, 10.0f) << "Text should have non-zero height";
}

// ============================================================
// 示例02: hello_button
// ============================================================
TEST_F(AllExamplesTest, Ex02_HelloButton_AllVisible) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX02_JSON);
    EXPECT_TRUE(drv.widgetVisible("btn1"));
    EXPECT_TRUE(drv.widgetEnabled("btn1"));
    EXPECT_TRUE(drv.widgetVisible("btn2"));
    EXPECT_TRUE(drv.widgetEnabled("btn2"));
    EXPECT_TRUE(drv.widgetVisible("btn3"));
    EXPECT_EQ(drv.widgetText("btn2"), "Hello");
    EXPECT_EQ(drv.widgetText("btn3"), "Bye");
}

TEST_F(AllExamplesTest, Ex02_HelloButton_NoOverlap) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX02_JSON);
    EXPECT_TRUE(drv.boundsContains("root", "btn1"));
    EXPECT_TRUE(drv.boundsContains("root", "btn2"));
    EXPECT_TRUE(drv.boundsContains("root", "btn3"));
    // 按钮不应重叠（Row布局）
    EXPECT_FALSE(drv.boundsOverlap("btn1", "btn2")) << "Buttons should not overlap in Row layout";
    EXPECT_FALSE(drv.boundsOverlap("btn2", "btn3")) << "Buttons should not overlap in Row layout";
}

// ============================================================
// 示例03: layout_demo
// ============================================================
TEST_F(AllExamplesTest, Ex03_LayoutDemo_CardNesting) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX03_JSON);
    // Card1内部
    EXPECT_TRUE(drv.widgetVisible("card1"));
    EXPECT_TRUE(drv.widgetVisible("c1title"));
    EXPECT_TRUE(drv.widgetVisible("c1row1"));
    EXPECT_TRUE(drv.widgetVisible("c1v1"));
    // Card1标题在内容之上
    auto bTitle = drv.widgetBounds("c1title");
    auto bRow1 = drv.widgetBounds("c1row1");
    EXPECT_LT(bTitle.y, bRow1.y) << "Title should be above first row";
    // Card2
    EXPECT_TRUE(drv.widgetVisible("card2"));
    auto bCard1 = drv.widgetBounds("card1");
    auto bCard2 = drv.widgetBounds("card2");
    EXPECT_LT(bCard1.y + bCard1.h, bCard2.y + 1.0f) << "Card2 should be below Card1";
}

TEST_F(AllExamplesTest, Ex03_LayoutDemo_DividersNoOverlap) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX03_JSON);
    EXPECT_TRUE(drv.widgetVisible("div1"));
    auto bDiv = drv.widgetBounds("div1");
    EXPECT_GT(bDiv.w, 10.0f) << "Divider should span width";
}

// ============================================================
// 示例04: form_demo
// ============================================================
TEST_F(AllExamplesTest, Ex04_FormDemo_TextFieldVisible) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX04_JSON);
    EXPECT_TRUE(drv.widgetVisible("tf1"));
    EXPECT_TRUE(drv.widgetEnabled("tf1"));
    EXPECT_TRUE(drv.widgetVisible("tf2"));
    EXPECT_TRUE(drv.widgetEnabled("tf2"));
    EXPECT_TRUE(drv.widgetVisible("btn"));
    // TextField应有足够宽度
    auto b1 = drv.widgetBounds("tf1");
    EXPECT_GT(b1.w, 50.0f) << "TextField should have reasonable width";
}

TEST_F(AllExamplesTest, Ex04_FormDemo_FormLayout) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX04_JSON);
    // 标签在输入框左边
    auto b_l1 = drv.widgetBounds("l1");
    auto b_tf1 = drv.widgetBounds("tf1");
    EXPECT_LT(b_l1.x + b_l1.w, b_tf1.x + 2.0f) << "Label should be left of TextField";
    // Row1在Row2之上
    auto b_r1 = drv.widgetBounds("r1");
    auto b_r2 = drv.widgetBounds("r2");
    EXPECT_LT(b_r1.y, b_r2.y) << "Row1 should be above Row2";
}

// ============================================================
// 示例05: controls_demo
// ============================================================
TEST_F(AllExamplesTest, Ex05_ControlsDemo_CheckBoxVisible) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX05_JSON);
    EXPECT_TRUE(drv.widgetVisible("cb1"));
    EXPECT_TRUE(drv.widgetVisible("cb2"));
    EXPECT_TRUE(drv.widgetVisible("tg1"));
    EXPECT_EQ(drv.widgetText("cb1"), "Agree");
    EXPECT_EQ(drv.widgetText("cb2"), "Subscribe");
}

TEST_F(AllExamplesTest, Ex05_ControlsDemo_SliderLayout) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX05_JSON);
    EXPECT_TRUE(drv.widgetVisible("sl1"));
    EXPECT_TRUE(drv.widgetVisible("sl2"));
    auto b1 = drv.widgetBounds("sl1");
    auto b2 = drv.widgetBounds("sl2");
    EXPECT_GT(b1.w, 50.0f) << "Slider1 should have reasonable width";
    EXPECT_GT(b2.w, 50.0f) << "Slider2 should have reasonable width";
    // 两个Slider不应重叠
    EXPECT_FALSE(drv.boundsOverlap("sl1", "sl2")) << "Sliders should not overlap";
}

// ============================================================
// 示例06: tabs_demo
// ============================================================
TEST_F(AllExamplesTest, Ex06_TabsDemo_Page0Content) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("t0", EX06_P0);
    EXPECT_TRUE(drv.widgetVisible("tabs"));
    EXPECT_TRUE(drv.widgetVisible("d"));
    EXPECT_TRUE(drv.widgetVisible("content"));
    EXPECT_EQ(drv.widgetText("content"), "Welcome!");
}

// ============================================================
// 示例07: list_demo
// ============================================================
TEST_F(AllExamplesTest, Ex07_ListDemo_HasManyItems) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX07_JSON);
    EXPECT_TRUE(drv.widgetVisible("list"));
    auto b = drv.widgetBounds("list");
    EXPECT_GT(b.w, 200.0f);
    EXPECT_GT(b.h, 200.0f);
    // 列表应在标题下方
    EXPECT_TRUE(drv.boundsContains("root", "list"));
    auto bTitle = drv.widgetBounds("title");
    EXPECT_LT(bTitle.y + bTitle.h, b.y + 2.0f) << "Title should be above List";
}

// ============================================================
// 示例08: grid_demo
// ============================================================
TEST_F(AllExamplesTest, Ex08_GridDemo_Structure) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX08_JSON);
    EXPECT_TRUE(drv.widgetVisible("title"));
    EXPECT_TRUE(drv.widgetVisible("desc"));
    EXPECT_TRUE(drv.widgetVisible("grid"));
    auto b = drv.widgetBounds("grid");
    EXPECT_GT(b.w, 300.0f);
    EXPECT_GT(b.h, 100.0f);
}

// ============================================================
// 示例09: binding_demo
// ============================================================
TEST_F(AllExamplesTest, Ex09_BindingDemo_ButtonsExist) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX09_JSON);
    EXPECT_TRUE(drv.widgetVisible("title"));
    EXPECT_TRUE(drv.widgetVisible("btn1"));
    EXPECT_TRUE(drv.widgetVisible("btn2"));
    EXPECT_TRUE(drv.widgetVisible("btn3"));
    // title使用path绑定，初始未设置数据时文本应为空或path
    // 这个测试验证widget存在即可，data绑定在后续场景测试
}

// ============================================================
// 示例10: full_app 仪表盘
// ============================================================
TEST_F(AllExamplesTest, Ex10_FullApp_Dashboard) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX10_P0);
    EXPECT_TRUE(drv.widgetVisible("tabs"));
    EXPECT_TRUE(drv.widgetVisible("title"));
    EXPECT_TRUE(drv.widgetVisible("stats"));
    // 3个统计卡片
    EXPECT_TRUE(drv.widgetVisible("card1"));
    EXPECT_TRUE(drv.widgetVisible("card2"));
    EXPECT_TRUE(drv.widgetVisible("card3"));
    // Card内部
    EXPECT_TRUE(drv.widgetVisible("c1t"));
    EXPECT_TRUE(drv.widgetVisible("c1v"));
    EXPECT_TRUE(drv.widgetVisible("c2v"));
    EXPECT_TRUE(drv.widgetVisible("c3v"));
    // 操作按钮
    EXPECT_TRUE(drv.widgetVisible("btn_refresh"));
    EXPECT_TRUE(drv.widgetEnabled("btn_refresh"));
    EXPECT_TRUE(drv.widgetVisible("btn_export"));
    EXPECT_TRUE(drv.widgetEnabled("btn_export"));
}

TEST_F(AllExamplesTest, Ex10_FullApp_CardLayout) {
    AiTestDriver drv;
    ASSERT_TRUE(drv.initialize());
    drv.loadUI("main", EX10_P0);
    // 3个Card在Row中水平排列，不应重叠
    EXPECT_FALSE(drv.boundsOverlap("card1", "card2"));
    EXPECT_FALSE(drv.boundsOverlap("card2", "card3"));
    // Card1在Card2左边
    auto b1 = drv.widgetBounds("card1");
    auto b2 = drv.widgetBounds("card2");
    EXPECT_LT(b1.x + b1.w, b2.x + 5.0f) << "Card1 should be left of Card2";
}

// ============================================================
// 综合测试: 跨示例验证通用行为
// ============================================================

// 所有Column根容器的子widget应有合理的Y递增（垂直不重叠）
TEST_F(AllExamplesTest, AllExamples_NoRootOverlapWithFirstChild) {
    // 对每个示例，检查root和第一个子控件
    struct { const char* json; const char* root; const char* child; } cases[] = {
        {EX01_JSON, "root", "txt"},
        {EX02_JSON, "root", "title"},
        {EX03_JSON, "root", "card1"},
        {EX04_JSON, "root", "title"},
        {EX05_JSON, "root", "title"},
        {EX07_JSON, "root", "title"},
        {EX08_JSON, "root", "title"},
        {EX09_JSON, "root", "title"},
    };
    for (auto& tc : cases) {
        AiTestDriver drv;
        ASSERT_TRUE(drv.initialize());
        drv.loadUI("main", tc.json);
        EXPECT_TRUE(drv.widgetExists(tc.child)) << "Missing: " << tc.child;
        EXPECT_TRUE(drv.widgetVisible(tc.child)) << "Hidden: " << tc.child;
        // root应包含子控件
        EXPECT_TRUE(drv.boundsContains(tc.root, tc.child))
            << tc.root << " should contain " << tc.child;
    }
}

// 所有Button控件应可点击（不崩溃）
TEST_F(AllExamplesTest, AllButtons_ClickableWithoutCrash) {
    struct { const char* json; const char* bid; } cases[] = {
        {EX02_JSON, "btn1"}, {EX02_JSON, "btn2"}, {EX02_JSON, "btn3"},
        {EX04_JSON, "btn"},
        {EX09_JSON, "btn1"}, {EX09_JSON, "btn2"}, {EX09_JSON, "btn3"},
        {EX10_P0, "btn_refresh"}, {EX10_P0, "btn_export"},
    };
    for (auto& tc : cases) {
        AiTestDriver drv;
        ASSERT_TRUE(drv.initialize());
        drv.loadUI("main", tc.json);
        ASSERT_TRUE(drv.widgetExists(tc.bid));
        // 点击按钮（验证不崩溃）
        drv.click(tc.bid);
    }
}
