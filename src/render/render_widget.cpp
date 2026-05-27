#include "jui/render/render_widget.h"
#include <algorithm>
#include <cmath>
#include <cwctype>

namespace jui {

// ============================================================
// UTF-8 → UTF-16 正确转换（修复中文乱码）
// ============================================================
static std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                                   nullptr, 0);
    if (len <= 0) return L"";
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                         &result[0], len);
    return result;
}

// ============================================================
// RenderWidget 基类
// ============================================================

bool RenderWidget::hitTest(float x, float y) const {
    return x >= bounds_.x && x <= bounds_.x + bounds_.w &&
           y >= bounds_.y && y <= bounds_.y + bounds_.h;
}

// ============================================================
// RenderWidgetFactory
// ============================================================

RenderWidgetPtr RenderWidgetFactory::create(WidgetPtr widget) {
    if (!widget) return nullptr;
    switch (widget->type()) {
        case WidgetType::Text:      return std::make_shared<TextRenderWidget>(widget);
        case WidgetType::Button:    return std::make_shared<ButtonRenderWidget>(widget);
        case WidgetType::TextField: return std::make_shared<TextFieldRenderWidget>(widget);
        case WidgetType::CheckBox:  return std::make_shared<CheckBoxRenderWidget>(widget);
        case WidgetType::Row:
        case WidgetType::Column:
        case WidgetType::Card:
        case WidgetType::List:      return std::make_shared<ContainerRenderWidget>(widget);
        case WidgetType::Divider:   return std::make_shared<DividerRenderWidget>(widget);
        case WidgetType::Image:     return std::make_shared<ImageRenderWidget>(widget);
        default: return nullptr;
    }
}

// ============================================================
// 辅助函数
// ============================================================

static ComPtr<IDWriteTextFormat> makeTextFormat(
    IDWriteFactory* dwrite, float fontSize = 14,
    const std::wstring& fontFamily = L"Microsoft YaHei",
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
    const std::string& align = "left")
{
    ComPtr<IDWriteTextFormat> fmt;
    DWRITE_TEXT_ALIGNMENT ta = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (align == "center") ta = DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (align == "right") ta = DWRITE_TEXT_ALIGNMENT_TRAILING;

    dwrite->CreateTextFormat(fontFamily.c_str(), nullptr, weight,
                              DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                              fontSize, L"", &fmt);
    if (fmt) {
        fmt->SetTextAlignment(ta);
        fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    return fmt;
}

/// 使用 DWrite 测量 UTF-8 文本的实际宽度
static float measureTextWidth(IDWriteFactory* dwrite, const std::string& utf8Text,
                               float fontSize, const std::string& fontWeight = "normal",
                               float maxWidth = 10000.0f)
{
    if (utf8Text.empty() || !dwrite) return 0;
    std::wstring wtext = utf8ToWide(utf8Text);

    DWRITE_FONT_WEIGHT fw = (fontWeight == "bold") ? DWRITE_FONT_WEIGHT_BOLD
                                                    : DWRITE_FONT_WEIGHT_NORMAL;
    ComPtr<IDWriteTextFormat> fmt;
    dwrite->CreateTextFormat(L"Microsoft YaHei", nullptr, fw,
                              DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                              fontSize, L"zh-CN", &fmt);
    if (!fmt) return 0;

    ComPtr<IDWriteTextLayout> layout;
    dwrite->CreateTextLayout(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                              fmt.Get(), maxWidth, 10000, &layout);
    if (!layout) return 0;

    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);
    return std::ceilf(metrics.width);
}

// ============================================================
// TextRenderWidget
// ============================================================

ComPtr<IDWriteTextFormat> TextRenderWidget::createTextFormat(IDWriteFactory* dwrite) {
    float fontSize = widget_->property("fontSize").isNumber()
                     ? widget_->property("fontSize").asNumber() : 14.0f;
    std::string weight = widget_->property("fontWeight").isString()
                         ? widget_->property("fontWeight").asString() : "normal";
    std::string align = widget_->property("textAlign").isString()
                        ? widget_->property("textAlign").asString() : "left";

    DWRITE_FONT_WEIGHT fw = (weight == "bold") ? DWRITE_FONT_WEIGHT_BOLD
                                                : DWRITE_FONT_WEIGHT_NORMAL;
    return makeTextFormat(dwrite, fontSize, L"Microsoft YaHei", fw, align);
}

void TextRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if (!rt || !dwrite) return;

    std::string text;
    auto textVal = widget_->property("text");
    auto boundVal = widget_->boundValue("text");

    if (boundVal.isString()) text = boundVal.asString();
    else if (textVal.isString()) text = textVal.asString();
    else if (textVal.isNumber()) text = std::to_string(textVal.asInt());

    if (text.empty()) return;

    std::wstring wtext = utf8ToWide(text);  // ← 修复: 正确 UTF-8 → UTF-16
    auto fmt = createTextFormat(dwrite);

    D2D1_RECT_F rect = {bounds_.x, bounds_.y, bounds_.x + bounds_.w, bounds_.y + bounds_.h};

    ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);

    rt->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                  fmt.Get(), rect, brush.Get());
}

Size TextRenderWidget::measure(IDWriteFactory* dwrite) {
    // 使用 DWrite 精确测量文本宽度
    std::string text;
    auto textVal = widget_->property("text");
    auto boundVal = widget_->boundValue("text");
    if (boundVal.isString()) text = boundVal.asString();
    else if (textVal.isString()) text = textVal.asString();

    float fontSize = widget_->property("fontSize").isNumber()
                     ? widget_->property("fontSize").asNumber() : 14.0f;
    std::string weight = widget_->property("fontWeight").isString()
                         ? widget_->property("fontWeight").asString() : "normal";

    float textW = dwrite ? measureTextWidth(dwrite, text, fontSize, weight) : 100;
    return {std::max(40.0f, textW), std::max(18.0f, fontSize * 1.5f)};
}

// ============================================================
// TextFieldRenderWidget
// ============================================================

TextFieldRenderWidget::TextFieldRenderWidget(WidgetPtr widget)
    : RenderWidget(widget)
{
    auto val = widget->property("value");
    if (val.isString()) text_ = val.asString();

    auto ph = widget->property("placeholder");
    if (ph.isString()) placeholder_ = ph.asString();

    cursorPos_ = text_.length();
}

ComPtr<IDWriteTextFormat> TextFieldRenderWidget::createTextFormat(IDWriteFactory* dwrite) {
    return makeTextFormat(dwrite, 14, L"Microsoft YaHei");
}

void TextFieldRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    D2D1_RECT_F rect = {bounds_.x, bounds_.y, bounds_.x + bounds_.w, bounds_.y + bounds_.h};

    ComPtr<ID2D1SolidColorBrush> bgBrush;
    if (focused_) {
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &bgBrush);
    } else {
        rt->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.96f, 0.96f), &bgBrush);
    }
    rt->FillRectangle(rect, bgBrush.Get());

    ComPtr<ID2D1SolidColorBrush> borderBrush;
    if (focused_) {
        rt->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.5f, 0.9f), &borderBrush);
    } else {
        rt->CreateSolidColorBrush(D2D1::ColorF(0.75f, 0.75f, 0.75f), &borderBrush);
    }
    rt->DrawRectangle(rect, borderBrush.Get(), focused_ ? 1.5f : 1.0f);
}

void TextFieldRenderWidget::drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if (selectionStart_ == selectionEnd_) return;
    size_t start = (std::min)(selectionStart_, selectionEnd_);
    size_t end = (std::max)(selectionStart_, selectionEnd_);

    float x1 = getCharPosition(dwrite, start);
    float x2 = getCharPosition(dwrite, end);

    D2D1_RECT_F selRect = {
        bounds_.x + 6 + x1, bounds_.y + 4,
        bounds_.x + 6 + x2, bounds_.y + bounds_.h - 4
    };

    ComPtr<ID2D1SolidColorBrush> selBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.5f, 0.9f, 0.3f), &selBrush);
    rt->FillRectangle(selRect, selBrush.Get());
}

void TextFieldRenderWidget::drawText(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    auto fmt = createTextFormat(dwrite);
    D2D1_RECT_F rect = {
        bounds_.x + 6, bounds_.y + 2,
        bounds_.x + bounds_.w - 6, bounds_.y + bounds_.h - 2
    };

    if (!text_.empty()) {
        std::wstring wtext = utf8ToWide(text_);  // ← 修复
        ComPtr<ID2D1SolidColorBrush> textBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &textBrush);
        rt->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                      fmt.Get(), rect, textBrush.Get());
    } else if (!placeholder_.empty()) {
        std::wstring wph = utf8ToWide(placeholder_);  // ← 修复
        ComPtr<ID2D1SolidColorBrush> phBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &phBrush);
        rt->DrawText(wph.c_str(), static_cast<UINT32>(wph.length()),
                      fmt.Get(), rect, phBrush.Get());
    }
}

void TextFieldRenderWidget::drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if (!focused_) return;
    float cx = getCharPosition(dwrite, cursorPos_);

    D2D1_RECT_F caretRect = {
        bounds_.x + 6 + cx - 1, bounds_.y + 4,
        bounds_.x + 6 + cx + 1, bounds_.y + bounds_.h - 4
    };

    ComPtr<ID2D1SolidColorBrush> caretBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &caretBrush);
    rt->FillRectangle(caretRect, caretBrush.Get());
}

void TextFieldRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    drawBackground(rt);
    drawSelection(rt, dwrite);
    drawText(rt, dwrite);
    drawCaret(rt, dwrite);
}

Size TextFieldRenderWidget::measure(IDWriteFactory* dwrite) {
    float w = widget_->property("width").isNumber()
              ? widget_->property("width").asNumber() : 150.0f;
    return {w, 28};
}

float TextFieldRenderWidget::getCharPosition(IDWriteFactory* dwrite, size_t index) {
    if (index == 0 || text_.empty()) return 0;
    if (index > text_.length()) index = text_.length();

    auto fmt = createTextFormat(dwrite);
    std::wstring wsub = utf8ToWide(text_.substr(0, index));  // ← 修复: UTF-8 子串

    ComPtr<IDWriteTextLayout> layout;
    dwrite->CreateTextLayout(wsub.c_str(), static_cast<UINT32>(wsub.length()),
                              fmt.Get(), 10000, 10000, &layout);
    if (!layout) return 0;

    DWRITE_TEXT_METRICS metrics;
    layout->GetMetrics(&metrics);
    return metrics.width;
}

size_t TextFieldRenderWidget::getCharIndexFromPoint(IDWriteFactory* dwrite, float x) {
    if (text_.empty()) return 0;

    auto fmt = createTextFormat(dwrite);
    std::wstring wtext = utf8ToWide(text_);  // ← 修复

    ComPtr<IDWriteTextLayout> layout;
    dwrite->CreateTextLayout(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                              fmt.Get(), 10000, 10000, &layout);
    if (!layout) return 0;

    BOOL trailing = FALSE, inside = FALSE;
    DWRITE_HIT_TEST_METRICS hitMetrics;
    layout->HitTestPoint(x, 0, &trailing, &inside, &hitMetrics);

    return hitMetrics.textPosition + (trailing ? 1 : 0);
}

void TextFieldRenderWidget::onMouseDown(float x, float y) {
    selectionStart_ = selectionEnd_ = text_.length();
    dragging_ = true;
}

void TextFieldRenderWidget::onMouseMove(float x, float y) {
    if (dragging_) {
        selectionEnd_ = text_.length();
    }
}

void TextFieldRenderWidget::onChar(uint32_t ch) {
    if (!focused_) return;
    if (ch >= 32 && ch != 127) {
        if (selectionStart_ != selectionEnd_) {
            size_t start = (std::min)(selectionStart_, selectionEnd_);
            size_t end = (std::max)(selectionStart_, selectionEnd_);
            text_.erase(start, end - start);
            cursorPos_ = start;
            selectionStart_ = selectionEnd_ = cursorPos_;
        }

        wchar_t wch = static_cast<wchar_t>(ch);
        char mb[4] = {0};
        if (wch <= 0x7F) {
            mb[0] = static_cast<char>(wch);
        } else {
            WideCharToMultiByte(CP_UTF8, 0, &wch, 1, mb, 4, nullptr, nullptr);
        }
        text_.insert(cursorPos_, mb);
        cursorPos_ += strlen(mb);
        selectionStart_ = selectionEnd_ = cursorPos_;
    }
}

void TextFieldRenderWidget::onKeyDown(int vk) {
    if (!focused_) return;

    switch (vk) {
        case VK_LEFT:
            if (cursorPos_ > 0) cursorPos_--;
            if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                selectionStart_ = selectionEnd_ = cursorPos_;
            } else {
                selectionEnd_ = cursorPos_;
            }
            break;
        case VK_RIGHT:
            if (cursorPos_ < text_.length()) cursorPos_++;
            if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                selectionStart_ = selectionEnd_ = cursorPos_;
            } else {
                selectionEnd_ = cursorPos_;
            }
            break;
        case VK_HOME:
            cursorPos_ = 0;
            selectionStart_ = selectionEnd_ = cursorPos_;
            break;
        case VK_END:
            cursorPos_ = text_.length();
            selectionStart_ = selectionEnd_ = cursorPos_;
            break;
        case VK_BACK:
            if (selectionStart_ != selectionEnd_) {
                size_t start = (std::min)(selectionStart_, selectionEnd_);
                size_t end = (std::max)(selectionStart_, selectionEnd_);
                text_.erase(start, end - start);
                cursorPos_ = start;
                selectionStart_ = selectionEnd_ = cursorPos_;
            } else if (cursorPos_ > 0) {
                text_.erase(cursorPos_ - 1, 1);
                cursorPos_--;
                selectionStart_ = selectionEnd_ = cursorPos_;
            }
            break;
        case VK_DELETE:
            if (selectionStart_ != selectionEnd_) {
                size_t start = (std::min)(selectionStart_, selectionEnd_);
                size_t end = (std::max)(selectionStart_, selectionEnd_);
                text_.erase(start, end - start);
                cursorPos_ = start;
            } else if (cursorPos_ < text_.length()) {
                text_.erase(cursorPos_, 1);
            }
            selectionStart_ = selectionEnd_ = cursorPos_;
            break;
    }
}

// ============================================================
// ButtonRenderWidget
// ============================================================

void ButtonRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    D2D1_RECT_F rect = {bounds_.x, bounds_.y, bounds_.x + bounds_.w, bounds_.y + bounds_.h};
    float radius = 4;

    ComPtr<ID2D1SolidColorBrush> bgBrush;
    D2D1_COLOR_F bgColor = pressed_
        ? D2D1::ColorF(0.15f, 0.45f, 0.85f)
        : (hovered_ ? D2D1::ColorF(0.25f, 0.55f, 0.95f)
                     : D2D1::ColorF(0.2f, 0.5f, 0.9f));
    rt->CreateSolidColorBrush(bgColor, &bgBrush);

    D2D1_ROUNDED_RECT roundedRect = {rect, radius, radius};
    rt->FillRoundedRectangle(roundedRect, bgBrush.Get());

    std::string text;
    auto tv = widget_->property("text");
    if (tv.isString()) text = tv.asString();
    if (text.empty()) text = "Button";

    std::wstring wtext = utf8ToWide(text);  // ← 修复
    auto fmt = makeTextFormat(dwrite, 14, L"Microsoft YaHei",
                               DWRITE_FONT_WEIGHT_NORMAL, "center");

    ComPtr<ID2D1SolidColorBrush> textBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &textBrush);

    rt->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                  fmt.Get(), rect, textBrush.Get());
}

Size ButtonRenderWidget::measure(IDWriteFactory* dwrite) {
    // 根据文字宽度动态计算按钮宽度
    std::string text;
    auto tv = widget_->property("text");
    if (tv.isString()) text = tv.asString();
    if (text.empty()) text = "Button";

    float textW = dwrite ? measureTextWidth(dwrite, text, 14) : 80;
    return {std::max(60.0f, textW + 24), 32};
}

bool ButtonRenderWidget::hitTest(float x, float y) const {
    return x >= bounds_.x && x <= bounds_.x + bounds_.w &&
           y >= bounds_.y && y <= bounds_.y + bounds_.h;
}

void ButtonRenderWidget::onMouseDown(float x, float y) {
    pressed_ = true;
}

void ButtonRenderWidget::onMouseUp(float x, float y) {
    pressed_ = false;
}

// ============================================================
// CheckBoxRenderWidget
// ============================================================

void CheckBoxRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    float boxSize = 16;
    float boxX = bounds_.x + 4;
    float boxY = bounds_.y + (bounds_.h - boxSize) / 2;

    D2D1_RECT_F boxRect = {boxX, boxY, boxX + boxSize, boxY + boxSize};
    float radius = 3;

    ComPtr<ID2D1SolidColorBrush> boxBg;
    D2D1_COLOR_F bgColor = checked_
        ? D2D1::ColorF(0.2f, 0.5f, 0.9f)
        : D2D1::ColorF(0.95f, 0.95f, 0.95f);
    rt->CreateSolidColorBrush(bgColor, &boxBg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{boxRect, radius, radius}, boxBg.Get());

    ComPtr<ID2D1SolidColorBrush> borderBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &borderBrush);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{boxRect, radius, radius}, borderBrush.Get());

    if (checked_) {
        ComPtr<ID2D1SolidColorBrush> checkBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &checkBrush);

        float pad = 3;
        D2D1_POINT_2F p1 = {boxX + pad, boxY + boxSize / 2};
        D2D1_POINT_2F p2 = {boxX + boxSize / 2 - 1, boxY + boxSize - pad};
        D2D1_POINT_2F p3 = {boxX + boxSize - pad, boxY + pad};
        rt->DrawLine(p1, p2, checkBrush.Get(), 1.5f);
        rt->DrawLine(p2, p3, checkBrush.Get(), 1.5f);
    }

    std::string label;
    auto lv = widget_->property("text");
    if (lv.isString()) label = lv.asString();

    if (!label.empty()) {
        std::wstring wlabel = utf8ToWide(label);  // ← 修复
        auto fmt = makeTextFormat(dwrite, 13, L"Microsoft YaHei");
        D2D1_RECT_F textRect = {boxX + boxSize + 6, bounds_.y,
                                 bounds_.x + bounds_.w, bounds_.y + bounds_.h};
        ComPtr<ID2D1SolidColorBrush> textBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &textBrush);
        rt->DrawText(wlabel.c_str(), static_cast<UINT32>(wlabel.length()),
                      fmt.Get(), textRect, textBrush.Get());
    }
}

Size CheckBoxRenderWidget::measure(IDWriteFactory* dwrite) {
    std::string label;
    auto lv = widget_->property("text");
    if (lv.isString()) label = lv.asString();

    float labelW = 0;
    if (dwrite && !label.empty()) {
        labelW = measureTextWidth(dwrite, label, 13) + 22 + 6; // box + gap
    }
    return {std::max(100.0f, labelW), 24};
}

void CheckBoxRenderWidget::onMouseDown(float x, float y) {
    checked_ = !checked_;
}

// ============================================================
// ContainerRenderWidget
// ============================================================

void ContainerRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    D2D1_RECT_F rect = {bounds_.x, bounds_.y, bounds_.x + bounds_.w, bounds_.y + bounds_.h};

    switch (widget_->type()) {
        case WidgetType::Card: {
            ComPtr<ID2D1SolidColorBrush> bg;
            rt->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.98f, 0.98f), &bg);
            D2D1_ROUNDED_RECT rr = {rect, 8, 8};
            rt->FillRoundedRectangle(rr, bg.Get());

            ComPtr<ID2D1SolidColorBrush> border;
            rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f), &border);
            rt->DrawRoundedRectangle(rr, border.Get(), 1.0f);
            break;
        }
        case WidgetType::List: {
            ComPtr<ID2D1SolidColorBrush> bg;
            rt->CreateSolidColorBrush(D2D1::ColorF(0.97f, 0.97f, 0.97f), &bg);
            rt->FillRectangle(rect, bg.Get());
            break;
        }
        default:
            break;
    }
}

void ContainerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    drawBackground(rt);
}

Size ContainerRenderWidget::measure(IDWriteFactory* dwrite) {
    return {bounds_.w, bounds_.h};
}

// ============================================================
// DividerRenderWidget
// ============================================================

void DividerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    float midY = bounds_.y + bounds_.h / 2;
    ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f), &brush);
    rt->DrawLine({bounds_.x, midY}, {bounds_.x + bounds_.w, midY}, brush.Get(), 1.0f);
}

Size DividerRenderWidget::measure(IDWriteFactory* dwrite) {
    return {bounds_.w, 8}; // 加一些垂直间距
}

// ============================================================
// ImageRenderWidget
// ============================================================

void ImageRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    D2D1_RECT_F rect = {bounds_.x, bounds_.y, bounds_.x + bounds_.w, bounds_.y + bounds_.h};

    ComPtr<ID2D1SolidColorBrush> bg;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.93f, 0.93f, 0.93f), &bg);
    rt->FillRectangle(rect, bg.Get());

    // 站位文字（不依赖编码）
    auto fmt = makeTextFormat(dwrite, 24, L"Microsoft YaHei",
                               DWRITE_FONT_WEIGHT_NORMAL, "center");
    ComPtr<ID2D1SolidColorBrush> txtBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f), &txtBrush);
    rt->DrawText(L"[IMG]", 5, fmt.Get(), rect, txtBrush.Get());
}

Size ImageRenderWidget::measure(IDWriteFactory* dwrite) {
    float w = widget_->property("width").isNumber()
              ? widget_->property("width").asNumber() : 64.0f;
    float h = widget_->property("height").isNumber()
              ? widget_->property("height").asNumber() : 64.0f;
    return {w, h};
}

} // namespace jui
