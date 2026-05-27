#pragma once
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
// RenderWidget — 渲染控件基类（与逻辑控件完全分离）
// 每个逻辑 Widget 对应一个 RenderWidget，负责 D2D 绘制
// ============================================================

class RenderWidget {
public:
    explicit RenderWidget(WidgetPtr widget) : widget_(std::move(widget)) {}
    virtual ~RenderWidget() = default;

    // 绘制（纯 D2D 渲染）
    virtual void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) = 0;

    // 测量
    virtual Size measure(IDWriteFactory* dwrite) = 0;

    // 设置布局边界
    void setBounds(const Rect& r) { bounds_ = r; }
    const Rect& bounds() const { return bounds_; }

    // 命中测试
    virtual bool hitTest(float x, float y) const;

    // 焦点
    virtual bool canFocus() const { return false; }
    void setFocused(bool f) { focused_ = f; }
    bool focused() const { return focused_; }

    // 输入处理
    virtual void onMouseDown(float x, float y) {}
    virtual void onMouseUp(float x, float y) {}
    virtual void onMouseMove(float x, float y) {}
    virtual void onChar(uint32_t ch) {}
    virtual void onKeyDown(int vk) {}
    virtual void onKeyUp(int vk) {}

    WidgetPtr widget() const { return widget_; }

protected:
    WidgetPtr widget_;
    Rect bounds_;
    bool focused_ = false;
    bool hovered_ = false;
};

using RenderWidgetPtr = std::shared_ptr<RenderWidget>;

// ============================================================
// TextRenderWidget — 文本渲染控件（DWrite）
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
// TextFieldRenderWidget — 编辑框渲染控件（D2D+DWrite）
// 支持：文字编辑、光标、选区、IME
// ============================================================
class TextFieldRenderWidget : public RenderWidget {
public:
    explicit TextFieldRenderWidget(WidgetPtr widget);
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
    bool canFocus() const override { return true; }
    void onMouseDown(float x, float y) override;
    void onMouseMove(float x, float y) override;
    void onChar(uint32_t ch) override;
    void onKeyDown(int vk) override;

    std::string text() const { return text_; }
    void setText(const std::string& t) { text_ = t; cursorPos_ = text_.length(); }

private:
    void drawBackground(ID2D1RenderTarget* rt);
    void drawText(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    void drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dwrite);
    float getCharPosition(IDWriteFactory* dwrite, size_t index);
    size_t getCharIndexFromPoint(IDWriteFactory* dwrite, float x);
    ComPtr<IDWriteTextFormat> createTextFormat(IDWriteFactory* dwrite);

    std::string text_;
    std::string placeholder_;
    size_t cursorPos_ = 0;
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;
    bool dragging_ = false;
    float caretBlinkTimer_ = 0;
    bool caretVisible_ = true;
};

// ============================================================
// ButtonRenderWidget — 按钮渲染控件
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
// CheckBoxRenderWidget — 复选框渲染控件
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
// ContainerRenderWidget — 容器渲染控件（Row/Column/Card）
// ============================================================
class ContainerRenderWidget : public RenderWidget {
public:
    explicit ContainerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;

    void drawBackground(ID2D1RenderTarget* rt);
};

// ============================================================
// DividerRenderWidget — 分割线渲染控件
// ============================================================
class DividerRenderWidget : public RenderWidget {
public:
    explicit DividerRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

// ============================================================
// ImageRenderWidget — 图片渲染控件
// ============================================================
class ImageRenderWidget : public RenderWidget {
public:
    explicit ImageRenderWidget(WidgetPtr widget) : RenderWidget(widget) {}
    void paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) override;
    Size measure(IDWriteFactory* dwrite) override;
};

// ============================================================
// RenderWidgetFactory — 根据逻辑控件创建渲染控件
// ============================================================
class RenderWidgetFactory {
public:
    static RenderWidgetPtr create(WidgetPtr widget);
};

} // namespace jui
