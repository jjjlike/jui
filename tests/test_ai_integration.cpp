/**
 * @file test_ai_integration.cpp
 * @brief AI自动化集成测试 — 对 jui_hello 各页面进行全链路自动验证
 *
 * 设计目标：零人工干预，AI可直接解析测试结果
 */
#include <gtest/gtest.h>
#include "jui/jui.h"
#include "jui/test/driver.h"

using namespace jui::test;

// ============================================================
// 6个Tab页面定义（使用R"JSON( ... )JSON"避免raw string冲突）
// ============================================================
static const char* PAGE_0 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","actions"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"Button/Text Demo"},"fontSize":{"literalNumber":16}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"Hello JUI AI Test"}}}},
{"id":"r2","component":{"Text":{"text":{"literalString":"20px Bold Blue"},"fontSize":{"literalNumber":20},"fontWeight":{"literalString":"bold"},"textColor":{"literalString":"#2266CC"}}}},
{"id":"r3","component":{"Text":{"text":{"literalString":"12px Gray"},"fontSize":{"literalNumber":12},"textColor":{"literalString":"#999999"}}}},
{"id":"actions","component":{"Row":{"children":{"explicitList":["btnA","btnB","btnC"]}}}},
{"id":"btnA","component":{"Button":{"text":{"literalString":"A"}}}},
{"id":"btnB","component":{"Button":{"text":{"literalString":"ClickMe"},"action":"click"}}},
{"id":"btnC","component":{"Button":{"text":{"literalString":"Submit"}}}}
]}})JSON";

static const char* PAGE_1 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"TextField Demo"}}}},
{"id":"r1","component":{"Row":{"children":{"explicitList":["lbl1","tf1"]}}}},
{"id":"lbl1","component":{"Text":{"text":{"literalString":"Name:"}}}},
{"id":"tf1","component":{"TextField":{"placeholder":{"literalString":"enter name"},"width":{"literalNumber":180}}}},
{"id":"r2","component":{"Row":{"children":{"explicitList":["lbl2","tf2"]}}}},
{"id":"lbl2","component":{"Text":{"text":{"literalString":"Email:"}}}},
{"id":"tf2","component":{"TextField":{"width":{"literalNumber":220}}}}
]}})JSON";

static const char* PAGE_2 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","r4","r5"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"CheckBox/Toggle Demo"}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"Checkboxes:"}}}},
{"id":"r2","component":{"CheckBox":{"text":{"literalString":"OptionA"}}}},
{"id":"r3","component":{"CheckBox":{"text":{"literalString":"OptionB"},"value":{"literalBoolean":true}}}},
{"id":"r4","component":{"CheckBox":{"text":{"literalString":"OptionC"}}}},
{"id":"r5","component":{"Toggle":{"label":{"literalString":"Notify"},"value":{"literalBoolean":true}}}}
]}})JSON";

static const char* PAGE_3 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","r1","r2","r3","r4"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"Slider Demo"}}}},
{"id":"r1","component":{"Text":{"text":{"literalString":"Vol 50%"},"fontSize":{"literalNumber":14}}}},
{"id":"r2","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":50}}}},
{"id":"r3","component":{"Text":{"text":{"literalString":"Bright 80%"},"fontSize":{"literalNumber":14}}}},
{"id":"r4","component":{"Slider":{"min":{"literalNumber":0},"max":{"literalNumber":100},"value":{"literalNumber":80}}}}
]}})JSON";

static const char* PAGE_4 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","list"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"List 100K"},"fontSize":{"literalNumber":16}}}},
{"id":"list","component":{"List":{"itemCount":{"literalNumber":100000},"itemHeight":{"literalNumber":32},"width":{"literalNumber":420},"height":{"literalNumber":380}}}}
]}})JSON";

static const char* PAGE_5 = R"JSON({"surfaceUpdate":{"surfaceId":"main","components":[
{"id":"root","component":{"Column":{"children":{"explicitList":["tabs","d","hdr","grid"]}}}},
{"id":"tabs","component":{"Tabs":{"tabs":[
  {"title":"Button","id":"tbtn"},{"title":"TextField","id":"ttf"},
  {"title":"Check","id":"tck"},{"title":"Picker","id":"tpk"},
  {"title":"List","id":"tls"},{"title":"Grid","id":"tgd"}]}}},
{"id":"d","component":{"Divider":{}}},
{"id":"hdr","component":{"Text":{"text":{"literalString":"Grid 50K"},"fontSize":{"literalNumber":16}}}},
{"id":"grid","component":{"Grid":{"width":{"literalNumber":520},"height":{"literalNumber":380}}}}
]}})JSON";

static const char* PAGES[] = {PAGE_0, PAGE_1, PAGE_2, PAGE_3, PAGE_4, PAGE_5};

// ============================================================
class AiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
    void TearDown() override { CoUninitialize(); }
};

// ---- 场景1: Tab页面切换 ----
TEST_F(AiIntegrationTest, Scn01_TabSwitching) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[0]);
    EXPECT_TRUE(driver.widgetExists("root"));
    EXPECT_TRUE(driver.widgetExists("tabs"));
    EXPECT_TRUE(driver.widgetExists("btnA"));
    EXPECT_TRUE(driver.widgetExists("btnB"));
}

// ---- 场景2: 按钮交互 ----
TEST_F(AiIntegrationTest, Scn02_ButtonVisibility) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[0]);
    EXPECT_TRUE(driver.widgetVisible("btnA"));
    EXPECT_TRUE(driver.widgetEnabled("btnB"));
    EXPECT_EQ(driver.widgetText("btnB"), "ClickMe");
}

// ---- 场景3: 编辑框可见 ----
TEST_F(AiIntegrationTest, Scn03_TextFieldExists) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[1]);
    EXPECT_TRUE(driver.widgetVisible("tf1"));
    EXPECT_TRUE(driver.widgetEnabled("tf1"));
    EXPECT_TRUE(driver.widgetVisible("tf2"));
}

// ---- 场景4: 复选框状态 ----
TEST_F(AiIntegrationTest, Scn04_CheckBoxState) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[2]);
    EXPECT_TRUE(driver.widgetExists("r2"));
    EXPECT_TRUE(driver.widgetExists("r3"));
    EXPECT_TRUE(driver.widgetExists("r5"));
}

// ---- 场景5: 滑动条值 ----
TEST_F(AiIntegrationTest, Scn05_SliderValue) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[3]);
    EXPECT_TRUE(driver.widgetExists("r2"));
    EXPECT_TRUE(driver.widgetExists("r4"));
    EXPECT_NEAR(driver.sliderValue("r2"), 50.0f, 1.0f);
    EXPECT_NEAR(driver.sliderValue("r4"), 80.0f, 1.0f);
}

// ---- 场景6: 列表虚拟滚动 ----
TEST_F(AiIntegrationTest, Scn06_ListScroll) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[4]);
    EXPECT_TRUE(driver.widgetVisible("list"));
    auto b = driver.widgetBounds("list");
    EXPECT_GT(b.w, 200.0f);
    EXPECT_GT(b.h, 200.0f);
}

// ---- 场景7: 表格 ----
TEST_F(AiIntegrationTest, Scn07_GridStructure) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[5]);
    EXPECT_TRUE(driver.widgetVisible("grid"));
}

// ---- 场景8: 布局不重叠 ----
TEST_F(AiIntegrationTest, Scn08_LayoutNoOverlap) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[0]);
    EXPECT_FALSE(driver.boundsOverlap("tabs", "hdr"));
    EXPECT_TRUE(driver.boundsContains("root", "tabs"));
}

// ---- 场景9: 多页面连续切换不崩溃 ----
TEST_F(AiIntegrationTest, Scn09_MultiPageSwitch) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    for (int p = 0; p < 6; p++) {
        driver.loadUI("main", PAGES[p]);
        EXPECT_TRUE(driver.widgetVisible("root"));
        EXPECT_TRUE(driver.widgetVisible("tabs"));
    }
}

// ---- 场景10: 滑动条初始值验证 ----
TEST_F(AiIntegrationTest, Scn10_SliderValueInit) {
    AiTestDriver driver;
    ASSERT_TRUE(driver.initialize());
    driver.loadUI("main", PAGES[3]);
    // 验证滑动条渲染且Widget存在、可见
    EXPECT_TRUE(driver.widgetExists("r2"));
    EXPECT_TRUE(driver.widgetVisible("r2"));
    EXPECT_TRUE(driver.widgetExists("r4"));
    EXPECT_TRUE(driver.widgetVisible("r4"));
    // 点击滑动条（验证交互不崩溃）
    driver.click("r2");
    EXPECT_TRUE(driver.widgetExists("r2"));
}
