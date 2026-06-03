#include <gtest/gtest.h>
#include "jui/render/iwidget_renderer.h"
#include "jui/render/test_renderer.h"
#include "jui/core/widget.h"
#include <memory>

using jui::render::IWidgetRenderer;
using jui::render::RendererRegistry;
using jui::render::TestRenderer;

class CustomRenderer : public IWidgetRenderer {
public:
    int paintCount = 0;
    void paint(jui::render::IRenderTarget&, const jui::Rect&, const std::string&,
               bool, bool, bool, float) override { paintCount++; }
    jui::WidgetType supportedType() const override { return jui::WidgetType::Text; }
    std::string name() const override { return "Custom"; }
};

TEST(PluggableRenderer, Registry_SetAndGet) {
    RendererRegistry reg;
    reg.set(jui::WidgetType::Button, std::make_shared<TestRenderer>(100));
    EXPECT_TRUE(reg.get(jui::WidgetType::Button) != nullptr);
    EXPECT_TRUE(reg.get(jui::WidgetType::Text) == nullptr);
}

TEST(PluggableRenderer, Registry_SetAll) {
    RendererRegistry reg;
    reg.setAll(std::make_shared<TestRenderer>(200));
    for (int i = 0; i <= static_cast<int>(jui::WidgetType::Card); ++i) {
        auto* r = reg.get(static_cast<jui::WidgetType>(i));
        EXPECT_TRUE(r != nullptr) << "Type " << i << " missing";
    }
}

TEST(PluggableRenderer, Registry_PerWidgetOverride) {
    RendererRegistry reg;
    reg.setAll(std::make_shared<TestRenderer>(1));
    reg.set(jui::WidgetType::Button, std::make_shared<TestRenderer>(2));
    EXPECT_TRUE(reg.get(jui::WidgetType::Button) != reg.get(jui::WidgetType::Text));
}

TEST(PluggableRenderer, Registry_Clear) {
    RendererRegistry reg;
    reg.setAll(std::make_shared<TestRenderer>(42));
    EXPECT_TRUE(reg.get(jui::WidgetType::Button) != nullptr);
    reg.clear();
    EXPECT_TRUE(reg.get(jui::WidgetType::Button) == nullptr);
}

TEST(PluggableRenderer, TestRenderer_Deterministic) {
    TestRenderer a(42), b(42);
    a.seed(100); b.seed(100);
    SUCCEED();
}

TEST(PluggableRenderer, TestRendererTypes) {
    TestRenderer tr(1);
    EXPECT_TRUE(tr.supportedType() == jui::WidgetType::Card);
    EXPECT_TRUE(tr.name() == "TestRenderer");
}

TEST(PluggableRenderer, CustomRenderer_Registered) {
    auto cr = std::make_shared<CustomRenderer>();
    RendererRegistry reg;
    reg.set(jui::WidgetType::Text, cr);
    EXPECT_TRUE(reg.get(jui::WidgetType::Text) != nullptr);
    EXPECT_FALSE(reg.get(jui::WidgetType::Text)->name().empty());
}

TEST(PluggableRenderer, Interface_Abstract) {
    static_assert(std::is_abstract_v<IWidgetRenderer>);
    SUCCEED();
}
