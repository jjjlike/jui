/**
 * @file test_modular_states.cpp
 * @brief 验证模块化状态类别名与原类兼容
 */
#include <gtest/gtest.h>
#include "jui/render/jui_states.h"
#include <type_traits>

using namespace jui;

TEST(ModularStates, TextState_IsAlias) {
    state::TextState s;
    s.setText("hello");
    EXPECT_EQ(s.text(), "hello");
    EXPECT_TRUE(s.enabled());
    s.setEnabled(false);
    EXPECT_FALSE(s.enabled());
}

TEST(ModularStates, ButtonState_IsAlias) {
    state::ButtonState s;
    s.setText("OK");
    s.press();
    EXPECT_EQ(s.visualState(), state::ButtonState::VisualState::Pressed);
    s.release();
    EXPECT_EQ(s.visualState(), state::ButtonState::VisualState::Normal);
}

TEST(ModularStates, CheckBoxState_IsAlias) {
    state::CheckBoxState s;
    EXPECT_FALSE(s.isChecked());
    s.toggle();
    EXPECT_TRUE(s.isChecked());
}

TEST(ModularStates, ToggleState_IsAlias) {
    state::ToggleState s;
    EXPECT_FALSE(s.toggled());
    s.toggle();
    EXPECT_TRUE(s.toggled());
}

TEST(ModularStates, SliderState_IsAlias) {
    state::SliderState s;
    s.setValue(42);
    EXPECT_NEAR(s.value(), 42, 0.1f);
    EXPECT_NEAR(s.minValue(), 0, 0.1f);
    EXPECT_NEAR(s.maxValue(), 100, 0.1f);
}

TEST(ModularStates, ListState_IsAlias) {
    state::ListState s;
    s.setItemCount(100);
    s.setItemHeight(32);
    EXPECT_EQ(s.itemCount(), 100);
    EXPECT_NEAR(s.itemHeight(), 32, 0.1f);
}

TEST(ModularStates, TabsState_IsAlias) {
    state::TabsState s;
    s.setTabs({{"A",""},{"B",""},{"C",""}});
    EXPECT_EQ(s.tabCount(), 3);
    EXPECT_EQ(s.activeIndex(), 0);
    s.setActiveIndex(1);
    EXPECT_EQ(s.activeIndex(), 1);
}

TEST(ModularStates, PickerState_IsAlias) {
    state::PickerState s;
    s.setOptions({"One","Two","Three"});
    s.selectIndex(2);
    EXPECT_EQ(s.selectedText(), "Three");
}

TEST(ModularStates, TextFieldState_IsAlias) {
    state::TextFieldState s;
    s.setText("test");
    EXPECT_EQ(s.text(), "test");
    s.insertText("ing");
    EXPECT_NE(s.text().find("testing"), std::string::npos);
}

TEST(ModularStates, GridState_IsAlias) {
    state::GridState s;
    s.setColumns({{"A",80},{"B",100}});
    s.setRowCount(1000);
    EXPECT_EQ(s.columnCount(), 2);
    EXPECT_EQ(s.rowCount(), 1000);
}
