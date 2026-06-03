#pragma once
#include "jui/render/iwidget_renderer.h"
#include "jui/render/irender_target.h"
#include <random>
#include <vector>
#include <string>

namespace jui::render {

/// 测试渲染器 — 所有控件填充随机半透明颜色
/// 用于黑盒测试中直观验证布局位置和层叠关系
class TestRenderer : public IWidgetRenderer {
public:
    TestRenderer(unsigned int seed = 42) : rng_(seed) {}

    void paint(IRenderTarget& rt, const Rect& bounds,
               const std::string& text, bool enabled,
               bool hovered, bool pressed, float value) override
    {
        (void)text; (void)enabled; (void)hovered; (void)pressed; (void)value;

        // 从种子生成确定性的"随机"颜色（同种子同控件同颜色，便于对比）
        float r = randomF(0.15f, 0.85f);
        float g = randomF(0.15f, 0.85f);
        float b = randomF(0.15f, 0.85f);
        float alpha = 0.45f;  // 半透明，便于看到层叠

        rt.drawRect(bounds, r, g, b, 0, alpha);

        // 绘制控件 ID 作为标签（左上角）
        if (!text.empty() && bounds.w > 40 && bounds.h > 14) {
            rt.drawText(text, Rect{bounds.x + 2, bounds.y + 2,
                                    bounds.w - 4, bounds.h - 4},
                        10.0f, "normal", 1.0f, 1.0f, 1.0f, 0.9f);
        }
    }

    WidgetType supportedType() const override { return WidgetType::Card; }
    std::string name() const override { return "TestRenderer"; }

    /// 重置随机数种子
    void seed(unsigned int s) { rng_.seed(s); }

private:
    float randomF(float lo, float hi) {
        return lo + (hi - lo) * std::uniform_real_distribution<float>(0, 1)(rng_);
    }
    std::mt19937 rng_;
};

} // namespace jui::render
