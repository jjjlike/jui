/**
 * render_widget.cpp — 纯 D2D/DWrite 渲染（状态逻辑在 WidgetState 中）
 */
#include "jui/render/render_widget.h"
#include <algorithm>
#include <cmath>

namespace jui {

// ============================================================
// UTF-8 → UTF-16 (DirectWrite 需要)
// ============================================================
static std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) return L"";
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &r[0], len);
    return r;
}

// ============================================================
// DirectWrite 精确测量文本宽度（公共工具函数）
// ============================================================
static float measureTextWidthDW(IDWriteFactory* dwrite, const std::string& utf8,
                                  float fontSize, DWRITE_FONT_WEIGHT fw, float maxW = 10000) {
    if (utf8.empty() || !dwrite) return 0;
    auto w = utf8ToWide(utf8);
    ComPtr<IDWriteTextFormat> fmt;
    dwrite->CreateTextFormat(L"Microsoft YaHei", nullptr, fw,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"", &fmt);
    if (!fmt) return 0;
    ComPtr<IDWriteTextLayout> lo;
    dwrite->CreateTextLayout(w.c_str(), static_cast<UINT32>(w.length()), fmt.Get(), maxW, 10000, &lo);
    if (!lo) return 0;
    DWRITE_TEXT_METRICS m;
    lo->GetMetrics(&m);
    return std::ceilf(m.width);
}

// ============================================================
// D2D 颜色解析
// ============================================================
static D2D1_COLOR_F parseHexColor(const std::string& h, float a = 1.0f) {
    D2D1_COLOR_F c = {0,0,0,a};
    if (h.length() < 6) return c;
    std::string s = (h[0]=='#') ? h.substr(1) : h;
    try {
        uint32_t v = static_cast<uint32_t>(std::stoul(s.substr(0,6), nullptr, 16));
        c.r=((v>>16)&0xFF)/255.0f; c.g=((v>>8)&0xFF)/255.0f; c.b=(v&0xFF)/255.0f;
    } catch(...) {}
    return c;
}

// ============================================================
// DWrite TextFormat 创建
// ============================================================
static ComPtr<IDWriteTextFormat> makeTextFormat(
    IDWriteFactory* dw, float fs, const std::wstring& fam,
    DWRITE_FONT_WEIGHT fw, const std::string& align)
{
    ComPtr<IDWriteTextFormat> f;
    DWRITE_TEXT_ALIGNMENT ta = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (align=="center") ta=DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (align=="right") ta=DWRITE_TEXT_ALIGNMENT_TRAILING;
    dw->CreateTextFormat(fam.c_str(),nullptr,fw,DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,fs,L"",&f);
    if(f){f->SetTextAlignment(ta);f->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);}
    return f;
}

// ============================================================
// RenderWidget 基类
// ============================================================
bool RenderWidget::hitTest(float x, float y) const {
    return x>=bounds_.x&&x<=bounds_.x+bounds_.w&&y>=bounds_.y&&y<=bounds_.y+bounds_.h;
}

// ============================================================
// RenderWidgetFactory
// ============================================================
RenderWidgetPtr RenderWidgetFactory::create(WidgetPtr widget) {
    if (!widget) return nullptr;
    switch (widget->type()) {
        case WidgetType::Text:         return std::make_shared<TextRenderWidget>(widget);
        case WidgetType::Button:       return std::make_shared<ButtonRenderWidget>(widget);
        case WidgetType::TextField:    return std::make_shared<TextFieldRenderWidget>(widget);
        case WidgetType::CheckBox:     return std::make_shared<CheckBoxRenderWidget>(widget);
        case WidgetType::Toggle:       return std::make_shared<ToggleRenderWidget>(widget);
        case WidgetType::ChoicePicker: return std::make_shared<ChoicePickerRenderWidget>(widget);
        case WidgetType::Slider:       return std::make_shared<SliderRenderWidget>(widget);
        case WidgetType::Row: case WidgetType::Column:
        case WidgetType::Card: case WidgetType::List:
            return std::make_shared<ContainerRenderWidget>(widget);
        case WidgetType::Divider:      return std::make_shared<DividerRenderWidget>(widget);
        case WidgetType::Image:        return std::make_shared<ImageRenderWidget>(widget);
        default: return nullptr;
    }
}

// ============================================================
// TextRenderWidget — DirectWrite 文字渲染
// ============================================================
TextRenderWidget::TextRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    state_ = std::make_unique<TextWidgetState>();
    auto tv = widget->property("text");
    if (tv.isString()) s().setText(tv.asString());
    s().setEnabled(widget->enabled());
}

ComPtr<IDWriteTextFormat> TextRenderWidget::createTextFormat(IDWriteFactory* dw) {
    float fs = widget_->property("fontSize").isNumber() ? widget_->property("fontSize").asNumber() : 14.0f;
    std::string w = widget_->property("fontWeight").isString() ? widget_->property("fontWeight").asString() : "normal";
    std::string a = widget_->property("textAlign").isString() ? widget_->property("textAlign").asString() : "left";
    DWRITE_FONT_WEIGHT fw = (w=="bold")?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL;
    return makeTextFormat(dw, fs, L"Microsoft YaHei", fw, a);
}

void TextRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    if (!rt||!dw) return;
    std::string text = s().text();
    if (text.empty()) return;

    auto fmt = createTextFormat(dw);
    D2D1_RECT_F r = {bounds_.x, bounds_.y, bounds_.x+bounds_.w, bounds_.y+bounds_.h};

    D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Black);
    auto tc = widget_->property("textColor");
    if (tc.isString()) color = parseHexColor(tc.asString());

    ComPtr<ID2D1SolidColorBrush> br;
    rt->CreateSolidColorBrush(color, &br);
    auto wtext = utf8ToWide(text);
    rt->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.length()), fmt.Get(), r, br.Get());
}

Size TextRenderWidget::measure(IDWriteFactory* dw) {
    float fs = widget_->property("fontSize").isNumber() ? widget_->property("fontSize").asNumber() : 14.0f;
    std::string w = widget_->property("fontWeight").isString() ? widget_->property("fontWeight").asString() : "normal";
    DWRITE_FONT_WEIGHT fw = (w=="bold")?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL;
    float tw = measureTextWidthDW(dw, s().text(), fs, fw);
    return {std::max(40.0f, tw+8), std::max(18.0f, fs*1.5f)};
}

// ============================================================
// TextFieldRenderWidget — D2D+DWrite 编辑框（纯渲染）
// ============================================================
TextFieldRenderWidget::TextFieldRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    auto st = std::make_unique<TextFieldWidgetState>();
    auto val = widget->property("value");
    if (val.isString()) st->setText(val.asString());
    auto ph = widget->property("placeholder");
    if (ph.isString()) st->setPlaceholder(ph.asString());
    st->setEnabled(widget->enabled());
    state_ = std::move(st);
}

ComPtr<IDWriteTextFormat> TextFieldRenderWidget::createTextFormat(IDWriteFactory* dw) {
    return makeTextFormat(dw, 14, L"Microsoft YaHei", DWRITE_FONT_WEIGHT_NORMAL, "left");
}

bool TextFieldRenderWidget::hitTest(float x, float y) const {
    return x>=bounds_.x&&x<=bounds_.x+bounds_.w&&y>=bounds_.y&&y<=bounds_.y+bounds_.h;
}

// ---- 绘制 ----
void TextFieldRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    float rd=4;
    ComPtr<ID2D1SolidColorBrush> bg;
    if(!s().enabled()) rt->CreateSolidColorBrush(D2D1::ColorF(0.92f,0.92f,0.92f),&bg);
    else if(s().focused()) rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&bg);
    else rt->CreateSolidColorBrush(D2D1::ColorF(0.96f,0.96f,0.96f),&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{r,rd,rd},bg.Get());
    ComPtr<ID2D1SolidColorBrush> bd;
    if(!s().enabled()) rt->CreateSolidColorBrush(D2D1::ColorF(0.85f,0.85f,0.85f),&bd);
    else if(s().focused()) rt->CreateSolidColorBrush(D2D1::ColorF(0.2f,0.5f,0.9f),&bd);
    else rt->CreateSolidColorBrush(D2D1::ColorF(0.7f,0.7f,0.7f),&bd);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{r,rd,rd},bd.Get(),s().focused()?2.0f:1.0f);
}

void TextFieldRenderWidget::drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    if(!s().hasSelection()||s().imeActive()) return;
    size_t a=std::min(s().selectionStart(),s().selectionEnd());
    size_t b=std::max(s().selectionStart(),s().selectionEnd());
    float x1=getCharPosition(dw,a), x2=getCharPosition(dw,b);
    D2D1_RECT_F r={bounds_.x+8+x1,bounds_.y+4,bounds_.x+8+x2,bounds_.y+bounds_.h-4};
    ComPtr<ID2D1SolidColorBrush> sb;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.0f,0.47f,0.84f,0.3f),&sb);
    rt->FillRectangle(r,sb.Get());
}

void TextFieldRenderWidget::drawTextContent(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    auto fmt=createTextFormat(dw);
    D2D1_RECT_F r={bounds_.x+8,bounds_.y+2,bounds_.x+bounds_.w-8,bounds_.y+bounds_.h-2};
    bool dis=!s().enabled();
    D2D1_COLOR_F tc=dis?D2D1::ColorF(0.55f,0.55f,0.55f):D2D1::ColorF(D2D1::ColorF::Black);
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(tc,&tb);
    if(!s().text().empty()||s().imeActive()) {
        std::string display=s().imeActive()?s().imeComposition():s().text();
        if(!display.empty()){
            auto w=utf8ToWide(display);
            rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),r,tb.Get());
        }
    } else if(!s().placeholder().empty()) {
        auto w=utf8ToWide(s().placeholder());
        ComPtr<ID2D1SolidColorBrush> ph;
        rt->CreateSolidColorBrush(dis?D2D1::ColorF(0.75f,0.75f,0.75f):D2D1::ColorF(0.5f,0.5f,0.5f),&ph);
        rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),r,ph.Get());
    }
}

void TextFieldRenderWidget::drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    if(!s().focused()||!s().caretVisible()||s().imeActive()) return;
    float cx=getCharPosition(dw,s().cursorPos());
    D2D1_RECT_F r={bounds_.x+8+cx-1,bounds_.y+5,bounds_.x+8+cx+1,bounds_.y+bounds_.h-5};
    ComPtr<ID2D1SolidColorBrush> cb; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&cb);
    rt->FillRectangle(r,cb.Get());
}

void TextFieldRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    drawBackground(rt); drawSelection(rt,dw); drawTextContent(rt,dw); drawCaret(rt,dw);
}

Size TextFieldRenderWidget::measure(IDWriteFactory* dw) {
    float w=widget_->property("width").isNumber()?widget_->property("width").asNumber():150.0f;
    return {w,30};
}

// ---- 光标位置（DirectWrite 精确计算）----
float TextFieldRenderWidget::getCharPosition(IDWriteFactory* dw, size_t idx) {
    if(idx==0||s().text().empty()) return 0;
    if(idx>s().text().length()) idx=s().text().length();
    auto fmt=createTextFormat(dw);
    auto sub=utf8ToWide(s().text().substr(0,idx));
    ComPtr<IDWriteTextLayout> lo;
    dw->CreateTextLayout(sub.c_str(),static_cast<UINT32>(sub.length()),fmt.Get(),10000,10000,&lo);
    if(!lo) return 0;
    DWRITE_TEXT_METRICS m; lo->GetMetrics(&m);
    return m.width;
}

size_t TextFieldRenderWidget::getCharIndexFromPoint(IDWriteFactory* dw, float x) {
    if(s().text().empty()) return 0;
    auto fmt=createTextFormat(dw);
    auto w=utf8ToWide(s().text());
    ComPtr<IDWriteTextLayout> lo;
    dw->CreateTextLayout(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),10000,10000,&lo);
    if(!lo) return 0;
    BOOL tr=FALSE,in=FALSE; DWRITE_HIT_TEST_METRICS h;
    lo->HitTestPoint(x,0,&tr,&in,&h);
    return h.textPosition+(tr?1:0);
}

// ---- 输入事件（委托给 WidgetState）----
void TextFieldRenderWidget::onChar(uint32_t ch) { s().handleChar(ch); }
int TextFieldRenderWidget::onKeyDown(int vk) { s().handleKeyDown(vk); return 0; }

bool TextFieldRenderWidget::canCopy() const { return s().hasSelection(); }
std::string TextFieldRenderWidget::copySelection() const { return s().copySelection(); }
void TextFieldRenderWidget::pasteText(const std::string& t) { s().insertText(t); }
void TextFieldRenderWidget::cutSelection() { s().cutSelection(); }
void TextFieldRenderWidget::selectAll() { s().selectAll(); }

// ============================================================
// ButtonRenderWidget — D2D 按钮渲染
// ============================================================
ButtonRenderWidget::ButtonRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    auto st = std::make_unique<ButtonWidgetState>();
    auto tv = widget->property("text");
    if (tv.isString()) st->setText(tv.asString());
    st->setEnabled(widget->enabled());
    state_ = std::move(st);
}

void ButtonRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    if(!rt) return;
    D2D1_RECT_F r={bounds_.x+1,bounds_.y+1,bounds_.x+bounds_.w-1,bounds_.y+bounds_.h-1};
    float rd=6; bool dis=!s().enabled();
    D2D1_COLOR_F bgc;
    auto vs = s().visualState();
    if(vs==ButtonWidgetState::VisualState::Disabled)  bgc=D2D1::ColorF(0.78f,0.78f,0.78f);
    else if(vs==ButtonWidgetState::VisualState::Pressed) bgc=D2D1::ColorF(0.13f,0.40f,0.80f);
    else if(vs==ButtonWidgetState::VisualState::Hovered) bgc=D2D1::ColorF(0.22f,0.54f,0.92f);
    else bgc=D2D1::ColorF(0.18f,0.48f,0.88f);
    ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(bgc,&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{r,rd,rd},bg.Get());
    std::string text=s().text(); if(text.empty())text="Button";
    auto wtext=utf8ToWide(text);
    auto fmt=makeTextFormat(dw,14,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"center");
    D2D1_COLOR_F txc=dis?D2D1::ColorF(0.55f,0.55f,0.55f):D2D1::ColorF(D2D1::ColorF::White);
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(txc,&tb);
    rt->DrawText(wtext.c_str(),static_cast<UINT32>(wtext.length()),fmt.Get(),r,tb.Get());
}

Size ButtonRenderWidget::measure(IDWriteFactory* dw) {
    std::string text=s().text(); if(text.empty())text="Button";
    float tw=measureTextWidthDW(dw,text,14,DWRITE_FONT_WEIGHT_NORMAL);
    return {std::max(70.0f,tw+24),32};
}

bool ButtonRenderWidget::hitTest(float x, float y) const {
    return x>=bounds_.x&&x<=bounds_.x+bounds_.w&&y>=bounds_.y&&y<=bounds_.y+bounds_.h;
}

// ============================================================
// CheckBoxRenderWidget
// ============================================================
CheckBoxRenderWidget::CheckBoxRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    auto st = std::make_unique<CheckBoxWidgetState>();
    auto tv = widget->property("text");
    if (tv.isString()) st->setText(tv.asString());
    state_ = std::move(st);
}

void CheckBoxRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    float bx=16,ym=bounds_.y+(bounds_.h-bx)/2;
    D2D1_RECT_F br={bounds_.x+4,ym,bounds_.x+4+bx,ym+bx}; float rr=3;
    ComPtr<ID2D1SolidColorBrush> bg;
    rt->CreateSolidColorBrush(s().isChecked()?D2D1::ColorF(0.18f,0.48f,0.88f):D2D1::ColorF(0.95f,0.95f,0.95f),&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{br,rr,rr},bg.Get());
    ComPtr<ID2D1SolidColorBrush> bd; rt->CreateSolidColorBrush(D2D1::ColorF(0.55f,0.55f,0.55f),&bd);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{br,rr,rr},bd.Get(),1.5f);
    if(s().isChecked()){
        ComPtr<ID2D1SolidColorBrush> ck; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&ck);
        rt->DrawLine({br.left+3,ym+bx/2},{br.left+bx/2-1,ym+bx-3},ck.Get(),1.8f);
        rt->DrawLine({br.left+bx/2-1,ym+bx-3},{br.left+bx-3,ym+3},ck.Get(),1.8f);
    }
    if(!s().text().empty()){
        auto w=utf8ToWide(s().text());
        auto f=makeTextFormat(dw,13,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
        D2D1_RECT_F tr={br.left+bx+6,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
        ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&tb);
        rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),f.Get(),tr,tb.Get());
    }
}

Size CheckBoxRenderWidget::measure(IDWriteFactory* dw) {
    float tw = measureTextWidthDW(dw,s().text(),13,DWRITE_FONT_WEIGHT_NORMAL);
    return {std::max(100.0f,22+6+tw),24};
}

// ============================================================
// ToggleRenderWidget — 开关渲染
// ============================================================
ToggleRenderWidget::ToggleRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    state_ = std::make_unique<ToggleWidgetState>();
    auto tv = widget->property("label"); if (tv.isString()) s().setLabel(tv.asString());
    auto val = widget->property("value");
    if (val.isBoolean()) s().setToggled(val.asBool());
    s().setEnabled(widget->enabled());
}

void ToggleRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    float tw=36, th=20, tx=bounds_.x+2, ty=bounds_.y+(bounds_.h-th)/2;
    float knobR=8, knobX=tx+(s().toggled()?tw-th+4:4);
    D2D1_ROUNDED_RECT bgR = {{tx,ty,tx+tw,ty+th}, th/2, th/2};

    D2D1_COLOR_F bgc = s().enabled()
        ? (s().toggled() ? D2D1::ColorF(0.18f,0.48f,0.88f) : D2D1::ColorF(0.75f,0.75f,0.75f))
        : D2D1::ColorF(0.85f,0.85f,0.85f);
    ComPtr<ID2D1SolidColorBrush> br; rt->CreateSolidColorBrush(bgc,&br);
    rt->FillRoundedRectangle(bgR,br.Get());

    D2D1_ELLIPSE knob = {{knobX+knobR,ty+th/2},knobR-1,knobR-1};
    ComPtr<ID2D1SolidColorBrush> kw; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&kw);
    rt->FillEllipse(knob,kw.Get());

    if(!s().label().empty()){
        auto w=utf8ToWide(s().label());
        auto f=makeTextFormat(dw,13,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
        D2D1_RECT_F lr={tx+tw+8,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
        ComPtr<ID2D1SolidColorBrush> lbr; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&lbr);
        rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),f.Get(),lr,lbr.Get());
    }
}

Size ToggleRenderWidget::measure(IDWriteFactory* dw) {
    float tw = 36 + 8 + measureTextWidthDW(dw,s().label(),13,DWRITE_FONT_WEIGHT_NORMAL);
    return {std::max(60.0f,tw),22};
}

// ============================================================
// ChoicePickerRenderWidget — 下拉选择器渲染
// ============================================================
ChoicePickerRenderWidget::ChoicePickerRenderWidget(WidgetPtr widget)
    : RenderWidget(widget), itemHeight(24) {
    auto st = std::make_unique<ChoicePickerWidgetState>();
    // 从 widget 属性读取 options 和 selected
    state_ = std::move(st);
}

void ChoicePickerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    float rd=4;
    ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.96f,0.96f,0.96f),&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{r,rd,rd},bg.Get());
    ComPtr<ID2D1SolidColorBrush> bd; rt->CreateSolidColorBrush(D2D1::ColorF(0.7f,0.7f,0.7f),&bd);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{r,rd,rd},bd.Get(),1.0f);

    // 箭头 ▼
    auto w= utf8ToWide("\xE2\x96\xBC");
    auto fmt=makeTextFormat(dw,12,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"right");
    D2D1_RECT_F ar={r.left,r.top,r.right-6,r.bottom};
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(D2D1::ColorF(0.4f,0.4f,0.4f),&tb);
    rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),ar,tb.Get());

    // 选中文字
    if(!s().selectedText().empty()){
        auto tw=utf8ToWide(s().selectedText());
        auto tf=makeTextFormat(dw,14,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
        D2D1_RECT_F tr={r.left+8,r.top,r.right-24,r.bottom};
        ComPtr<ID2D1SolidColorBrush> tb2; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&tb2);
        rt->DrawText(tw.c_str(),static_cast<UINT32>(tw.length()),tf.Get(),tr,tb2.Get());
    }

    // 下拉列表
    if(s().isOpen()){
        float ly=bounds_.y+bounds_.h+2;
        float lh=itemHeight*s().options().size()+4;
        D2D1_RECT_F lr={bounds_.x,ly,bounds_.x+bounds_.w,ly+lh};
        ComPtr<ID2D1SolidColorBrush> lbg; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&lbg);
        rt->FillRectangle(lr,lbg.Get());
        ComPtr<ID2D1SolidColorBrush> lbr; rt->CreateSolidColorBrush(D2D1::ColorF(0.6f,0.6f,0.6f),&lbr);
        rt->DrawRectangle(lr,lbr.Get(),1.0f);

        auto ifmt=makeTextFormat(dw,13,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
        for(size_t i=0;i<s().options().size();i++){
            D2D1_RECT_F ir={bounds_.x+4,ly+2+i*itemHeight,bounds_.x+bounds_.w-4,ly+2+(i+1)*itemHeight};
            if(static_cast<int>(i)==s().selectedIndex()){
                ComPtr<ID2D1SolidColorBrush> sb; rt->CreateSolidColorBrush(D2D1::ColorF(0.9f,0.95f,1.0f),&sb);
                rt->FillRectangle(ir,sb.Get());
            }
            auto ow=utf8ToWide(s().options()[i]);
            ComPtr<ID2D1SolidColorBrush> ob; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&ob);
            rt->DrawText(ow.c_str(),static_cast<UINT32>(ow.length()),ifmt.Get(),ir,ob.Get());
        }
    }
}

Size ChoicePickerRenderWidget::measure(IDWriteFactory* dw) {
    return {120,30};
}

// ============================================================
// SliderRenderWidget — 滑动条渲染
// ============================================================
SliderRenderWidget::SliderRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    auto st = std::make_unique<SliderWidgetState>();
    float min=0,max=100;
    auto mv = widget->property("min"); if(mv.isNumber()) min=static_cast<float>(mv.asNumber());
    auto xv = widget->property("max"); if(xv.isNumber()) max=static_cast<float>(xv.asNumber());
    st->setRange(min,max);
    auto vv = widget->property("value"); if(vv.isNumber()) st->setValue(static_cast<float>(vv.asNumber()));
    state_ = std::move(st);
}

void SliderRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    float trackH=4, trackY=bounds_.y+bounds_.h/2-trackH/2;
    float padding=8, trackX=bounds_.x+padding, trackW=bounds_.w-padding*2;
    float knobR=s().isDragging()?8:6;

    // 轨道背景
    D2D1_RECT_F track={trackX,trackY,trackX+trackW,trackY+trackH};
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(D2D1::ColorF(0.85f,0.85f,0.85f),&tb);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{track,trackH/2,trackH/2},tb.Get());

    // 已填充轨道
    float ratio=(s().value()-s().minValue())/(s().maxValue()-s().minValue());
    if(ratio>0){
        D2D1_RECT_F fill={trackX,trackY,trackX+trackW*ratio,trackY+trackH};
        ComPtr<ID2D1SolidColorBrush> fb; rt->CreateSolidColorBrush(D2D1::ColorF(0.18f,0.48f,0.88f),&fb);
        rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{fill,trackH/2,trackH/2},fb.Get());
    }

    // 滑块
    float kx=trackX+trackW*ratio-knobR;
    D2D1_ELLIPSE knob={{kx+knobR,trackY+trackH/2},knobR,knobR};
    ComPtr<ID2D1SolidColorBrush> kb; rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&kb);
    rt->FillEllipse(knob,kb.Get());
    ComPtr<ID2D1SolidColorBrush> kr; rt->CreateSolidColorBrush(D2D1::ColorF(0.18f,0.48f,0.88f),&kr);
    rt->DrawEllipse(knob,kr.Get(),1.5f);
}

Size SliderRenderWidget::measure(IDWriteFactory* dw) {
    return {150,24};
}

bool SliderRenderWidget::hitTest(float x, float y) const {
    // 扩大点击区域
    return x>=bounds_.x-4&&x<=bounds_.x+bounds_.w+4&&y>=bounds_.y-8&&y<=bounds_.y+bounds_.h+8;
}

// ============================================================
// ContainerRenderWidget
// ============================================================
void ContainerRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    switch(widget_->type()){
        case WidgetType::Card:{
            ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.98f,0.98f,0.98f),&bg);
            rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{r,8,8},bg.Get());
            ComPtr<ID2D1SolidColorBrush> bd; rt->CreateSolidColorBrush(D2D1::ColorF(0.85f,0.85f,0.85f),&bd);
            rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{r,8,8},bd.Get(),1.0f);
            break;}
        case WidgetType::List:{
            ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.97f,0.97f,0.97f),&bg);
            rt->FillRectangle(r,bg.Get()); break;}
        default: break;
    }
}
void ContainerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory*) { drawBackground(rt); }
Size ContainerRenderWidget::measure(IDWriteFactory*) { return {bounds_.w,bounds_.h}; }

// ============================================================
// DividerRenderWidget
// ============================================================
void DividerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory*) {
    float y=bounds_.y+bounds_.h/2;
    ComPtr<ID2D1SolidColorBrush> br; rt->CreateSolidColorBrush(D2D1::ColorF(0.82f,0.82f,0.82f),&br);
    rt->DrawLine({bounds_.x,y},{bounds_.x+bounds_.w,y},br.Get(),1.0f);
}
Size DividerRenderWidget::measure(IDWriteFactory*) { return {bounds_.w,8}; }

// ============================================================
// ImageRenderWidget
// ============================================================
void ImageRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dw) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.93f,0.93f,0.93f),&bg);
    rt->FillRectangle(r,bg.Get());
    auto fmt=makeTextFormat(dw,24,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"center");
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(D2D1::ColorF(0.55f,0.55f,0.55f),&tb);
    rt->DrawText(L"[IMG]",5,fmt.Get(),r,tb.Get());
}
Size ImageRenderWidget::measure(IDWriteFactory*) {
    float w=widget_->property("width").isNumber()?widget_->property("width").asNumber():64.0f;
    float h=widget_->property("height").isNumber()?widget_->property("height").asNumber():64.0f;
    return {w,h};
}

} // namespace jui
