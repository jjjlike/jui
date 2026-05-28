#pragma once
#include "widget_state.h"
#include "jui/core/widget.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <map>

namespace jui {

using Microsoft::WRL::ComPtr;

// ============================================================
// RenderWidget — 纯渲染基类（状态管理在 WidgetState 中）
// ============================================================

class RenderWidget {
public:
    explicit RenderWidget(WidgetPtr widget) : widget_(std::move(widget)) {}
    virtual ~RenderWidget() = default;

    // 绘制
    virtual void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) = 0;

    // 测量（使用 DWrite 精确测量，供布局引擎调用）
    virtual Size measure(IDWriteFactory* dwrite) = 0;

    // 布局边界
    void setBounds(const Rect& r) { bounds_ = r; }
    const Rect& bounds() const { return bounds_; }

    // 命中测试
    virtual bool hitTest(float x, float y) const;

    // 状态访问
    WidgetState* state() { return state_.get(); }
    const WidgetState* state() const { return state_.get(); }
    virtual bool canFocus() const { return false; }

    // 剪贴板
    virtual bool canCopy() const { return false; }
    virtual bool canPaste() const { return false; }
    virtual std::string copySelection() const { return ""; }
    virtual void pasteText(const std::string& text) {}
    virtual void cutSelection() {}
    virtual void selectAll() {}

    WidgetPtr widget() const { return widget_; }

protected:
    WidgetPtr widget_;
    Rect bounds_;
    std::unique_ptr<WidgetState> state_;
};

using RenderWidgetPtr = std::shared_ptr<RenderWidget>;

// ============================================================
// TextRenderWidget — DirectWrite 文字渲染
// ============================================================
class TextRenderWidget : public RenderWidget {
public:
    explicit TextRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;

private:
    TextWidgetState& s() { return static_cast<TextWidgetState&>(*state_); }
    const TextWidgetState& s() const { return static_cast<const TextWidgetState&>(*state_); }
    ComPtr<IDWriteTextFormat> createTextFormat(IDWriteFactory* dwrite);
};

// ============================================================
// TextFieldRenderWidget — Direct2D + DirectWrite 编辑框渲染
// （所有逻辑已移至 TextFieldWidgetState）
// ============================================================
class TextFieldRenderWidget : public RenderWidget {
public:
    explicit TextFieldRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;

    // 剪贴板
    bool canCopy() const override;
    bool canPaste() const override { return true; }
    std::string copySelection() const override;
    void pasteText(const std::string& text) override;
    void cutSelection() override;
    void selectAll() override;

    int onKeyDown(int vk);
    void onChar(uint32_t ch);

    // IME 支持
    void imeStart() { s().imeStart(); }
    void imeComposition(const std::string& str) { s().imeUpdate(str); }
    void imeEnd(const std::string& result) { s().imeEnd(result); }
    bool imeActive() const { return s().imeActive(); }

private:
    TextFieldWidgetState& s() { return static_cast<TextFieldWidgetState&>(*state_); }
    const TextFieldWidgetState& s() const { return static_cast<const TextFieldWidgetState&>(*state_); }

    void drawBackground(ID2D1RenderTarget* rt);
    void drawTextContent(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    float getCharPosition(IDWriteFactory* dwrite, size_t index);
    size_t getCharIndexFromPoint(IDWriteFactory* dwrite, float x);
    ComPtr<IDWriteTextFormat> createTextFormat(IDWriteFactory* dwrite);
};

// ============================================================
// ButtonRenderWidget — Direct2D 按钮渲染
// ============================================================
class ButtonRenderWidget : public RenderWidget {
public:
    explicit ButtonRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;

private:
    ButtonWidgetState& s() { return static_cast<ButtonWidgetState&>(*state_); }
    const ButtonWidgetState& s() const { return static_cast<const ButtonWidgetState&>(*state_); }
};

// ============================================================
// CheckBoxRenderWidget
// ============================================================
class CheckBoxRenderWidget : public RenderWidget {
public:
    explicit CheckBoxRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }

private:
    CheckBoxWidgetState& s() { return static_cast<CheckBoxWidgetState&>(*state_); }
    const CheckBoxWidgetState& s() const { return static_cast<const CheckBoxWidgetState&>(*state_); }
};

// ============================================================
// Container / Divider / Image
// ============================================================
class ContainerRenderWidget : public RenderWidget {
public:
    explicit ContainerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    void drawBackground(ID2D1RenderTarget* rt);
};

class DividerRenderWidget : public RenderWidget {
public:
    explicit DividerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

class ImageRenderWidget : public RenderWidget {
public:
    explicit ImageRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

// ============================================================
// ListRenderWidget — 虚拟滚动列表 (D2D)
// ============================================================
class ListRenderWidget : public RenderWidget {
public:
    explicit ListRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;
    void onScroll(float dy);
    void onClick(float x, float y);
    void setSurface(void* s) { (void)s; } // 预留接口


private:
    ListWidgetState& s() { return static_cast<ListWidgetState&>(*state_); }
    const ListWidgetState& s() const { return static_cast<const ListWidgetState&>(*state_); }
};

// ============================================================
// GridRenderWidget — 表格控件 (D2D)
// ============================================================
class TabsRenderWidget : public RenderWidget {
public:
    explicit TabsRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;
    int hitTabHeader(float x) const;
private:
    TabsWidgetState& s() { return static_cast<TabsWidgetState&>(*state_); }
    const TabsWidgetState& s() const { return static_cast<const TabsWidgetState&>(*state_); }
};

class GridRenderWidget : public RenderWidget {
public:
    explicit GridRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;
    void onScrollX(float dx);
    void onScrollY(float dy);
    void onClick(float x, float y);

private:
    GridWidgetState& s() { return static_cast<GridWidgetState&>(*state_); }
    const GridWidgetState& s() const { return static_cast<const GridWidgetState&>(*state_); }
};

// ============================================================
// ToggleRenderWidget — 开关控件 (D2D)
// ============================================================
class ToggleRenderWidget : public RenderWidget {
public:
    explicit ToggleRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
private:
    ToggleWidgetState& s() { return static_cast<ToggleWidgetState&>(*state_); }
};

// ============================================================
// ChoicePickerRenderWidget — 下拉选择器 (D2D)
// ============================================================
class ChoicePickerRenderWidget : public RenderWidget {
public:
    explicit ChoicePickerRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
private:
    ChoicePickerWidgetState& s() { return static_cast<ChoicePickerWidgetState&>(*state_); }
    float itemHeight; // 下拉列表项高度
};

// ============================================================
// SliderRenderWidget — 滑动条控件 (D2D)
// ============================================================
class SliderRenderWidget : public RenderWidget {
public:
    explicit SliderRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    bool hitTest(float x, float y) const override;
private:
    SliderWidgetState& s() { return static_cast<SliderWidgetState&>(*state_); }
};

// ============================================================
// RenderWidgetFactory
// ============================================================
class RenderWidgetFactory {
public:
    static RenderWidgetPtr create(WidgetPtr widget);
};

} // namespace jui
