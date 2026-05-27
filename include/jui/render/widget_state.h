#pragma once
#include <string>
#include <algorithm>
#include <functional>
#include <windows.h>

namespace jui {

// ============================================================
// WidgetState 基类
// ============================================================
class WidgetState {
public:
    virtual ~WidgetState() = default;

    // 焦点
    bool focused() const { return focused_; }
    void setFocused(bool f) {
        if (focused_ != f) { focused_ = f; notify(); }
    }
    // 悬停
    bool hovered() const { return hovered_; }
    void setHovered(bool h) { hovered_ = h; }
    // 启用
    bool enabled() const { return enabled_; }
    void setEnabled(bool e) {
        if (enabled_ != e) { enabled_ = e; if (!e) focused_ = false; notify(); }
    }

    // 变更通知
    using ChangeCb = std::function<void()>;
    void onChange(ChangeCb cb) { onChange_ = std::move(cb); }

protected:
    void notify() { if (onChange_) onChange_(); }
    bool focused_ = false;
    bool hovered_ = false;
    bool enabled_ = true;
    ChangeCb onChange_;
};

// ============================================================
// TextWidgetState — 标签控件状态
// ============================================================
class TextWidgetState : public WidgetState {
public:
    enum class Overflow { Visible, Ellipsis, Clip };
    void setText(const std::string& t) { text_ = t; notify(); }
    const std::string& text() const { return text_; }
    void setOverflow(Overflow o) { overflow_ = o; }
    Overflow overflow() const { return overflow_; }
private:
    std::string text_;
    Overflow overflow_ = Overflow::Visible;
};

// ============================================================
// TextFieldWidgetState — 编辑框状态机
// ============================================================
class TextFieldWidgetState : public WidgetState {
public:
    // ---- 状态枚举 ----
    enum class InputMode   { Normal, Password, ReadOnly };
    enum class IMEState    { Idle, Composing };
    enum class DragState   { None, Selecting };
    enum class EditAction  { None, InsertChar, InsertText, DeleteBack, DeleteForward,
                              Cut, Paste, Clear };

    static constexpr float CARET_BLINK   = 0.53f;
    static constexpr size_t NO_LIMIT     = static_cast<size_t>(-1);

    // ---- 模式 ----
    InputMode inputMode() const { return mode_; }
    void setInputMode(InputMode m) { mode_ = m; }
    bool isReadOnly() const { return mode_ == InputMode::ReadOnly; }

    // ---- 文本 ----
    const std::string& text() const { return text_; }
    void setText(const std::string& t) {
        text_ = enforceLimit(t);
        cursorPos_ = text_.length();
        clearSelection();
        notify();
    }

    const std::string& placeholder() const { return placeholder_; }
    void setPlaceholder(const std::string& ph) { placeholder_ = ph; }

    // ---- 长度限制 ----
    void setMaxLength(size_t n) { maxLen_ = n; }
    size_t maxLength() const { return maxLen_; }

    // ---- 光标 ----
    size_t cursorPos() const { return cursorPos_; }

    // ---- 选区 ----
    size_t selectionStart() const { return selStart_; }
    size_t selectionEnd() const { return selEnd_; }
    bool hasSelection() const { return selStart_ != selEnd_; }
    void clearSelection() { selStart_ = selEnd_ = cursorPos_; }

    // ---- 光标闪烁 ----
    bool caretVisible() const { return caretVisible_; }
    void updateCaret(float dt) {
        if (!focused_) { blink_ = 0; caretVisible_ = false; return; }
        blink_ += dt;
        if (blink_ >= CARET_BLINK) { blink_ -= CARET_BLINK; caretVisible_ = !caretVisible_; }
    }

    // ---- IME ----
    IMEState imeState() const { return imeState_; }
    bool imeActive() const { return imeState_ == IMEState::Composing; }
    const std::string& imeComposition() const { return imeComp_; }

    void imeStart() { imeState_ = IMEState::Composing; imeComp_.clear(); clearSelection(); notify(); }
    void imeUpdate(const std::string& s) { imeComp_ = s; notify(); }
    void imeEnd(const std::string& result) {
        imeState_ = IMEState::Idle;
        if (!result.empty()) insertText(result);
        imeComp_.clear();
        notify();
    }

    // ---- 拖拽选字 ----
    DragState dragState() const { return dragState_; }
    void mouseDown(bool ctrlHeld) {
        if (!enabled_) return;
        if (ctrlHeld) { selectAll(); return; }
        dragState_ = DragState::Selecting;
        // 简化为移动到末尾（精确位置需要 DWrite 计算）
        cursorPos_ = text_.length();
        selStart_ = selEnd_ = cursorPos_;
    }
    void mouseMove() {
        if (dragState_ == DragState::Selecting) selEnd_ = text_.length();
    }
    void mouseUp() { dragState_ = DragState::None; }

    // ---- 字符输入 ----
    void handleChar(uint32_t ch) {
        if (!enabled_ || isReadOnly()) return;
        if (ch < 32 || ch == 127) return; // 控制字符过滤

        deleteSelection();
        if (text_.length() >= maxLen_) return; // 长度限制

        char mb[4] = {0}; wchar_t wch = static_cast<wchar_t>(ch);
        if (wch <= 0x7F) mb[0] = static_cast<char>(wch);
        else WideCharToMultiByte(CP_UTF8, 0, &wch, 1, mb, 4, nullptr, nullptr);

        text_.insert(cursorPos_, mb);
        cursorPos_ += strlen(mb);
        clearSelection();
        lastAction_ = EditAction::InsertChar;
        notify();
    }

    void insertText(const std::string& s) {
        if (!enabled_ || isReadOnly() || s.empty()) return;
        deleteSelection();
        if (text_.length() >= maxLen_) return;
        text_.insert(cursorPos_, s);
        cursorPos_ += s.length();
        clearSelection();
        lastAction_ = EditAction::InsertText;
        notify();
    }

    // ---- 删除 ----
    void deleteSelection() {
        if (selStart_ == selEnd_) return;
        if (!enabled_ || isReadOnly()) return;
        size_t s = std::min(selStart_, selEnd_), e = std::max(selStart_, selEnd_);
        text_.erase(s, e - s);
        cursorPos_ = s; clearSelection();
        lastAction_ = EditAction::Clear;
        notify();
    }

    void backspace() {
        if (!enabled_ || isReadOnly()) return;
        if (hasSelection()) { deleteSelection(); return; }
        if (cursorPos_ == 0) return;
        text_.erase(cursorPos_ - 1, 1);
        cursorPos_--;
        clearSelection();
        lastAction_ = EditAction::DeleteBack;
        notify();
    }

    void deleteForward() {
        if (!enabled_ || isReadOnly()) return;
        if (hasSelection()) { deleteSelection(); return; }
        if (cursorPos_ >= text_.length()) return;
        text_.erase(cursorPos_, 1);
        clearSelection();
        lastAction_ = EditAction::DeleteForward;
        notify();
    }

    // ---- 光标移动 ----
    void moveCursorLeft(bool extend) {
        if (cursorPos_ > 0) cursorPos_--;
        if (!extend) clearSelection(); else selEnd_ = cursorPos_;
    }
    void moveCursorRight(bool extend) {
        if (cursorPos_ < text_.length()) cursorPos_++;
        if (!extend) clearSelection(); else selEnd_ = cursorPos_;
    }
    void moveCursorHome(bool extend) {
        cursorPos_ = 0;
        if (!extend) clearSelection(); else selEnd_ = 0;
    }
    void moveCursorEnd(bool extend) {
        cursorPos_ = text_.length();
        if (!extend) clearSelection(); else selEnd_ = cursorPos_;
    }

    // ---- 键盘处理器 ----
    void handleKeyDown(int vk) {
        if (!enabled_) return;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        switch (vk) {
            case VK_LEFT:  moveCursorLeft(shift); break;
            case VK_RIGHT: moveCursorRight(shift); break;
            case VK_HOME:  moveCursorHome(shift); break;
            case VK_END:   moveCursorEnd(shift); break;
            case VK_BACK:  backspace(); break;
            case VK_DELETE: deleteForward(); break;
            default: break;
        }
    }

    // ---- 剪贴板操作 ----
    std::string copySelection() const {
        if (!hasSelection()) return "";
        size_t s = std::min(selStart_, selEnd_), e = std::max(selStart_, selEnd_);
        return text_.substr(s, e - s);
    }
    void cutSelection() {
        if (!enabled_ || isReadOnly() || !hasSelection()) return;
        deleteSelection();
        lastAction_ = EditAction::Cut;
    }
    void selectAll() {
        selStart_ = 0; selEnd_ = text_.length(); cursorPos_ = text_.length();
    }

    // ---- 最后操作 ----
    EditAction lastAction() const { return lastAction_; }

private:
    std::string enforceLimit(const std::string& s) const {
        return (maxLen_ != NO_LIMIT && s.length() > maxLen_) ? s.substr(0, maxLen_) : s;
    }

    std::string text_;
    std::string placeholder_;
    std::string imeComp_;
    size_t cursorPos_ = 0, selStart_ = 0, selEnd_ = 0;
    size_t maxLen_ = NO_LIMIT;
    float blink_ = 0; bool caretVisible_ = true;
    InputMode mode_ = InputMode::Normal;
    IMEState imeState_ = IMEState::Idle;
    DragState dragState_ = DragState::None;
    EditAction lastAction_ = EditAction::None;
};

// ============================================================
// ButtonWidgetState — 按钮状态机
// ============================================================
class ButtonWidgetState : public WidgetState {
public:
    enum class VisualState { Normal, Hovered, Pressed, Disabled };

    VisualState visualState() const {
        if (!enabled_) return VisualState::Disabled;
        if (pressed_) return VisualState::Pressed;
        if (hovered_) return VisualState::Hovered;
        return VisualState::Normal;
    }

    void press()   { if (enabled_) { pressed_ = true;  notify(); } }
    void release() { if (pressed_) { pressed_ = false; notify(); } }
    void setText(const std::string& t) { text_ = t; }
    const std::string& text() const { return text_; }

private:
    bool pressed_ = false;
    std::string text_;
};

// ============================================================
// CheckBoxWidgetState — 复选框状态机
// ============================================================
class CheckBoxWidgetState : public WidgetState {
public:
    enum class CheckState { Unchecked, Checked, Indeterminate };

    CheckState checkState() const { return state_; }
    void setCheckState(CheckState s) { state_ = s; notify(); }
    void toggle() {
        if (state_ == CheckState::Indeterminate) state_ = CheckState::Checked;
        else state_ = (state_ == CheckState::Checked) ? CheckState::Unchecked : CheckState::Checked;
        notify();
    }
    bool isChecked() const { return state_ == CheckState::Checked; }
    void setText(const std::string& t) { text_ = t; }
    const std::string& text() const { return text_; }

private:
    CheckState state_ = CheckState::Unchecked;
    std::string text_;
};

// ============================================================
// ToggleWidgetState — 开关控件状态
// ============================================================
class ToggleWidgetState : public WidgetState {
public:
    bool toggled() const { return toggled_; }
    void setToggled(bool v) { toggled_ = v; notify(); }
    void toggle() { toggled_ = !toggled_; notify(); }
    void setLabel(const std::string& l) { label_ = l; }
    const std::string& label() const { return label_; }
private:
    bool toggled_ = false;
    std::string label_;
};

// ============================================================
// ChoicePickerWidgetState — 下拉选择器状态
// ============================================================
class ChoicePickerWidgetState : public WidgetState {
public:
    using IndexChanged = std::function<void(int)>;

    bool isOpen() const { return open_; }
    void open()  { open_ = true;  notify(); }
    void close() { open_ = false; notify(); }
    void toggle() { open_ ? close() : open(); }

    void setOptions(const std::vector<std::string>& opts) { options_ = opts; }
    const std::vector<std::string>& options() const { return options_; }

    int selectedIndex() const { return selIdx_; }
    void selectIndex(int i) {
        if (i >= 0 && i < static_cast<int>(options_.size()) && i != selIdx_) {
            selIdx_ = i; open_ = false;
            if (onIndexChanged_) onIndexChanged_(selIdx_);
            notify();
        }
    }
    const std::string& selectedText() const {
        static std::string empty;
        return (selIdx_ >= 0 && selIdx_ < static_cast<int>(options_.size())) ? options_[selIdx_] : empty;
    }
    void onIndexChanged(IndexChanged cb) { onIndexChanged_ = std::move(cb); }

private:
    std::vector<std::string> options_;
    int selIdx_ = 0;
    bool open_ = false;
    IndexChanged onIndexChanged_;
};

// ============================================================
// SliderWidgetState — 滑动条控件状态
// ============================================================
class SliderWidgetState : public WidgetState {
public:
    using ValueChanged = std::function<void(float)>;

    float value() const { return value_; }
    float minValue() const { return min_; }
    float maxValue() const { return max_; }
    float step() const { return step_; }

    void setRange(float min, float max, float step = 1) {
        min_ = min; max_ = max; step_ = step;
        value_ = std::clamp(value_, min_, max_);
    }
    void setValue(float v) {
        float nv = snapToStep(std::clamp(v, min_, max_));
        if (nv != value_) { value_ = nv; if (onValueChanged_) onValueChanged_(value_); notify(); }
    }
    // 拖拽中临时值（不触发回调）
    void setTempValue(float v) { temp_ = std::clamp(v, min_, max_); notify(); }
    void commitTemp() { setValue(temp_); }
    bool isDragging() const { return dragging_; }
    void setDragging(bool d) {
        if (!d && dragging_) commitTemp();
        dragging_ = d;
    }
    void onValueChanged(ValueChanged cb) { onValueChanged_ = std::move(cb); }

private:
    float snapToStep(float v) const {
        if (step_ <= 0) return v;
        return min_ + std::round((v - min_) / step_) * step_;
    }
    float value_ = 0, temp_ = 0, min_ = 0, max_ = 100, step_ = 1;
    bool dragging_ = false;
    ValueChanged onValueChanged_;
};

} // namespace jui
