#pragma once
#include "jui/core/widget.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <map>
#include <vector>

namespace jui {

using Microsoft::WRL::ComPtr;

// ============================================================
// RenderWidget — 渲染控件基类
// ============================================================

class RenderWidget {
public:
    explicit RenderWidget(WidgetPtr widget) : widget_(std::move(widget)) {}
    virtual ~RenderWidget() = default;

    // 绘制
    virtual void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) = 0;

    // 测量
    virtual Size measure(IDWriteFactory* dwrite) = 0;

    // 每一帧更新（delta 秒，用于动画如光标闪烁）
    virtual void update(float dt) {}

    // 布局边界
    void setBounds(const Rect& r) { bounds_ = r; }
    const Rect& bounds() const { return bounds_; }

    // 命中测试
    virtual bool hitTest(float x, float y) const;

    // 焦点
    virtual bool canFocus() const { return false; }
    void setFocused(bool f) { focused_ = f; }
    bool focused() const { return focused_; }

    // 悬停
    void setHovered(bool h) { hovered_ = h; }
    bool hovered() const { return hovered_; }

    // 输入
    virtual void onMouseDown(float x, float y) {}
    virtual void onMouseUp(float x, float y) {}
    virtual void onMouseMove(float x, float y) {}
    virtual void onChar(uint32_t ch) {}
    virtual void onKeyDown(int vk) {}
    virtual void onKeyUp(int vk) {}
    virtual bool onContextMenu(float x, float y) { return false; }

    // 剪贴板（由 D2DRenderer 调用）
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
    bool focused_ = false;
    bool hovered_ = false;
};

using RenderWidgetPtr = std::shared_ptr<RenderWidget>;

// ============================================================
// TextRenderWidget — 文字控件 (DWrite)
// ============================================================
class TextRenderWidget : public RenderWidget {
public:
    explicit TextRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;

private:
    ComPtr<IDWriteTextFormat> createTextFormat(IDWriteFactory* dwrite);
};

// ============================================================
// TextFieldRenderWidget — 编辑框控件 (D2D + DWrite)
// ============================================================
class TextFieldRenderWidget : public RenderWidget {
public:
    explicit TextFieldRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    void update(float dt) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    void onMouseDown(float x, float y) override;
    void onMouseMove(float x, float y) override;
    void onChar(uint32_t ch) override;
    void onKeyDown(int vk) override;
    bool onContextMenu(float x, float y) override { return true; }

    // 剪贴板
    bool canCopy() const override { return selectionStart_ != selectionEnd_; }
    bool canPaste() const override { return true; }
    std::string copySelection() const override;
    void pasteText(const std::string& text) override;
    void cutSelection() override;
    void selectAll() override;

    std::string text() const { return text_; }
    void setText(const std::string& t) { text_ = t; cursorPos_ = t.length(); selectionStart_ = selectionEnd_ = cursorPos_; }

private:
    void drawBackground(ID2D1RenderTarget* rt);
    void drawTextContent(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    float getCharPosition(IDWriteFactory* dwrite, size_t index);
    size_t getCharIndexFromPoint(IDWriteFactory* dwrite, float x);
    void deleteSelection();
    ComPtr<IDWriteTextFormat> createTextFormat(IDWriteFactory* dwrite);
    void onCharInternal(uint32_t ch, bool isIME);

    std::string text_;
    std::string placeholder_;
    std::string imeComposition_; // IME 合成中的文本

    size_t cursorPos_ = 0;
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;
    bool dragging_ = false;
    bool imeActive_ = false;

    // 光标闪烁: 每 500ms 切换一次
    float caretBlinkTime_ = 0;
    static constexpr float CARET_BLINK_INTERVAL = 0.53f;
    bool caretVisible_ = true;
};

// ============================================================
// ButtonRenderWidget — 按钮控件 (D2D + DWrite)
// ============================================================
class ButtonRenderWidget : public RenderWidget {
public:
    explicit ButtonRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    void onMouseDown(float x, float y) override;
    void onMouseUp(float x, float y) override;
    bool hitTest(float x, float y) const override;

private:
    bool pressed_ = false;
};

// ============================================================
// CheckBoxRenderWidget
// ============================================================
class CheckBoxRenderWidget : public RenderWidget {
public:
    explicit CheckBoxRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    void onMouseDown(float x, float y) override;

private:
    bool checked_ = false;
};

// ============================================================
// ContainerRenderWidget (Row/Column/Card/List)
// ============================================================
class ContainerRenderWidget : public RenderWidget {
public:
    explicit ContainerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    void drawBackground(ID2D1RenderTarget* rt);
};

// ============================================================
// DividerRenderWidget
// ============================================================
class DividerRenderWidget : public RenderWidget {
public:
    explicit DividerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

// ============================================================
// ImageRenderWidget
// ============================================================
class ImageRenderWidget : public RenderWidget {
public:
    explicit ImageRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

// ============================================================
// RenderWidgetFactory
// ============================================================
class RenderWidgetFactory {
public:
    static RenderWidgetPtr create(WidgetPtr widget);
};

} // namespace jui
