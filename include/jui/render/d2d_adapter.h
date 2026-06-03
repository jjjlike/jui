#pragma once
#include "jui/render/irender_target.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <windows.h>

namespace jui::render { namespace detail {
inline std::wstring u8toW(const std::string& u8) {
    if (u8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, &w[0], len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}
}}

namespace jui::render {

/// 将原始 D2D 指针适配为 IRenderTarget 接口（零拷贝）
/// 供自定义 IWidgetRenderer 使用
class D2DRenderTargetAdapter : public IRenderTarget {
public:
    D2DRenderTargetAdapter(ID2D1RenderTarget* rt, IDWriteFactory* dw)
        : rt_(rt), dw_(dw) {}

    void beginDraw() override {}
    void endDraw() override {}

    void clear(float r, float g, float b) override {
        if (rt_) rt_->Clear(D2D1::ColorF(r, g, b));
    }

    void drawText(const std::string& text, const Rect& rect,
                  float fontSize, const std::string& fontWeight,
                  float r, float g, float b, float a) override
    {
        if (!rt_ || !dw_ || text.empty()) return;

        DWRITE_FONT_WEIGHT fw = (fontWeight == "bold")
            ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;

        Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
        dw_->CreateTextFormat(L"Microsoft YaHei", nullptr, fw,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fontSize, L"", &fmt);
        if (fmt) {
            fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> br;
        rt_->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &br);

        auto wtext = detail::u8toW(text);
        D2D1_RECT_F d2dR = { rect.x, rect.y, rect.x + rect.w, rect.y + rect.h };
        if (fmt && br)
            rt_->DrawText(wtext.c_str(), static_cast<UINT32>(wtext.length()),
                          fmt.Get(), d2dR, br.Get());
    }

    void drawRect(const Rect& rect, float r, float g, float b,
                  float radius, float a) override
    {
        if (!rt_) return;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> br;
        rt_->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &br);
        if (!br) return;

        D2D1_RECT_F d2dR = { rect.x, rect.y, rect.x + rect.w, rect.y + rect.h };
        if (radius > 0) {
            D2D1_ROUNDED_RECT rr{d2dR, radius, radius};
            rt_->FillRoundedRectangle(rr, br.Get());
        } else {
            rt_->FillRectangle(d2dR, br.Get());
        }
    }

    void drawLine(float x1, float y1, float x2, float y2,
                  float r, float g, float b) override
    {
        if (!rt_) return;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> br;
        rt_->CreateSolidColorBrush(D2D1::ColorF(r, g, b), &br);
        if (br) rt_->DrawLine({x1, y1}, {x2, y2}, br.Get());
    }

    Size measureText(const std::string& text, float fontSize,
                     const std::string& fontWeight) override
    {
        if (!dw_ || text.empty()) return {0, 0};
        DWRITE_FONT_WEIGHT fw = (fontWeight == "bold")
            ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
        dw_->CreateTextFormat(L"Microsoft YaHei", nullptr, fw,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fontSize, L"", &fmt);
        if (!fmt) return {0, 0};

        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        auto wtext = detail::u8toW(text);
        dw_->CreateTextLayout(wtext.c_str(), static_cast<UINT32>(wtext.length()),
            fmt.Get(), 10000, 10000, &layout);
        if (!layout) return {0, 0};

        DWRITE_TEXT_METRICS metrics = {};
        layout->GetMetrics(&metrics);
        return { metrics.width, metrics.height };
    }

    int width() const override { return rt_ ? static_cast<int>(rt_->GetSize().width) : 0; }
    int height() const override { return rt_ ? static_cast<int>(rt_->GetSize().height) : 0; }
    bool isValid() const override { return rt_ != nullptr && dw_ != nullptr; }

private:
    ID2D1RenderTarget* rt_;
    IDWriteFactory* dw_;
};

} // namespace jui::render
