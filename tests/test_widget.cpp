/**
 * test_widget.cpp — Widget 纯逻辑控件全覆盖单元测试
 *
 * 测试范围:
 *   - Rect/Size/Point 结构体
 *   - Widget 构造: 所有 WidgetType 枚举值
 *   - 属性系统: setProperty/get/has/properties
 *   - 子组件: Children Explicit/Template 模式
 *   - 绑定值: setBoundValue/boundValue
 *   - 布局缓存: setLayoutBounds/layoutBounds
 *   - 事件: setAction/action
 *   - 可见性/启用: setVisible/visible setEnabled/enabled
 *   - WidgetStyle 结构体
 *   - WidgetPtr 智能指针
 */

#include <gtest/gtest.h>
#include "jui/core/widget.h"

using namespace jui;

// ============================================================
// 1. Rect 结构体
// ============================================================

TEST(RectTest, DefaultConstructor_Zeros) {
    // 验证: Rect 默认构造全部为 0
    Rect r;
    EXPECT_FLOAT_EQ(r.x, 0.0f);
    EXPECT_FLOAT_EQ(r.y, 0.0f);
    EXPECT_FLOAT_EQ(r.w, 0.0f);
    EXPECT_FLOAT_EQ(r.h, 0.0f);
    EXPECT_TRUE(r.isEmpty());
}

TEST(RectTest, IsEmpty_PositiveDimensions) {
    // 验证: 正尺寸时 isEmpty() 返回 false
    Rect r{10, 20, 100, 50};
    EXPECT_FALSE(r.isEmpty());
}

TEST(RectTest, IsEmpty_ZeroWidth) {
    // 验证: 宽度 <= 0 时 isEmpty() 返回 true
    Rect r{0, 0, 0, 100};
    EXPECT_TRUE(r.isEmpty());
}

TEST(RectTest, IsEmpty_ZeroHeight) {
    // 验证: 高度 <= 0 时 isEmpty() 返回 true
    Rect r{0, 0, 100, 0};
    EXPECT_TRUE(r.isEmpty());
}

TEST(RectTest, IsEmpty_NegativeDimensions) {
    // 验证: 负尺寸时 isEmpty() 返回 true
    Rect r{0, 0, -10, -5};
    EXPECT_TRUE(r.isEmpty());
}

// ============================================================
// 2. Size 结构体
// ============================================================

TEST(SizeTest, DefaultConstructor_Zeros) {
    // 验证: Size 默认值为 0
    Size s;
    EXPECT_FLOAT_EQ(s.w, 0.0f);
    EXPECT_FLOAT_EQ(s.h, 0.0f);
}

TEST(SizeTest, ValueInitialization) {
    // 验证: Size 的列表初始化
    Size s{1920.0f, 1080.0f};
    EXPECT_FLOAT_EQ(s.w, 1920.0f);
    EXPECT_FLOAT_EQ(s.h, 1080.0f);
}

// ============================================================
// 3. Point 结构体
// ============================================================

TEST(PointTest, DefaultConstructor_Zeros) {
    // 验证: Point 默认值为 0
    Point p;
    EXPECT_FLOAT_EQ(p.x, 0.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}

// ============================================================
// 4. WidgetStyle 结构体
// ============================================================

TEST(WidgetStyleTest, Defaults) {
    // 验证: WidgetStyle 各字段的默认值
    WidgetStyle s;
    EXPECT_FLOAT_EQ(s.width, 0.0f);       // auto
    EXPECT_FLOAT_EQ(s.height, 0.0f);      // auto
    EXPECT_FLOAT_EQ(s.flex, 0.0f);
    EXPECT_FLOAT_EQ(s.fontSize, 14.0f);
    EXPECT_EQ(s.fontWeight, "normal");
    EXPECT_EQ(s.textAlign, "left");
    EXPECT_FLOAT_EQ(s.borderRadius, 0.0f);
}

TEST(WidgetStyleTest, MarginPadding_Sets) {
    // 验证: margin/padding 数组默认值为 0
    WidgetStyle s;
    for (int i = 0; i < 4; i++) {
        EXPECT_FLOAT_EQ(s.margin[i], 0.0f);
        EXPECT_FLOAT_EQ(s.padding[i], 0.0f);
    }
}

// ============================================================
// 5. Children 结构体
// ============================================================

TEST(ChildrenTest, DefaultMode_Explicit) {
    // 验证: Children 默认模式是 Explicit
    Children c;
    EXPECT_EQ(c.mode, Children::Explicit);
    EXPECT_TRUE(c.explicitList.empty());
}

TEST(ChildrenTest, ExplicitList_Add) {
    // 验证: explicitList 可以添加子组件 ID
    Children c;
    c.explicitList = {"child1", "child2", "child3"};
    EXPECT_EQ(c.mode, Children::Explicit);
    EXPECT_EQ(c.explicitList.size(), 3);
    EXPECT_EQ(c.explicitList[0], "child1");
    EXPECT_EQ(c.explicitList[2], "child3");
}

TEST(ChildrenTest, Template_DataBinding) {
    // 验证: Template 模式设置 dataBinding 和 componentId
    Children c;
    c.mode = Children::Template;
    c.templateInfo.dataBinding = "/items";
    c.templateInfo.componentId = "item-template";

    EXPECT_EQ(c.mode, Children::Template);
    EXPECT_EQ(c.templateInfo.dataBinding, "/items");
    EXPECT_EQ(c.templateInfo.componentId, "item-template");
}

// ============================================================
// 6. Widget 构造
// ============================================================

TEST(WidgetTest, Constructor_Id) {
    // 验证: 构造后 id() 返回正确的 ID
    auto w = std::make_shared<Widget>("my-widget", WidgetType::Button);
    EXPECT_EQ(w->id(), "my-widget");
}

TEST(WidgetTest, Constructor_Type_Text) {
    // 验证: Text 类型 Widget
    auto w = std::make_shared<Widget>("t1", WidgetType::Text);
    EXPECT_EQ(w->type(), WidgetType::Text);
}

TEST(WidgetTest, Constructor_Type_Button) {
    // 验证: Button 类型
    auto w = std::make_shared<Widget>("b1", WidgetType::Button);
    EXPECT_EQ(w->type(), WidgetType::Button);
}

TEST(WidgetTest, Constructor_Type_TextField) {
    // 验证: TextField 类型
    auto w = std::make_shared<Widget>("tf1", WidgetType::TextField);
    EXPECT_EQ(w->type(), WidgetType::TextField);
}

TEST(WidgetTest, Constructor_Type_CheckBox) {
    // 验证: CheckBox 类型
    auto w = std::make_shared<Widget>("cb1", WidgetType::CheckBox);
    EXPECT_EQ(w->type(), WidgetType::CheckBox);
}

TEST(WidgetTest, Constructor_Type_Image) {
    // 验证: Image 类型
    auto w = std::make_shared<Widget>("img1", WidgetType::Image);
    EXPECT_EQ(w->type(), WidgetType::Image);
}

TEST(WidgetTest, Constructor_Type_Row) {
    // 验证: Row 布局类型
    auto w = std::make_shared<Widget>("row1", WidgetType::Row);
    EXPECT_EQ(w->type(), WidgetType::Row);
}

TEST(WidgetTest, Constructor_Type_Column) {
    // 验证: Column 布局类型
    auto w = std::make_shared<Widget>("col1", WidgetType::Column);
    EXPECT_EQ(w->type(), WidgetType::Column);
}

TEST(WidgetTest, Constructor_Type_Card) {
    // 验证: Card 容器类型
    auto w = std::make_shared<Widget>("card1", WidgetType::Card);
    EXPECT_EQ(w->type(), WidgetType::Card);
}

TEST(WidgetTest, Constructor_Type_List) {
    // 验证: List 类型
    auto w = std::make_shared<Widget>("list1", WidgetType::List);
    EXPECT_EQ(w->type(), WidgetType::List);
}

TEST(WidgetTest, Constructor_Type_Divider) {
    // 验证: Divider 类型
    auto w = std::make_shared<Widget>("div1", WidgetType::Divider);
    EXPECT_EQ(w->type(), WidgetType::Divider);
}

TEST(WidgetTest, Constructor_Type_Custom) {
    // 验证: Custom 类型（用于未知组件）
    auto w = std::make_shared<Widget>("c1", WidgetType::Custom);
    EXPECT_EQ(w->type(), WidgetType::Custom);
}

// ============================================================
// 7. 属性系统
// ============================================================

TEST(WidgetTest, SetProperty_String) {
    // 验证: 设置字符串属性
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setProperty("text", JValue("Hello, World!"));
    EXPECT_TRUE(w->hasProperty("text"));
    EXPECT_EQ(w->property("text").asString(), "Hello, World!");
}

TEST(WidgetTest, SetProperty_Number) {
    // 验证: 设置数值属性
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setProperty("fontSize", JValue(16.0));
    EXPECT_TRUE(w->hasProperty("fontSize"));
    EXPECT_DOUBLE_EQ(w->property("fontSize").asNumber(), 16.0);
}

TEST(WidgetTest, SetProperty_Boolean) {
    // 验证: 设置布尔属性
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setProperty("enabled", JValue(true));
    EXPECT_TRUE(w->hasProperty("enabled"));
    EXPECT_TRUE(w->property("enabled").asBool());
}

TEST(WidgetTest, Property_NonExistent_ReturnsNull) {
    // 验证: 读取不存在的属性返回 Null 值（静态引用）
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    EXPECT_FALSE(w->hasProperty("nonexistent"));
    EXPECT_TRUE(w->property("nonexistent").isNull());
}

TEST(WidgetTest, SetProperty_Overwrite) {
    // 验证: 覆盖已有属性
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setProperty("text", JValue("old"));
    w->setProperty("text", JValue("new"));
    EXPECT_EQ(w->property("text").asString(), "new");
}

TEST(WidgetTest, Properties_ReturnsAll) {
    // 验证: properties() 返回完整的属性 map
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setProperty("a", JValue(1));
    w->setProperty("b", JValue(2));
    const auto& props = w->properties();
    EXPECT_EQ(props.size(), 2);
}

// ============================================================
// 8. 子组件
// ============================================================

TEST(WidgetTest, SetChildren_Explicit) {
    // 验证: 设置静态子组件列表
    auto w = std::make_shared<Widget>("row", WidgetType::Row);
    Children c;
    c.mode = Children::Explicit;
    c.explicitList = {"a", "b", "c"};
    w->setChildren(c);

    const auto& result = w->children();
    EXPECT_EQ(result.mode, Children::Explicit);
    EXPECT_EQ(result.explicitList.size(), 3);
    EXPECT_EQ(result.explicitList[1], "b");
}

TEST(WidgetTest, SetChildren_Template) {
    // 验证: 设置模板驱动的动态子组件
    auto w = std::make_shared<Widget>("list", WidgetType::List);
    Children c;
    c.mode = Children::Template;
    c.templateInfo.dataBinding = "/users";
    c.templateInfo.componentId = "user-card";
    w->setChildren(c);

    const auto& result = w->children();
    EXPECT_EQ(result.mode, Children::Template);
    EXPECT_EQ(result.templateInfo.dataBinding, "/users");
    EXPECT_EQ(result.templateInfo.componentId, "user-card");
}

TEST(WidgetTest, Children_DefaultEmpty) {
    // 验证: 不设置 children 时默认值
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    const auto& c = w->children();
    EXPECT_EQ(c.mode, Children::Explicit);
    EXPECT_TRUE(c.explicitList.empty());
}

// ============================================================
// 9. 绑定值
// ============================================================

TEST(WidgetTest, BoundValue_SetGet) {
    // 验证: 设置和读取 BoundValue
    auto w = std::make_shared<Widget>("w", WidgetType::TextField);
    w->setBoundValue("value", JValue::fromPath("/form/name"));
    EXPECT_TRUE(w->boundValue("value").isPath());
    EXPECT_EQ(w->boundValue("value").asString(), "/form/name");
}

TEST(WidgetTest, BoundValue_NonExistent_ReturnsNull) {
    // 验证: 读取不存在的绑定值返回 Null
    auto w = std::make_shared<Widget>("w", WidgetType::TextField);
    EXPECT_TRUE(w->boundValue("not_exists").isNull());
}

TEST(WidgetTest, BoundValue_Multiple) {
    // 验证: 多个绑定值并存
    auto w = std::make_shared<Widget>("w", WidgetType::TextField);
    w->setBoundValue("text", JValue("literal"));
    w->setBoundValue("color", JValue::fromPath("/theme/color"));
    EXPECT_TRUE(w->boundValue("text").isString());
    EXPECT_TRUE(w->boundValue("color").isPath());
}

// ============================================================
// 10. 布局缓存
// ============================================================

TEST(WidgetTest, LayoutBounds_SetGet) {
    // 验证: 设置和读取布局边界
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    Rect r{10.0f, 20.0f, 100.0f, 50.0f};
    w->setLayoutBounds(r);

    const auto& result = w->layoutBounds();
    EXPECT_FLOAT_EQ(result.x, 10.0f);
    EXPECT_FLOAT_EQ(result.y, 20.0f);
    EXPECT_FLOAT_EQ(result.w, 100.0f);
    EXPECT_FLOAT_EQ(result.h, 50.0f);
}

TEST(WidgetTest, LayoutBounds_DefaultEmpty) {
    // 验证: 默认布局边界全部为 0
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    const auto& result = w->layoutBounds();
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_TRUE(result.isEmpty());
}

// ============================================================
// 11. 事件 Action
// ============================================================

TEST(WidgetTest, Action_SetGet) {
    // 验证: 设置和读取 action 名称
    auto w = std::make_shared<Widget>("btn", WidgetType::Button);
    w->setAction("submit_form");
    EXPECT_EQ(w->action(), "submit_form");
}

TEST(WidgetTest, Action_DefaultEmpty) {
    // 验证: 默认 action 为空字符串
    auto w = std::make_shared<Widget>("btn", WidgetType::Button);
    EXPECT_EQ(w->action(), "");
}

// ============================================================
// 12. 可见性与启用状态
// ============================================================

TEST(WidgetTest, Visible_DefaultTrue) {
    // 验证: 默认可见性为 true
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    EXPECT_TRUE(w->visible());
}

TEST(WidgetTest, Visible_SetFalse) {
    // 验证: 设置为不可见
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setVisible(false);
    EXPECT_FALSE(w->visible());
}

TEST(WidgetTest, Visible_Toggle) {
    // 验证: 切换可见性
    auto w = std::make_shared<Widget>("w", WidgetType::Text);
    w->setVisible(false);
    EXPECT_FALSE(w->visible());
    w->setVisible(true);
    EXPECT_TRUE(w->visible());
}

TEST(WidgetTest, Enabled_DefaultTrue) {
    // 验证: 默认启用状态为 true
    auto w = std::make_shared<Widget>("w", WidgetType::Button);
    EXPECT_TRUE(w->enabled());
}

TEST(WidgetTest, Enabled_SetFalse) {
    // 验证: 设置为禁用
    auto w = std::make_shared<Widget>("w", WidgetType::Button);
    w->setEnabled(false);
    EXPECT_FALSE(w->enabled());
}

// ============================================================
// 13. WidgetPtr 智能指针
// ============================================================

TEST(WidgetPtrTest, SharedPtr_Usage) {
    // 验证: WidgetPtr 可以正常使用
    WidgetPtr pw = std::make_shared<Widget>("ptr-test", WidgetType::Text);
    ASSERT_NE(pw, nullptr);
    EXPECT_EQ(pw->id(), "ptr-test");
}

TEST(WidgetPtrTest, NullPtr_Check) {
    // 验证: 空指针行为
    WidgetPtr pw = nullptr;
    EXPECT_EQ(pw, nullptr);
}

// ============================================================
// 14. 复合场景测试
// ============================================================

TEST(WidgetTest, FullWidget_Populated) {
    // 验证: 一个完整配置的控件，所有字段都能正常工作
    auto w = std::make_shared<Widget>("comp1", WidgetType::TextField);

    // 设置属性
    w->setProperty("placeholder", JValue("Enter text here"));
    w->setProperty("maxLength", JValue(100));
    w->setProperty("fontSize", JValue(16.0));

    // 设置绑定值
    w->setBoundValue("value", JValue::fromPath("/form/input"));

    // 设置 action
    w->setAction("on_change");

    // 设置可见性和状态
    w->setVisible(true);
    w->setEnabled(true);

    // 验证
    EXPECT_EQ(w->id(), "comp1");
    EXPECT_EQ(w->type(), WidgetType::TextField);
    EXPECT_EQ(w->property("placeholder").asString(), "Enter text here");
    EXPECT_TRUE(w->boundValue("value").isPath());
    EXPECT_EQ(w->action(), "on_change");
    EXPECT_TRUE(w->visible());
    EXPECT_TRUE(w->enabled());
}
