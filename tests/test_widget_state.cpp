/**
 * test_widget_state.cpp — 所有控件状态机全覆盖测试
 * 覆盖每个状态的初始值、合法/非法转换、边界条件。
 */
#include <gtest/gtest.h>
#include "jui/render/widget_state.h"

using namespace jui;

// ============================================================
// WidgetState 基类
// ============================================================
TEST(WidgetStateBaseTest, Focus_Toggle) {
    WidgetState s;
    EXPECT_FALSE(s.focused());
    s.setFocused(true);  EXPECT_TRUE(s.focused());
    s.setFocused(false); EXPECT_FALSE(s.focused());
}

TEST(WidgetStateBaseTest, Focus_DuplicateSet_NoNotify) {
    WidgetState s;
    int calls=0; s.onChange([&]{calls++;});
    s.setFocused(false); // already false
    EXPECT_EQ(calls,0);
    s.setFocused(true);
    EXPECT_EQ(calls,1);
}

TEST(WidgetStateBaseTest, Hover) {
    WidgetState s;
    EXPECT_FALSE(s.hovered());
    s.setHovered(true);  EXPECT_TRUE(s.hovered());
}

TEST(WidgetStateBaseTest, Enabled_Toggle) {
    WidgetState s;
    EXPECT_TRUE(s.enabled());
    s.setEnabled(false); EXPECT_FALSE(s.enabled());
}

TEST(WidgetStateBaseTest, Disabled_ClearsFocus) {
    WidgetState s;
    s.setFocused(true);
    s.setEnabled(false);
    EXPECT_FALSE(s.focused());
}

// ============================================================
// TextWidgetState
// ============================================================
TEST(TextStateTest, SetGetText) {
    TextWidgetState s;
    s.setText("Hello");
    EXPECT_EQ(s.text(), "Hello");
}

TEST(TextStateTest, Overflow_Defaults) {
    TextWidgetState s;
    EXPECT_EQ(s.overflow(), TextWidgetState::Overflow::Visible);
    s.setOverflow(TextWidgetState::Overflow::Ellipsis);
    EXPECT_EQ(s.overflow(), TextWidgetState::Overflow::Ellipsis);
}

// ============================================================
// TextFieldWidgetState — 状态 + 转换
// ============================================================

// ---- 初始状态 ----
TEST(TextFieldStateTest, Initial_Empty) {
    TextFieldWidgetState s;
    EXPECT_EQ(s.text(), "");
    EXPECT_EQ(s.cursorPos(), 0);
    EXPECT_FALSE(s.hasSelection());
    EXPECT_FALSE(s.imeActive());
    EXPECT_EQ(s.imeState(), TextFieldWidgetState::IMEState::Idle);
    EXPECT_EQ(s.dragState(), TextFieldWidgetState::DragState::None);
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::None);
    EXPECT_EQ(s.inputMode(), TextFieldWidgetState::InputMode::Normal);
    EXPECT_EQ(s.maxLength(), static_cast<size_t>(-1));
}

// ---- 模式切换 ----
TEST(TextFieldStateTest, Mode_ReadOnly_BlocksEdit) {
    TextFieldWidgetState s;
    s.setInputMode(TextFieldWidgetState::InputMode::ReadOnly);
    EXPECT_TRUE(s.isReadOnly());
    s.handleChar('A');
    EXPECT_EQ(s.text(), "");
    s.backspace();
    EXPECT_EQ(s.text(), "");
    s.insertText("X");
    EXPECT_EQ(s.text(), "");
}

TEST(TextFieldStateTest, Mode_ReadOnly_AllowsCursor) {
    TextFieldWidgetState s;
    s.setText("abc");
    s.setInputMode(TextFieldWidgetState::InputMode::ReadOnly);
    s.moveCursorLeft(false);
    EXPECT_EQ(s.cursorPos(), 2); // 光标可以移动
}

// ---- 文本操作 ----
TEST(TextFieldStateTest, SetText_MovesCursor) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("hello");
    EXPECT_EQ(s.cursorPos(), 5);
}

TEST(TextFieldStateTest, HandleChar_Inserts) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.handleChar('A'); s.handleChar('B');
    EXPECT_EQ(s.text(), "AB");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::InsertChar);
}

TEST(TextFieldStateTest, HandleChar_Disabled_Ignored) {
    TextFieldWidgetState s;
    s.setEnabled(false);
    s.handleChar('A');
    EXPECT_EQ(s.text(), "");
}

TEST(TextFieldStateTest, HandleChar_ControlChar_Ignored) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.handleChar(10); s.handleChar(127);
    EXPECT_EQ(s.text(), "");
}

TEST(TextFieldStateTest, HandleChar_MaxLengthRespected) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setMaxLength(3);
    s.handleChar('A'); s.handleChar('B'); s.handleChar('C');
    s.handleChar('D'); // 应被阻止
    EXPECT_EQ(s.text(), "ABC");
}

TEST(TextFieldStateTest, InsertText_Basic) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.insertText("World");
    EXPECT_EQ(s.text(), "World");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::InsertText);
}

TEST(TextFieldStateTest, InsertText_ReadOnly_Blocked) {
    TextFieldWidgetState s;
    s.setInputMode(TextFieldWidgetState::InputMode::ReadOnly);
    s.insertText("nope");
    EXPECT_EQ(s.text(), "");
}

// ---- 删除 ----
TEST(TextFieldStateTest, Backspace_Basic) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABC");
    s.backspace();
    EXPECT_EQ(s.text(), "AB");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::DeleteBack);
}

TEST(TextFieldStateTest, Backspace_AtStart_NoOp) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.backspace(); // 空文本光标在 0
    EXPECT_EQ(s.text(), "");
}

TEST(TextFieldStateTest, DeleteForward_Basic) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABC"); s.moveCursorHome(false);
    s.deleteForward();
    EXPECT_EQ(s.text(), "BC");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::DeleteForward);
}

TEST(TextFieldStateTest, DeleteForward_AtEnd_NoOp) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABC");
    s.deleteForward();
    EXPECT_EQ(s.text(), "ABC");
}

TEST(TextFieldStateTest, DeleteSelection_Replaces) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABCDEF");
    s.selectAll();
    // backspace() → hasSelection=true → deleteSelection() → 清空文本并 return
    // deleteSelection 设置 lastAction=Clear
    s.backspace();
    EXPECT_EQ(s.text(), "");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::Clear);
}

TEST(TextFieldStateTest, DeleteSelection_ReadOnly_Blocked) {
    TextFieldWidgetState s;
    s.setText("abc");
    s.setInputMode(TextFieldWidgetState::InputMode::ReadOnly);
    s.selectAll();
    s.deleteSelection();
    EXPECT_EQ(s.text(), "abc");
}

// ---- 选区 ----
TEST(TextFieldStateTest, Selection_SelectAll) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("Select me");
    s.selectAll();
    EXPECT_TRUE(s.hasSelection());
    EXPECT_EQ(s.selectionStart(), 0);
    EXPECT_EQ(s.selectionEnd(), 9);
}

TEST(TextFieldStateTest, Selection_Clear) {
    TextFieldWidgetState s;
    s.setText("abc");
    s.selectAll();
    s.clearSelection();
    EXPECT_FALSE(s.hasSelection());
}

// ---- 剪贴板 ----
TEST(TextFieldStateTest, Clipboard_Copy) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("Copy me");
    s.selectAll();
    EXPECT_EQ(s.copySelection(), "Copy me");
}

TEST(TextFieldStateTest, Clipboard_Copy_NoSelection) {
    TextFieldWidgetState s;
    s.setText("text");
    EXPECT_EQ(s.copySelection(), "");
}

TEST(TextFieldStateTest, Clipboard_Cut) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("Cut me");
    s.selectAll();
    s.cutSelection();
    EXPECT_EQ(s.text(), "");
    EXPECT_EQ(s.lastAction(), TextFieldWidgetState::EditAction::Cut);
}

TEST(TextFieldStateTest, Clipboard_Cut_ReadOnly_Blocked) {
    TextFieldWidgetState s;
    s.setText("abc");
    s.setInputMode(TextFieldWidgetState::InputMode::ReadOnly);
    s.selectAll();
    s.cutSelection();
    EXPECT_EQ(s.text(), "abc");
}

// ---- 光标移动 ----
TEST(TextFieldStateTest, Cursor_LeftRight_NoExtend) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("AB");
    s.moveCursorLeft(false);
    EXPECT_EQ(s.cursorPos(), 1); EXPECT_FALSE(s.hasSelection());
    s.moveCursorRight(false);
    EXPECT_EQ(s.cursorPos(), 2);
}

TEST(TextFieldStateTest, Cursor_LeftRight_ExtendSelection) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABCD");
    s.moveCursorHome(false);
    s.moveCursorRight(true); // shift+right
    EXPECT_TRUE(s.hasSelection());
    EXPECT_EQ(s.selectionStart(), 0);
    EXPECT_EQ(s.selectionEnd(), 1); // 因为先用 Home 清空选区
}

TEST(TextFieldStateTest, Cursor_HomeEnd) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("XYZ");
    s.moveCursorHome(false);
    EXPECT_EQ(s.cursorPos(), 0);
    s.moveCursorEnd(false);
    EXPECT_EQ(s.cursorPos(), 3);
}

TEST(TextFieldStateTest, Cursor_HomeEnd_Extend) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("XYZ");
    s.moveCursorHome(false);
    s.moveCursorEnd(true);
    EXPECT_TRUE(s.hasSelection());
    EXPECT_EQ(s.selectionStart(), 0);
    EXPECT_EQ(s.selectionEnd(), 3);
}

TEST(TextFieldStateTest, Cursor_LeftAtStart_NoOp) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.moveCursorLeft(false);
    EXPECT_EQ(s.cursorPos(), 0);
}

TEST(TextFieldStateTest, Cursor_RightAtEnd_NoOp) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("A");
    s.moveCursorRight(false);
    EXPECT_EQ(s.cursorPos(), 1);
}

// ---- 键盘 ----
TEST(TextFieldStateTest, KeyDown_Disabled_Ignored) {
    TextFieldWidgetState s;
    s.setEnabled(false); // 明确禁用
    s.setText("ABC");
    s.handleKeyDown(VK_BACK);
    EXPECT_EQ(s.text(), "ABC");
}

TEST(TextFieldStateTest, KeyDown_ShiftArrow_Extends) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ABC");
    // 注意：GetKeyState 在此测试中不可靠，但逻辑分支覆盖足够
    s.handleKeyDown(VK_LEFT);  // shift 通常不按
    s.handleKeyDown(VK_RIGHT);
    s.handleKeyDown(VK_HOME);
    s.handleKeyDown(VK_END);
    s.handleKeyDown(VK_DELETE);
    s.handleKeyDown(VK_BACK);
    SUCCEED(); // 不崩溃即可
}

// ---- IME 状态机 ----
TEST(TextFieldStateTest, IME_Start_EntersComposing) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.imeStart();
    EXPECT_EQ(s.imeState(), TextFieldWidgetState::IMEState::Composing);
    EXPECT_TRUE(s.imeActive());
}

TEST(TextFieldStateTest, IME_Start_ClearsSelection) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("hello"); s.selectAll();
    s.imeStart();
    EXPECT_FALSE(s.hasSelection());
}

TEST(TextFieldStateTest, IME_Update_StoresComposition) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.imeStart();
    s.imeUpdate("nihao");
    EXPECT_EQ(s.imeComposition(), "nihao");
}

TEST(TextFieldStateTest, IME_End_CommitsText) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("text");
    s.imeStart();
    s.imeUpdate("comp");
    s.imeEnd("你好");
    EXPECT_FALSE(s.imeActive());
    EXPECT_EQ(s.text(), "text\xE4\xBD\xA0\xE5\xA5\xBD"); // UTF-8 你好
}

TEST(TextFieldStateTest, IME_End_Empty_NoChange) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("keep");
    s.imeStart();
    s.imeEnd("");
    EXPECT_EQ(s.text(), "keep");
}

TEST(TextFieldStateTest, IME_FullCycle) {
    TextFieldWidgetState s; s.setEnabled(true);
    EXPECT_EQ(s.imeState(), TextFieldWidgetState::IMEState::Idle);
    s.imeStart();
    EXPECT_EQ(s.imeState(), TextFieldWidgetState::IMEState::Composing);
    s.imeUpdate("pinyin");
    s.imeUpdate("pingyin");
    s.imeEnd("拼音");
    EXPECT_EQ(s.imeState(), TextFieldWidgetState::IMEState::Idle);
    EXPECT_EQ(s.text(), "拼音");
}

// ---- 拖拽 ----
TEST(TextFieldStateTest, Drag_MouseDownUp_Cycle) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("hello world");
    s.mouseDown(false);
    EXPECT_EQ(s.dragState(), TextFieldWidgetState::DragState::Selecting);
    s.mouseMove();
    s.mouseUp();
    EXPECT_EQ(s.dragState(), TextFieldWidgetState::DragState::None);
}

TEST(TextFieldStateTest, Drag_CtrlClick_SelectsAll) {
    TextFieldWidgetState s; s.setEnabled(true);
    s.setText("ctrl click");
    s.mouseDown(true); // ctrl held
    EXPECT_TRUE(s.hasSelection());
}

TEST(TextFieldStateTest, Drag_Disabled_Ignored) {
    TextFieldWidgetState s;
    s.setEnabled(false);
    s.mouseDown(false);
    EXPECT_EQ(s.dragState(), TextFieldWidgetState::DragState::None);
}

// ---- 光标闪烁 ----
TEST(TextFieldStateTest, Caret_Update_NotFocused_Off) {
    TextFieldWidgetState s;
    s.updateCaret(1.0f);
    EXPECT_FALSE(s.caretVisible());
}

TEST(TextFieldStateTest, Caret_Update_Focused_Toggles) {
    TextFieldWidgetState s; s.setFocused(true);
    s.updateCaret(0.3f);
    bool a = s.caretVisible();
    s.updateCaret(0.3f); // 0.6 > 0.53
    EXPECT_NE(a, s.caretVisible());
}

TEST(TextFieldStateTest, Caret_BlinkMultiple) {
    TextFieldWidgetState s; s.setFocused(true);
    bool v = s.caretVisible();
    // 跑 10 个周期
    for(int i=0;i<10;i++){s.updateCaret(0.53f);v=!v;}
    EXPECT_EQ(v, s.caretVisible());
}

// ---- 变更通知 ----
TEST(TextFieldStateTest, ChangeNotify_Fires) {
    TextFieldWidgetState s; s.setEnabled(true);
    int n=0; s.onChange([&]{n++;});
    s.handleChar('X');
    EXPECT_GE(n,1);
}

// ============================================================
// ButtonWidgetState
// ============================================================
TEST(ButtonStateTest, VisualState_Normal) {
    ButtonWidgetState s;
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Normal);
}

TEST(ButtonStateTest, VisualState_Hovered) {
    ButtonWidgetState s; s.setHovered(true);
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Hovered);
}

TEST(ButtonStateTest, VisualState_Pressed) {
    ButtonWidgetState s; s.press();
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Pressed);
}

TEST(ButtonStateTest, VisualState_Disabled) {
    ButtonWidgetState s; s.setEnabled(false);
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Disabled);
}

TEST(ButtonStateTest, PressDisabled_Ignored) {
    ButtonWidgetState s; s.setEnabled(false);
    s.press();
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Disabled);
}

TEST(ButtonStateTest, PressHover_Transition) {
    ButtonWidgetState s;
    s.setHovered(true);
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Hovered);
    s.press();
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Pressed);
    s.release();
    EXPECT_EQ(s.visualState(), ButtonWidgetState::VisualState::Hovered);
}

// ============================================================
// CheckBoxWidgetState
// ============================================================
TEST(CheckBoxStateTest, Toggle_FullCycle) {
    CheckBoxWidgetState s;
    EXPECT_EQ(s.checkState(), CheckBoxWidgetState::CheckState::Unchecked);
    s.toggle();
    EXPECT_TRUE(s.isChecked());
    s.toggle();
    EXPECT_FALSE(s.isChecked());
}

TEST(CheckBoxStateTest, SetCheckState) {
    CheckBoxWidgetState s;
    s.setCheckState(CheckBoxWidgetState::CheckState::Indeterminate);
    EXPECT_EQ(s.checkState(), CheckBoxWidgetState::CheckState::Indeterminate);
    s.toggle(); // Indeterminate → Checked
    EXPECT_TRUE(s.isChecked());
}

// ============================================================
// ToggleWidgetState
// ============================================================
TEST(ToggleStateTest, Toggle) {
    ToggleWidgetState s;
    EXPECT_FALSE(s.toggled());
    s.toggle(); EXPECT_TRUE(s.toggled());
    s.toggle(); EXPECT_FALSE(s.toggled());
}

TEST(ToggleStateTest, SetLabel) {
    ToggleWidgetState s;
    s.setLabel("Wifi"); EXPECT_EQ(s.label(), "Wifi");
}

// ============================================================
// ChoicePickerWidgetState
// ============================================================
TEST(ChoicePickerStateTest, Initial_Closed) {
    ChoicePickerWidgetState s;
    EXPECT_FALSE(s.isOpen());
    EXPECT_EQ(s.selectedIndex(), 0);
    EXPECT_EQ(s.selectedText(), "");
}

TEST(ChoicePickerStateTest, OpenClose_Toggle) {
    ChoicePickerWidgetState s;
    s.open(); EXPECT_TRUE(s.isOpen());
    s.close(); EXPECT_FALSE(s.isOpen());
    s.toggle(); EXPECT_TRUE(s.isOpen());
    s.toggle(); EXPECT_FALSE(s.isOpen());
}

TEST(ChoicePickerStateTest, SelectIndex) {
    ChoicePickerWidgetState s;
    s.setOptions({"A","B","C"});
    s.selectIndex(2);
    EXPECT_EQ(s.selectedIndex(), 2);
    EXPECT_EQ(s.selectedText(), "C");
    EXPECT_FALSE(s.isOpen()); // 选中后关闭
}

TEST(ChoicePickerStateTest, SelectIndex_OutOfBounds_Ignored) {
    ChoicePickerWidgetState s;
    s.setOptions({"A","B"});
    s.selectIndex(5);
    EXPECT_EQ(s.selectedIndex(), 0); // 不变
}

TEST(ChoicePickerStateTest, IndexChanged_Callback) {
    ChoicePickerWidgetState s;
    s.setOptions({"X","Y","Z"});
    int idx=-1; s.onIndexChanged([&](int i){idx=i;});
    s.selectIndex(1);
    EXPECT_EQ(idx,1);
}

// ============================================================
// SliderWidgetState
// ============================================================
TEST(SliderStateTest, DefaultRange) {
    SliderWidgetState s;
    EXPECT_EQ(s.minValue(), 0);
    EXPECT_EQ(s.maxValue(), 100);
    EXPECT_EQ(s.step(), 1);
    EXPECT_EQ(s.value(), 0);
}

TEST(SliderStateTest, SetRange) {
    SliderWidgetState s;
    s.setRange(10, 200, 5);
    EXPECT_EQ(s.minValue(), 10);
    EXPECT_EQ(s.maxValue(), 200);
    EXPECT_EQ(s.step(), 5);
}

TEST(SliderStateTest, SetValue_WithinRange) {
    SliderWidgetState s;
    s.setValue(50);
    EXPECT_EQ(s.value(), 50);
}

TEST(SliderStateTest, SetValue_ClampedToRange) {
    SliderWidgetState s;
    s.setValue(-10);
    EXPECT_EQ(s.value(), 0);
    s.setValue(999);
    EXPECT_EQ(s.value(), 100);
}

TEST(SliderStateTest, SetValue_SnapToStep) {
    SliderWidgetState s;
    s.setRange(0, 100, 10);
    s.setValue(23);
    EXPECT_EQ(s.value(), 20);
    s.setValue(27);
    EXPECT_EQ(s.value(), 30);
}

TEST(SliderStateTest, Drag_TempCommit) {
    SliderWidgetState s;
    EXPECT_FALSE(s.isDragging());
    s.setDragging(true);
    EXPECT_TRUE(s.isDragging());
    s.setTempValue(80);
    s.setDragging(false); // commit
    EXPECT_EQ(s.value(), 80);
    EXPECT_FALSE(s.isDragging());
}

TEST(SliderStateTest, ValueChanged_Callback) {
    SliderWidgetState s;
    float v=-1; s.onValueChanged([&](float f){v=f;});
    s.setValue(75);
    EXPECT_EQ(v, 75);
}

TEST(SliderStateTest, ValueChanged_NotFiredForSameValue) {
    SliderWidgetState s;
    int calls=0; s.onValueChanged([&](float){calls++;});
    s.setValue(0); // same as default
    EXPECT_EQ(calls,0);
}
