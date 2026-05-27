/**
 * render_widget.cpp — 标准控件实现
 *
 * - Text:  DWrite 文字渲染，支持 fontSize/fontWeight/textColor/textAlign
 * - TextField: D2D 编辑框，光标闪烁/选区/剪贴板/IME/右键菜单
 * - Button: D2D 按钮，hover/pressed/disabled 三态
 * - CheckBox/Container/Divider/Image
 */
#include "jui/render/render_widget.h"
#include <algorithm>
#include <cmath>
#include <cwctype>

namespace jui {

// ============================================================
// UTF-8 → UTF-16 正确转换
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
// D2D 颜色辅助
// ============================================================

static D2D1_COLOR_F parseColor(const std::string& hex, float alpha = 1.0f) {
    D2D1_COLOR_F c = {0,0,0,alpha};
    if (hex.empty() || hex.length() < 6) return c;
    std::string h = (hex[0]=='#') ? hex.substr(1) : hex;
    if (h.length()>=6) {
        try {
            uint32_t v = static_cast<uint32_t>(std::stoul(h.substr(0,6), nullptr, 16));
            c.r=((v>>16)&0xFF)/255.0f; c.g=((v>>8)&0xFF)/255.0f; c.b=(v&0xFF)/255.0f;
        } catch(...) {}
    }
    return c;
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
// 通用辅助
// ============================================================

static ComPtr<IDWriteTextFormat> makeTextFormat(
    IDWriteFactory* dwrite, float fontSize, const std::wstring& fontFamily,
    DWRITE_FONT_WEIGHT weight, const std::string& align)
{
    ComPtr<IDWriteTextFormat> fmt;
    DWRITE_TEXT_ALIGNMENT ta = DWRITE_TEXT_ALIGNMENT_LEADING;
    if (align=="center") ta=DWRITE_TEXT_ALIGNMENT_CENTER;
    else if (align=="right") ta=DWRITE_TEXT_ALIGNMENT_TRAILING;

    dwrite->CreateTextFormat(fontFamily.c_str(),nullptr,weight,
        DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,
        fontSize,L"",&fmt);
    if(fmt){fmt->SetTextAlignment(ta);fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);}
    return fmt;
}

// ============================================================
// TextRenderWidget (DWrite 文字)
// ============================================================

ComPtr<IDWriteTextFormat> TextRenderWidget::createTextFormat(IDWriteFactory* dwrite) {
    float fs = widget_->property("fontSize").isNumber() ? widget_->property("fontSize").asNumber() : 14.0f;
    std::string w = widget_->property("fontWeight").isString() ? widget_->property("fontWeight").asString() : "normal";
    std::string a = widget_->property("textAlign").isString() ? widget_->property("textAlign").asString() : "left";
    DWRITE_FONT_WEIGHT fw = (w=="bold")?DWRITE_FONT_WEIGHT_BOLD:DWRITE_FONT_WEIGHT_NORMAL;
    return makeTextFormat(dwrite, fs, L"Microsoft YaHei", fw, a);
}

void TextRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if(!rt||!dwrite) return;
    std::string text;
    auto tv=widget_->property("text"), bv=widget_->boundValue("text");
    if(bv.isString())text=bv.asString(); else if(tv.isString())text=tv.asString();
    else if(tv.isNumber())text=std::to_string(tv.asInt());
    if(text.empty())return;

    std::wstring wtext=utf8ToWide(text);
    auto fmt=createTextFormat(dwrite);
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};

    // 颜色: 支持 textColor 属性
    D2D1_COLOR_F color=D2D1::ColorF(D2D1::ColorF::Black);
    auto tc=widget_->property("textColor");
    if(tc.isString()) color=parseColor(tc.asString());

    ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(color,&brush);
    rt->DrawText(wtext.c_str(),static_cast<UINT32>(wtext.length()),fmt.Get(),r,brush.Get());
}

Size TextRenderWidget::measure(IDWriteFactory* dwrite) {
    std::string text;
    auto tv=widget_->property("text"), bv=widget_->boundValue("text");
    if(bv.isString())text=bv.asString(); else if(tv.isString())text=tv.asString();
    float fs = widget_->property("fontSize").isNumber()?widget_->property("fontSize").asNumber():14.0f;
    std::string fw = widget_->property("fontWeight").isString()?widget_->property("fontWeight").asString():"normal";
    // 估算宽度
    float tw=0;
    for(size_t i=0;i<text.size();){
        unsigned char c=static_cast<unsigned char>(text[i]);
        tw+=fs*(c<0x80?0.6f:1.0f);
        i+=(c<0x80?1:(c&0xE0)==0xC0?2:3);
    }
    return {std::max(40.0f,tw+8),std::max(18.0f,fs*1.5f)};
}

// ============================================================
// TextFieldRenderWidget (D2D 编辑框)
// ============================================================

TextFieldRenderWidget::TextFieldRenderWidget(WidgetPtr widget) : RenderWidget(widget) {
    auto val=widget->property("value"); if(val.isString()) text_=val.asString();
    auto ph=widget->property("placeholder"); if(ph.isString()) placeholder_=ph.asString();
    cursorPos_=text_.length(); selectionStart_=selectionEnd_=cursorPos_;
}

ComPtr<IDWriteTextFormat> TextFieldRenderWidget::createTextFormat(IDWriteFactory* dwrite) {
    return makeTextFormat(dwrite,14,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
}

void TextFieldRenderWidget::deleteSelection() {
    if(selectionStart_==selectionEnd_) return;
    size_t s=std::min(selectionStart_,selectionEnd_);
    size_t e=std::max(selectionStart_,selectionEnd_);
    text_.erase(s,e-s);
    cursorPos_=s; selectionStart_=selectionEnd_=s;
}

// ---- 绘制 ----

void TextFieldRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    bool disabled = !widget_->enabled();
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    float radius=4;

    ComPtr<ID2D1SolidColorBrush> bg;
    if(disabled) rt->CreateSolidColorBrush(D2D1::ColorF(0.92f,0.92f,0.92f),&bg);
    else if(focused_) rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&bg);
    else rt->CreateSolidColorBrush(D2D1::ColorF(0.96f,0.96f,0.96f),&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{r,radius,radius},bg.Get());

    ComPtr<ID2D1SolidColorBrush> border;
    if(disabled) rt->CreateSolidColorBrush(D2D1::ColorF(0.85f,0.85f,0.85f),&border);
    else if(focused_) rt->CreateSolidColorBrush(D2D1::ColorF(0.2f,0.5f,0.9f),&border);
    else rt->CreateSolidColorBrush(D2D1::ColorF(0.7f,0.7f,0.7f),&border);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{r,radius,radius},border.Get(),focused_?2.0f:1.0f);
}

void TextFieldRenderWidget::drawTextContent(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    auto fmt=createTextFormat(dwrite);
    float pad=8;
    D2D1_RECT_F r={bounds_.x+pad,bounds_.y+2,bounds_.x+bounds_.w-pad,bounds_.y+bounds_.h-2};

    bool disabled = !widget_->enabled();
    D2D1_COLOR_F textColor = disabled ? D2D1::ColorF(0.55f,0.55f,0.55f) : D2D1::ColorF(D2D1::ColorF::Black);
    ComPtr<ID2D1SolidColorBrush> textBrush;
    rt->CreateSolidColorBrush(textColor,&textBrush);

    if(!text_.empty()||imeActive_) {
        std::string display=imeActive_?imeComposition_:text_;
        if(!display.empty()){
            std::wstring w= utf8ToWide(display);
            rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),r,textBrush.Get());
        }
    } else if(!placeholder_.empty()) {
        std::wstring w= utf8ToWide(placeholder_);
        ComPtr<ID2D1SolidColorBrush> ph;
        D2D1_COLOR_F phc = disabled ? D2D1::ColorF(0.75f,0.75f,0.75f) : D2D1::ColorF(0.5f,0.5f,0.5f);
        rt->CreateSolidColorBrush(phc,&ph);
        rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),r,ph.Get());
    }
}

void TextFieldRenderWidget::drawSelection(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if(selectionStart_==selectionEnd_||imeActive_) return;
    size_t s=std::min(selectionStart_,selectionEnd_);
    size_t e=std::max(selectionStart_,selectionEnd_);
    float x1=getCharPosition(dwrite,s);
    float x2=getCharPosition(dwrite,e);
    D2D1_RECT_F r={bounds_.x+8+x1,bounds_.y+4,bounds_.x+8+x2,bounds_.y+bounds_.h-4};
    ComPtr<ID2D1SolidColorBrush> sb;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.0f,0.47f,0.84f,0.3f),&sb);
    rt->FillRectangle(r,sb.Get());
}

void TextFieldRenderWidget::drawCaret(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if(!focused_||!caretVisible_||imeActive_) return;
    float cx=getCharPosition(dwrite,cursorPos_);
    D2D1_RECT_F r={bounds_.x+8+cx-1,bounds_.y+5,bounds_.x+8+cx+1,bounds_.y+bounds_.h-5};
    ComPtr<ID2D1SolidColorBrush> cb;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&cb);
    rt->FillRectangle(r,cb.Get());
}

void TextFieldRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    drawBackground(rt);
    drawSelection(rt,dwrite);
    drawTextContent(rt,dwrite);
    drawCaret(rt,dwrite);
}

void TextFieldRenderWidget::update(float dt) {
    if(focused_) {
        caretBlinkTime_+=dt;
        if(caretBlinkTime_>=CARET_BLINK_INTERVAL) {
            caretBlinkTime_-=CARET_BLINK_INTERVAL;
            caretVisible_=!caretVisible_;
        }
    } else {
        caretBlinkTime_=0; caretVisible_=false;
    }
}

Size TextFieldRenderWidget::measure(IDWriteFactory* dwrite) {
    float w=widget_->property("width").isNumber()?widget_->property("width").asNumber():150.0f;
    return {w,30};
}

// ---- 光标位置计算 ----

float TextFieldRenderWidget::getCharPosition(IDWriteFactory* dwrite, size_t index) {
    if(index==0||text_.empty()) return 0;
    if(index>text_.length()) index=text_.length();
    auto fmt=createTextFormat(dwrite);
    std::wstring sub=utf8ToWide(text_.substr(0,index));
    ComPtr<IDWriteTextLayout> layout;
    dwrite->CreateTextLayout(sub.c_str(),static_cast<UINT32>(sub.length()),fmt.Get(),10000,10000,&layout);
    if(!layout) return 0;
    DWRITE_TEXT_METRICS m;
    layout->GetMetrics(&m);
    return m.width;
}

size_t TextFieldRenderWidget::getCharIndexFromPoint(IDWriteFactory* dwrite, float x) {
    if(text_.empty()) return 0;
    auto fmt=createTextFormat(dwrite);
    std::wstring w=utf8ToWide(text_);
    ComPtr<IDWriteTextLayout> layout;
    dwrite->CreateTextLayout(w.c_str(),static_cast<UINT32>(w.length()),fmt.Get(),10000,10000,&layout);
    if(!layout) return 0;
    BOOL trailing=FALSE,inside=FALSE; DWRITE_HIT_TEST_METRICS h;
    layout->HitTestPoint(x,0,&trailing,&inside,&h);
    return h.textPosition+(trailing?1:0);
}

// ---- 鼠标 ----

void TextFieldRenderWidget::onMouseDown(float x, float y) {
    dragging_=true;
    // 简化: 默认全选或点击末尾
    selectionStart_=0; selectionEnd_=text_.length();
    cursorPos_=text_.length();
}

void TextFieldRenderWidget::onMouseMove(float x, float y) {
    if(dragging_) selectionEnd_=text_.length();
}

// ---- 键盘 ----

void TextFieldRenderWidget::onCharInternal(uint32_t ch, bool isIME) {
    if(!focused_||!widget_->enabled()) return;
    if(ch<32&&ch!=8&&ch!=13&&ch!=9&&!isIME) return; // 过滤控制字符
    if(ch==127) return; // DEL

    deleteSelection();

    wchar_t wch=static_cast<wchar_t>(ch);
    char mb[4]={0};
    if(wch<=0x7F) mb[0]=static_cast<char>(wch);
    else WideCharToMultiByte(CP_UTF8,0,&wch,1,mb,4,nullptr,nullptr);
    text_.insert(cursorPos_,mb);
    cursorPos_+=strlen(mb);
    selectionStart_=selectionEnd_=cursorPos_;
}

void TextFieldRenderWidget::onChar(uint32_t ch) {
    if(ch>=32) onCharInternal(ch,false);
}

void TextFieldRenderWidget::onKeyDown(int vk) {
    if(!focused_||!widget_->enabled()) return;

    switch(vk) {
        case VK_LEFT:
            if(cursorPos_>0)cursorPos_--;
            if(!(GetKeyState(VK_SHIFT)&0x8000)) selectionStart_=selectionEnd_=cursorPos_;
            else selectionEnd_=cursorPos_;
            break;
        case VK_RIGHT:
            if(cursorPos_<text_.length())cursorPos_++;
            if(!(GetKeyState(VK_SHIFT)&0x8000))selectionStart_=selectionEnd_=cursorPos_;
            else selectionEnd_=cursorPos_;
            break;
        case VK_HOME:
            cursorPos_=0;
            if(!(GetKeyState(VK_SHIFT)&0x8000))selectionStart_=selectionEnd_=0;
            else selectionEnd_=0;
            break;
        case VK_END:
            cursorPos_=text_.length();
            if(!(GetKeyState(VK_SHIFT)&0x8000))selectionStart_=selectionEnd_=cursorPos_;
            else selectionEnd_=cursorPos_;
            break;
        case VK_BACK: {
            if(selectionStart_!=selectionEnd_) { deleteSelection(); break; }
            if(cursorPos_>0) {
                text_.erase(cursorPos_-1,1); cursorPos_--;
                selectionStart_=selectionEnd_=cursorPos_;
            }
            break;
        }
        case VK_DELETE: {
            if(selectionStart_!=selectionEnd_) { deleteSelection(); break; }
            if(cursorPos_<text_.length()) { text_.erase(cursorPos_,1); }
            selectionStart_=selectionEnd_=cursorPos_;
            break;
        }
        case VK_RETURN:
            // 编辑框单行模式: 忽略回车
            break;
        case VK_TAB:
            // Tab 键由窗口层处理焦点切换
            break;
    }
}

// ---- 剪贴板 ----

std::string TextFieldRenderWidget::copySelection() const {
    if(selectionStart_==selectionEnd_) return "";
    size_t s=std::min(selectionStart_,selectionEnd_);
    size_t e=std::max(selectionStart_,selectionEnd_);
    return text_.substr(s,e-s);
}

void TextFieldRenderWidget::pasteText(const std::string& text) {
    if(text.empty()) return;
    deleteSelection();
    text_.insert(cursorPos_,text);
    cursorPos_+=text.length();
    selectionStart_=selectionEnd_=cursorPos_;
}

void TextFieldRenderWidget::cutSelection() {
    if(selectionStart_==selectionEnd_) return;
    size_t s=std::min(selectionStart_,selectionEnd_);
    size_t e=std::max(selectionStart_,selectionEnd_);
    text_.erase(s,e-s);
    cursorPos_=s; selectionStart_=selectionEnd_=s;
}

void TextFieldRenderWidget::selectAll() {
    selectionStart_=0; selectionEnd_=text_.length(); cursorPos_=text_.length();
}

// ============================================================
// ButtonRenderWidget (D2D 按钮)
// ============================================================

void ButtonRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    if(!rt) return;
    D2D1_RECT_F r={bounds_.x+1,bounds_.y+1,bounds_.x+bounds_.w-1,bounds_.y+bounds_.h-1};
    float radius=6;
    bool disabled=!widget_->enabled();

    // 背景色: pressed > hover > normal > disabled
    D2D1_COLOR_F bgColor;
    if(disabled)       bgColor=D2D1::ColorF(0.78f,0.78f,0.78f);
    else if(pressed_)  bgColor=D2D1::ColorF(0.13f,0.40f,0.80f);
    else if(hovered_)  bgColor=D2D1::ColorF(0.22f,0.54f,0.92f);
    else               bgColor=D2D1::ColorF(0.18f,0.48f,0.88f);

    ComPtr<ID2D1SolidColorBrush> bgBrush;
    rt->CreateSolidColorBrush(bgColor,&bgBrush);
    D2D1_ROUNDED_RECT rr={r,radius,radius};
    rt->FillRoundedRectangle(rr,bgBrush.Get());

    // 文字
    std::string text;
    auto tv=widget_->property("text");
    if(tv.isString()) text=tv.asString();
    if(text.empty()) text="Button";

    std::wstring wtext=utf8ToWide(text);
    auto fmt=makeTextFormat(dwrite,14,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"center");

    D2D1_COLOR_F textColor=disabled?D2D1::ColorF(0.55f,0.55f,0.55f):D2D1::ColorF(D2D1::ColorF::White);
    ComPtr<ID2D1SolidColorBrush> textBrush;
    rt->CreateSolidColorBrush(textColor,&textBrush);
    rt->DrawText(wtext.c_str(),static_cast<UINT32>(wtext.length()),fmt.Get(),r,textBrush.Get());
}

Size ButtonRenderWidget::measure(IDWriteFactory* dwrite) {
    std::string text;
    auto tv=widget_->property("text"); if(tv.isString())text=tv.asString();
    if(text.empty())text="Button";
    float tw=0;
    for(size_t i=0;i<text.size();){
        unsigned char c=static_cast<unsigned char>(text[i]);
        tw+=14*(c<0x80?0.6f:1.0f); i+=(c<0x80?1:(c&0xE0)==0xC0?2:3);
    }
    return {std::max(70.0f,tw+24),32};
}

bool ButtonRenderWidget::hitTest(float x, float y) const {
    return x>=bounds_.x&&x<=bounds_.x+bounds_.w&&y>=bounds_.y&&y<=bounds_.y+bounds_.h;
}

void ButtonRenderWidget::onMouseDown(float x, float y) { pressed_=true; }
void ButtonRenderWidget::onMouseUp(float x, float y) { pressed_=false; }

// ============================================================
// CheckBoxRenderWidget
// ============================================================

void CheckBoxRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    float box=16,yMid=bounds_.y+(bounds_.h-box)/2;
    D2D1_RECT_F br={bounds_.x+4,yMid,bounds_.x+4+box,yMid+box};
    float rr=3;

    ComPtr<ID2D1SolidColorBrush> bg;
    rt->CreateSolidColorBrush(checked_?D2D1::ColorF(0.18f,0.48f,0.88f):D2D1::ColorF(0.95f,0.95f,0.95f),&bg);
    rt->FillRoundedRectangle(D2D1_ROUNDED_RECT{br,rr,rr},bg.Get());

    ComPtr<ID2D1SolidColorBrush> bd;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.55f,0.55f,0.55f),&bd);
    rt->DrawRoundedRectangle(D2D1_ROUNDED_RECT{br,rr,rr},bd.Get(),1.5f);

    if(checked_){
        ComPtr<ID2D1SolidColorBrush> ch;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&ch);
        rt->DrawLine({br.left+3,yMid+box/2},{br.left+box/2-1,yMid+box-3},ch.Get(),1.8f);
        rt->DrawLine({br.left+box/2-1,yMid+box-3},{br.left+box-3,yMid+3},ch.Get(),1.8f);
    }

    std::string label;
    auto lv=widget_->property("text"); if(lv.isString()) label=lv.asString();
    if(!label.empty()){
        std::wstring w=utf8ToWide(label);
        auto f=makeTextFormat(dwrite,13,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"left");
        D2D1_RECT_F tr={br.left+box+6,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
        ComPtr<ID2D1SolidColorBrush> tb;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),&tb);
        rt->DrawText(w.c_str(),static_cast<UINT32>(w.length()),f.Get(),tr,tb.Get());
    }
}

Size CheckBoxRenderWidget::measure(IDWriteFactory* dwrite) {
    std::string l; auto lv=widget_->property("text"); if(lv.isString())l=lv.asString();
    float lw=0; if(!l.empty()){ for(size_t i=0;i<l.size();){unsigned char c=static_cast<unsigned char>(l[i]); lw+=13*(c<0x80?0.6f:1.0f); i+=(c<0x80?1:(c&0xE0)==0xC0?2:3);}}
    return {std::max(100.0f,22+6+lw),24};
}

void CheckBoxRenderWidget::onMouseDown(float x, float y) { checked_=!checked_; }

// ============================================================
// ContainerRenderWidget
// ============================================================

void ContainerRenderWidget::drawBackground(ID2D1RenderTarget* rt) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    switch(widget_->type()){
        case WidgetType::Card:{
            ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.98f,0.98f,0.98f),&bg);
            D2D1_ROUNDED_RECT rr={r,8,8}; rt->FillRoundedRectangle(rr,bg.Get());
            ComPtr<ID2D1SolidColorBrush> bd; rt->CreateSolidColorBrush(D2D1::ColorF(0.85f,0.85f,0.85f),&bd);
            rt->DrawRoundedRectangle(rr,bd.Get(),1.0f);
            break;}
        case WidgetType::List:{
            ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.97f,0.97f,0.97f),&bg);
            rt->FillRectangle(r,bg.Get());
            break;}
        default: break;
    }
}

void ContainerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    drawBackground(rt);
}
Size ContainerRenderWidget::measure(IDWriteFactory* dwrite) { return {bounds_.w,bounds_.h}; }

// ============================================================
// DividerRenderWidget
// ============================================================

void DividerRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    float y=bounds_.y+bounds_.h/2;
    ComPtr<ID2D1SolidColorBrush> br; rt->CreateSolidColorBrush(D2D1::ColorF(0.82f,0.82f,0.82f),&br);
    rt->DrawLine({bounds_.x,y},{bounds_.x+bounds_.w,y},br.Get(),1.0f);
}
Size DividerRenderWidget::measure(IDWriteFactory* dwrite) { return {bounds_.w,8}; }

// ============================================================
// ImageRenderWidget
// ============================================================

void ImageRenderWidget::paint(ID2D1RenderTarget* rt, IDWriteFactory* dwrite) {
    D2D1_RECT_F r={bounds_.x,bounds_.y,bounds_.x+bounds_.w,bounds_.y+bounds_.h};
    ComPtr<ID2D1SolidColorBrush> bg; rt->CreateSolidColorBrush(D2D1::ColorF(0.93f,0.93f,0.93f),&bg);
    rt->FillRectangle(r,bg.Get());
    auto fmt=makeTextFormat(dwrite,24,L"Microsoft YaHei",DWRITE_FONT_WEIGHT_NORMAL,"center");
    ComPtr<ID2D1SolidColorBrush> tb; rt->CreateSolidColorBrush(D2D1::ColorF(0.55f,0.55f,0.55f),&tb);
    rt->DrawText(L"[IMG]",5,fmt.Get(),r,tb.Get());
}
Size ImageRenderWidget::measure(IDWriteFactory* dwrite) {
    float w=widget_->property("width").isNumber()?widget_->property("width").asNumber():64.0f;
    float h=widget_->property("height").isNumber()?widget_->property("height").asNumber():64.0f;
    return {w,h};
}

} // namespace jui
