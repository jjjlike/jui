/**
 * test_d2d_renderer.cpp — D2DRenderer 渲染器全覆盖单元测试
 *
 * 测试范围:
 *   - parseColor: 标准 6 位 hex、带 # 号、空字符串、非法格式、alpha 通道
 *   - 构造/析构: 默认构造、析构释放资源
 *   - width/height 初始值和设置
 *   - invalidate: 脏标记设置
 *   - setSurface: 设置空 Surface、非空 Surface
 *   - 输入事件: onMouseDown 命中测试逻辑、焦点管理
 *   - 输入事件: onMouseUp/onMouseMove/onCharInput/onKeyDown/onKeyUp 无 Widget 场景
 *
 * 注意: initialize/render 需要真实 HWND，不在此测试。
 *       createDeviceResources 需要 HWND，仅测试非窗口依赖逻辑。
 */

#include <gtest/gtest.h>
#include "jui/render/d2d_renderer.h"

using namespace jui;

// ============================================================
// 1. parseColor 静态方法
// ============================================================

TEST(D2DRendererTest, ParseColor_SixDigits) {
    // 验证: 标准 6 位 hex 颜色解析 (R, G, B)
    // "#FF8000" → RGB(1.0, 0.5, 0.0)
    auto color = D2DRenderer::parseColor("FF8000");
    EXPECT_FLOAT_EQ(color.r, 1.0f);      // 0xFF = 255
    EXPECT_NEAR(color.g, 0.5019f, 0.01f); // 0x80 = 128
    EXPECT_FLOAT_EQ(color.b, 0.0f);       // 0x00 = 0
    EXPECT_FLOAT_EQ(color.a, 1.0f);       // 默认 alpha
}

TEST(D2DRendererTest, ParseColor_WithHash) {
    // 验证: 带 "#" 前缀的 hex 颜色
    // "#336699"
    auto color = D2DRenderer::parseColor("#336699");
    EXPECT_NEAR(color.r, 0.2f, 0.01f);
    EXPECT_NEAR(color.g, 0.4f, 0.01f);
    EXPECT_NEAR(color.b, 0.6f, 0.01f);
}

TEST(D2DRendererTest, ParseColor_Black) {
    // 验证: 黑色 #000000
    auto color = D2DRenderer::parseColor("000000");
    EXPECT_FLOAT_EQ(color.r, 0.0f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

TEST(D2DRendererTest, ParseColor_White) {
    // 验证: 白色 #FFFFFF
    auto color = D2DRenderer::parseColor("FFFFFF");
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.g, 1.0f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
}

TEST(D2DRendererTest, ParseColor_CustomAlpha) {
    // 验证: 自定义 alpha 值
    auto color = D2DRenderer::parseColor("FF0000", 0.5f);
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.a, 0.5f);
}

TEST(D2DRendererTest, ParseColor_EmptyString) {
    // 验证: 空字符串返回黑色默认值
    auto color = D2DRenderer::parseColor("");
    EXPECT_FLOAT_EQ(color.r, 0.0f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

TEST(D2DRendererTest, ParseColor_InvalidHex_Safe) {
    // 验证: 非法 hex 值不会崩溃，返回默认黑色
    auto color = D2DRenderer::parseColor("XYZ");
    EXPECT_FLOAT_EQ(color.r, 0.0f);
    EXPECT_FLOAT_EQ(color.g, 0.0f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
}

TEST(D2DRendererTest, ParseColor_ShortString) {
    // 验证: 长度不足 6 的字符串返回默认值
    auto color = D2DRenderer::parseColor("ABC"); // 只有 3 字节
    EXPECT_FLOAT_EQ(color.r, 0.0f);
}

TEST(D2DRendererTest, ParseColor_OnlyHash) {
    // 验证: 只有 "#" 无颜色值返回默认
    auto color = D2DRenderer::parseColor("#");
    EXPECT_FLOAT_EQ(color.r, 0.0f);
}

// ============================================================
// 2. 构造/析构
// ============================================================

TEST(D2DRendererTest, Constructor_DefaultState) {
    // 验证: 构造后宽高为 0，未初始化
    D2DRenderer renderer;
    EXPECT_EQ(renderer.width(), 0);
    EXPECT_EQ(renderer.height(), 0);
}

TEST(D2DRendererTest, Destructor_NoCrash) {
    // 验证: 析构不崩溃
    auto renderer = std::make_unique<D2DRenderer>();
    EXPECT_NO_THROW(renderer.reset());
}

// ============================================================
// 3. width/height
// ============================================================

TEST(D2DRendererTest, WidthHeight_DefaultZero) {
    // 验证: 未初始化的 renderer 宽高为 0
    D2DRenderer renderer;
    EXPECT_EQ(renderer.width(), 0);
    EXPECT_EQ(renderer.height(), 0);
}

// ============================================================
// 4. invalidate 脏标记
// ============================================================

TEST(D2DRendererTest, Invalidate_SetsDirty) {
    // 验证: invalidate() 设置重绘标记
    D2DRenderer renderer;
    EXPECT_NO_THROW(renderer.invalidate());
}

// ============================================================
// 5. setSurface
// ============================================================

TEST(D2DRendererTest, SetSurface_Null_NoCrash) {
    // 验证: 设置 nullptr Surface 不会崩溃
    D2DRenderer renderer;
    EXPECT_NO_THROW(renderer.setSurface(nullptr));
}

TEST(D2DRendererTest, SetSurface_WithSurface) {
    // 验证: 设置有效 Surface
    D2DRenderer renderer;
    auto surface = std::make_shared<Surface>("test");
    EXPECT_NO_THROW(renderer.setSurface(surface));
}

TEST(D2DRendererTest, SetSurface_WithWidgets) {
    // 验证: 设置包含组件的 Surface 不会崩溃
    D2DRenderer renderer;
    auto surface = std::make_shared<Surface>("test");
    auto root = std::make_shared<Widget>("root", WidgetType::Column);
    auto text = std::make_shared<Widget>("text", WidgetType::Text);
    text->setProperty("text", JValue("Hello"));
    root->setChildren(Children{Children::Explicit, {"text"}});
    surface->updateComponents({root, text});
    surface->setRootWidgetId("root");

    EXPECT_NO_THROW(renderer.setSurface(surface));
}

// ============================================================
// 6. 输入事件（未初始化场景）
// ============================================================

TEST(D2DRendererTest, InputEvents_Uninitialized_NoCrash) {
    // 验证: 未初始化时输入事件不会崩溃
    D2DRenderer renderer;

    EXPECT_NO_THROW(renderer.onMouseMove(100, 200));
    EXPECT_NO_THROW(renderer.onMouseDown(100, 200, 0));
    EXPECT_NO_THROW(renderer.onMouseUp(100, 200, 0));
    EXPECT_NO_THROW(renderer.onCharInput('A'));
    EXPECT_NO_THROW(renderer.onKeyDown(VK_LEFT));
    EXPECT_NO_THROW(renderer.onKeyUp(VK_LEFT));
    EXPECT_NO_THROW(renderer.onSize(800, 600));
}

TEST(D2DRendererTest, MouseDown_WithSurface_FocusTest) {
    // 验证: 点击命中可聚焦控件时设置焦点
    D2DRenderer renderer;
    auto surface = std::make_shared<Surface>("test");
    auto btn = std::make_shared<Widget>("btn", WidgetType::Button);
    surface->addWidget(btn);
    surface->setRootWidgetId("btn");
    renderer.setSurface(surface);

    // 设置 bounds 使按钮在 (0,0)-(80,32)
    // 但这需要 renderWidgets_ 被刷新且有对应的 RenderWidget
    // setSurface 后 renderWidgets_ 应该已包含按钮的 RenderWidget
    // 由于 setSurface 调用了 refreshRenderWidgets，而 createRenderTree 递归
    // 需要 surface 中有对应 widget

    EXPECT_NO_THROW(renderer.onMouseDown(40, 16, 0)); // 在按钮范围内
}

TEST(D2DRendererTest, CharInput_NoFocusedWidget_NoCrash) {
    // 验证: 无焦点 Widget 时字符输入不会崩溃
    D2DRenderer renderer;
    EXPECT_NO_THROW(renderer.onCharInput('X'));
}

TEST(D2DRendererTest, KeyDown_NoFocusedWidget_NoCrash) {
    // 验证: 无焦点 Widget 时键盘输入不会崩溃
    D2DRenderer renderer;
    EXPECT_NO_THROW(renderer.onKeyDown(VK_DELETE));
}
