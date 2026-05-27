/**
 * test_render_widget.cpp — 渲染控件测试（状态测试已移至 test_widget_state.cpp）
 */
#include <gtest/gtest.h>
#include "jui/render/render_widget.h"

using namespace jui;

// ---- 基类 ----
class TestRenderWidget : public RenderWidget {
public:
    explicit TestRenderWidget(WidgetPtr w) : RenderWidget(w) {}
    void paint(ID2D1RenderTarget*, IDWriteFactory*) override {}
    Size measure(IDWriteFactory*) override { return {0, 0}; }
};

TEST(RenderBaseTest, HitTest_Inside) {
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    TestRenderWidget rw(w); rw.setBounds({10,10,100,50});
    EXPECT_TRUE(rw.hitTest(50,30)); EXPECT_FALSE(rw.hitTest(200,30));
}
TEST(RenderBaseTest, Bounds_SetGet) {
    TestRenderWidget rw(std::make_shared<Widget>("w",WidgetType::Text));
    rw.setBounds({5,10,200,100}); EXPECT_FLOAT_EQ(rw.bounds().w,200);
}
TEST(RenderBaseTest, CanFocus_False) {
    TestRenderWidget rw(std::make_shared<Widget>("w",WidgetType::Text));
    EXPECT_FALSE(rw.canFocus());
}

// ---- Factory ----
TEST(FactoryTest, AllTypes) {
    auto mk=[&](WidgetType t){ return RenderWidgetFactory::create(std::make_shared<Widget>("x",t)); };
    ASSERT_NE(mk(WidgetType::Text), nullptr);
    ASSERT_NE(mk(WidgetType::Button), nullptr);
    ASSERT_NE(mk(WidgetType::TextField), nullptr);
    ASSERT_NE(mk(WidgetType::CheckBox), nullptr);
    ASSERT_NE(mk(WidgetType::Toggle), nullptr);
    ASSERT_NE(mk(WidgetType::ChoicePicker), nullptr);
    ASSERT_NE(mk(WidgetType::Slider), nullptr);
    ASSERT_NE(mk(WidgetType::Row), nullptr);
    ASSERT_NE(mk(WidgetType::Column), nullptr);
    ASSERT_NE(mk(WidgetType::Card), nullptr);
    ASSERT_NE(mk(WidgetType::List), nullptr);
    ASSERT_NE(mk(WidgetType::Divider), nullptr);
    ASSERT_NE(mk(WidgetType::Image), nullptr);
    EXPECT_EQ(RenderWidgetFactory::create(nullptr), nullptr);
    EXPECT_EQ(mk(WidgetType::Custom), nullptr);
}

// ---- TextField Render ----
TEST(TextFieldRenderTest, CanFocus) {
    TextFieldRenderWidget rw(std::make_shared<Widget>("tf",WidgetType::TextField));
    EXPECT_TRUE(rw.canFocus());
}
TEST(TextFieldRenderTest, HitTest) {
    TextFieldRenderWidget rw(std::make_shared<Widget>("tf",WidgetType::TextField));
    rw.setBounds({10,10,150,30});
    EXPECT_TRUE(rw.hitTest(50,20)); EXPECT_FALSE(rw.hitTest(200,20));
}
TEST(TextFieldRenderTest, Measure) {
    auto w=std::make_shared<Widget>("tf",WidgetType::TextField);
    w->setProperty("width",JValue(200.0f));
    TextFieldRenderWidget rw(w);
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,200.0f);
}
TEST(TextFieldRenderTest, Clipboard) {
    auto w=std::make_shared<Widget>("tf",WidgetType::TextField);
    TextFieldRenderWidget rw(w);
    auto* st=dynamic_cast<TextFieldWidgetState*>(rw.state());
    st->setEnabled(true); st->setText("Hello"); st->selectAll();
    EXPECT_TRUE(rw.canCopy());
    EXPECT_EQ(rw.copySelection(),"Hello");
    rw.cutSelection();
    EXPECT_EQ(st->text(),"");
    rw.pasteText("World");
    EXPECT_EQ(st->text(),"World");
}

// ---- Button Render ----
TEST(ButtonRenderTest, CanFocus) {
    ButtonRenderWidget rw(std::make_shared<Widget>("b",WidgetType::Button));
    EXPECT_TRUE(rw.canFocus());
}
TEST(ButtonRenderTest, HitTest) {
    ButtonRenderWidget rw(std::make_shared<Widget>("b",WidgetType::Button));
    rw.setBounds({10,10,80,32});
    EXPECT_TRUE(rw.hitTest(50,20));
}
TEST(ButtonRenderTest, Measure) {
    ButtonRenderWidget rw(std::make_shared<Widget>("b",WidgetType::Button));
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,70.0f);
}

// ---- CheckBox Render ----
TEST(CheckBoxRenderTest, CanFocus) {
    CheckBoxRenderWidget rw(std::make_shared<Widget>("cb",WidgetType::CheckBox));
    EXPECT_TRUE(rw.canFocus());
}
TEST(CheckBoxRenderTest, Measure) {
    CheckBoxRenderWidget rw(std::make_shared<Widget>("cb",WidgetType::CheckBox));
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,100.0f);
}

// ---- Toggle/Choice/Slider Render ----
TEST(ToggleRenderTest, CanFocus) {
    ToggleRenderWidget rw(std::make_shared<Widget>("tg",WidgetType::Toggle));
    EXPECT_TRUE(rw.canFocus());
}
TEST(ChoicePickerRenderTest, CanFocus) {
    ChoicePickerRenderWidget rw(std::make_shared<Widget>("cp",WidgetType::ChoicePicker));
    EXPECT_TRUE(rw.canFocus());
}
TEST(SliderRenderTest, CanFocus) {
    SliderRenderWidget rw(std::make_shared<Widget>("sl",WidgetType::Slider));
    EXPECT_TRUE(rw.canFocus());
}
TEST(SliderRenderTest, HitTest_Expanded) {
    SliderRenderWidget rw(std::make_shared<Widget>("sl",WidgetType::Slider));
    rw.setBounds({10,10,150,24});
    EXPECT_TRUE(rw.hitTest(10,2)); // expanded area
}

// ---- Container/Divider/Image ----
TEST(ContainerRenderTest, Measure) {
    ContainerRenderWidget rw(std::make_shared<Widget>("c",WidgetType::Row));
    rw.setBounds({0,0,500,300});
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,500.0f);
}
TEST(DividerRenderTest, Measure) {
    DividerRenderWidget rw(std::make_shared<Widget>("d",WidgetType::Divider));
    rw.setBounds({0,0,600,0});
    EXPECT_FLOAT_EQ(rw.measure(nullptr).h,8.0f);
}
TEST(ImageRenderTest, Measure) {
    auto w=std::make_shared<Widget>("img",WidgetType::Image);
    ImageRenderWidget rw(w);
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,64.0f);
}
