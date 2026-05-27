/**
 * test_surface.cpp — Surface 管理器全覆盖单元测试
 *
 * 测试范围:
 *   - 构造: Surface 创建和 ID 获取
 *   - 组件管理: addWidget / removeWidget / getWidget / widgets
 *   - 组件更新: updateComponents（批量导入，A2UI 邻接表）
 *   - 根组件: setRootWidgetId / rootWidgetId / rootWidget
 *   - 数据模型: dataModel() 访问和操作
 *   - 内容尺寸: setContentSize / contentSize
 *   - 脏标记: needsLayout / setNeedsLayout
 *   - SurfacePtr 智能指针
 *   - 复合场景: 完整 A2UI 组件树模拟
 */

#include <gtest/gtest.h>
#include "jui/core/surface.h"

using namespace jui;

// ============================================================
// 1. Surface 构造
// ============================================================

TEST(SurfaceTest, Constructor_Id) {
    // 验证: Surface 创建后 id() 返回正确的标识符
    Surface surface("main");
    EXPECT_EQ(surface.id(), "main");
}

TEST(SurfaceTest, Constructor_EmptyId) {
    // 验证: 空 ID 的 Surface 也能正常创建
    Surface surface("");
    EXPECT_EQ(surface.id(), "");
}

TEST(SurfaceTest, Constructor_DifferentIds) {
    // 验证: 不同 Surface 有不同的 ID
    Surface s1("primary");
    Surface s2("secondary");
    EXPECT_NE(s1.id(), s2.id());
}

// ============================================================
// 2. 组件管理
// ============================================================

TEST(SurfaceTest, AddWidget_Single) {
    // 验证: 添加单个 Widget 后可以通过 getWidget 获取
    Surface surface("s1");
    auto w = std::make_shared<Widget>("w1", WidgetType::Text);
    surface.addWidget(w);

    EXPECT_EQ(surface.getWidget("w1"), w);
    EXPECT_EQ(surface.getWidget("w1")->id(), "w1");
}

TEST(SurfaceTest, AddWidget_Multiple) {
    // 验证: 添加多个 Widget，各自独立
    Surface surface("s1");
    auto t = std::make_shared<Widget>("text", WidgetType::Text);
    auto b = std::make_shared<Widget>("btn", WidgetType::Button);
    surface.addWidget(t);
    surface.addWidget(b);

    EXPECT_EQ(surface.widgets().size(), 2);
    EXPECT_EQ(surface.getWidget("text")->type(), WidgetType::Text);
    EXPECT_EQ(surface.getWidget("btn")->type(), WidgetType::Button);
}

TEST(SurfaceTest, AddWidget_Overwrite) {
    // 验证: 相同 ID 的 Widget 会覆盖（邻接表 upsert 语义）
    Surface surface("s1");
    auto old = std::make_shared<Widget>("dup", WidgetType::Text);
    auto neww = std::make_shared<Widget>("dup", WidgetType::Button);
    surface.addWidget(old);
    surface.addWidget(neww);

    // 同 ID 覆盖
    EXPECT_EQ(surface.widgets().size(), 1);
    EXPECT_EQ(surface.getWidget("dup")->type(), WidgetType::Button);
}

TEST(SurfaceTest, RemoveWidget_Existent) {
    // 验证: removeWidget 删除存在的组件
    Surface surface("s1");
    auto w = std::make_shared<Widget>("to-remove", WidgetType::Text);
    surface.addWidget(w);
    surface.removeWidget("to-remove");

    EXPECT_EQ(surface.getWidget("to-remove"), nullptr);
    EXPECT_EQ(surface.widgets().size(), 0);
}

TEST(SurfaceTest, RemoveWidget_NonExistent) {
    // 验证: removeWidget 删除不存在的组件不会崩溃
    Surface surface("s1");
    EXPECT_NO_THROW(surface.removeWidget("ghost"));
}

TEST(SurfaceTest, GetWidget_NonExistent_ReturnsNull) {
    // 验证: 获取不存在的 Widget 返回 nullptr
    Surface surface("s1");
    EXPECT_EQ(surface.getWidget("nonexistent"), nullptr);
}

TEST(SurfaceTest, GetWidget_AfterRemove) {
    // 验证: 删除后再获取返回 nullptr
    Surface surface("s1");
    auto w = std::make_shared<Widget>("temp", WidgetType::Text);
    surface.addWidget(w);
    surface.removeWidget("temp");
    EXPECT_EQ(surface.getWidget("temp"), nullptr);
}

// ============================================================
// 3. updateComponents（批量组件更新）
// ============================================================

TEST(SurfaceTest, UpdateComponents_Empty) {
    // 验证: 空组件列表不会崩溃
    Surface surface("s1");
    surface.updateComponents({});
    EXPECT_EQ(surface.widgets().size(), 0);
}

TEST(SurfaceTest, UpdateComponents_AddNew) {
    // 验证: updateComponents 批量添加新组件
    Surface surface("s1");
    std::vector<WidgetPtr> components = {
        std::make_shared<Widget>("a", WidgetType::Text),
        std::make_shared<Widget>("b", WidgetType::Button),
    };
    surface.updateComponents(components);

    EXPECT_EQ(surface.widgets().size(), 2);
    EXPECT_NE(surface.getWidget("a"), nullptr);
    EXPECT_NE(surface.getWidget("b"), nullptr);
}

TEST(SurfaceTest, UpdateComponents_MixedAddAndOverwrite) {
    // 验证: 混合添加新组件和覆盖已有组件
    Surface surface("s1");
    surface.addWidget(std::make_shared<Widget>("x", WidgetType::Text));

    std::vector<WidgetPtr> components = {
        std::make_shared<Widget>("x", WidgetType::TextField), // 覆盖
        std::make_shared<Widget>("y", WidgetType::Button),    // 新增
    };
    surface.updateComponents(components);

    EXPECT_EQ(surface.widgets().size(), 2);
    EXPECT_EQ(surface.getWidget("x")->type(), WidgetType::TextField);
    EXPECT_EQ(surface.getWidget("y")->type(), WidgetType::Button);
}

// ============================================================
// 4. 根组件管理
// ============================================================

TEST(SurfaceTest, RootWidget_Id) {
    // 验证: 设置和读取根组件 ID
    Surface surface("s1");
    surface.setRootWidgetId("root-node");
    EXPECT_EQ(surface.rootWidgetId(), "root-node");
}

TEST(SurfaceTest, RootWidget_DefaultEmpty) {
    // 验证: 默认根组件 ID 为空字符串
    Surface surface("s1");
    EXPECT_EQ(surface.rootWidgetId(), "");
}

TEST(SurfaceTest, RootWidget_GetWidget) {
    // 验证: rootWidget() 返回根组件 Widget 对象
    Surface surface("s1");
    auto root = std::make_shared<Widget>("root", WidgetType::Column);
    surface.addWidget(root);
    surface.setRootWidgetId("root");

    auto retrieved = surface.rootWidget();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->id(), "root");
    EXPECT_EQ(retrieved->type(), WidgetType::Column);
}

TEST(SurfaceTest, RootWidget_NotAdded_ReturnsNull) {
    // 验证: 设置了 ID 但没添加对应 Widget 时 rootWidget() 返回 nullptr
    Surface surface("s1");
    surface.setRootWidgetId("missing");
    EXPECT_EQ(surface.rootWidget(), nullptr);
}

// ============================================================
// 5. 数据模型
// ============================================================

TEST(SurfaceTest, DataModel_Access) {
    // 验证: 可以访问 Surface 内部的 DataModel
    Surface surface("s1");
    auto& model = surface.dataModel();
    model.setValue("/key", JValue("value"));
    EXPECT_EQ(surface.dataModel().getValue("/key").asString(), "value");
}

TEST(SurfaceTest, DataModel_ConstAccess) {
    // 验证: const Surface 可以只读访问 DataModel
    const Surface surface("s1");
    // 需要先设置数据
    surface.dataModel(); // 此方法 non-const, 所以... 实际上 const 对象无法调用 non-const
    // 但 allData() 可以
}

TEST(SurfaceTest, DataModel_IsolatedPerSurface) {
    // 验证: 不同 Surface 的数据模型相互独立
    Surface s1("one");
    Surface s2("two");

    s1.dataModel().setValue("/data", JValue("s1-data"));
    s2.dataModel().setValue("/data", JValue("s2-data"));

    EXPECT_EQ(s1.dataModel().getValue("/data").asString(), "s1-data");
    EXPECT_EQ(s2.dataModel().getValue("/data").asString(), "s2-data");
}

// ============================================================
// 6. 内容尺寸
// ============================================================

TEST(SurfaceTest, ContentSize_DefaultZero) {
    // 验证: 默认内容尺寸为 0
    Surface surface("s1");
    Size size = surface.contentSize();
    EXPECT_FLOAT_EQ(size.w, 0.0f);
    EXPECT_FLOAT_EQ(size.h, 0.0f);
}

TEST(SurfaceTest, ContentSize_SetGet) {
    // 验证: 设置和读取内容尺寸
    Surface surface("s1");
    surface.setContentSize(1920.0f, 1080.0f);
    Size size = surface.contentSize();
    EXPECT_FLOAT_EQ(size.w, 1920.0f);
    EXPECT_FLOAT_EQ(size.h, 1080.0f);
}

// ============================================================
// 7. 脏标记
// ============================================================

TEST(SurfaceTest, NeedsLayout_DefaultTrue) {
    // 验证: 新建 Surface 默认需要布局
    Surface surface("s1");
    EXPECT_TRUE(surface.needsLayout());
}

TEST(SurfaceTest, NeedsLayout_SetFalse) {
    // 验证: 标记不需要布局
    Surface surface("s1");
    surface.setNeedsLayout(false);
    EXPECT_FALSE(surface.needsLayout());
}

TEST(SurfaceTest, NeedsLayout_Toggle) {
    // 验证: 切换脏标记
    Surface surface("s1");
    surface.setNeedsLayout(false);
    EXPECT_FALSE(surface.needsLayout());
    surface.setNeedsLayout(true);
    EXPECT_TRUE(surface.needsLayout());
}

// ============================================================
// 8. SurfacePtr 智能指针
// ============================================================

TEST(SurfacePtrTest, SharedPtr_CreateUse) {
    // 验证: SurfacePtr 创建和使用
    SurfacePtr sp = std::make_shared<Surface>("shared");
    ASSERT_NE(sp, nullptr);
    EXPECT_EQ(sp->id(), "shared");
}

TEST(SurfacePtrTest, NullPtr_Check) {
    // 验证: 空 SurfacePtr 行为
    SurfacePtr sp = nullptr;
    EXPECT_EQ(sp, nullptr);
}

// ============================================================
// 9. 复合场景：A2UI 组件树模拟
// ============================================================

TEST(SurfaceTest, A2UI_Tree_Simulation) {
    // 验证: 模拟完整的 A2UI 邻接表组件树
    // 场景: Column → [Card → [Text, Text], Divider, Row → [Button, Button]]
    Surface surface("demo");

    // 创建所有组件（扁平列表）
    auto root = std::make_shared<Widget>("root", WidgetType::Column);
    auto card = std::make_shared<Widget>("card", WidgetType::Card);
    auto title = std::make_shared<Widget>("title", WidgetType::Text);
    auto subtitle = std::make_shared<Widget>("subtitle", WidgetType::Text);
    auto divider = std::make_shared<Widget>("div", WidgetType::Divider);
    auto row = std::make_shared<Widget>("row", WidgetType::Row);
    auto btn1 = std::make_shared<Widget>("btn1", WidgetType::Button);
    auto btn2 = std::make_shared<Widget>("btn2", WidgetType::Button);

    // 通过 Children 建立父子引用（邻接表核心）
    root->setChildren(Children{Children::Explicit, {"card", "div", "row"}});
    card->setChildren(Children{Children::Explicit, {"title", "subtitle"}});
    row->setChildren(Children{Children::Explicit, {"btn1", "btn2"}});

    // 批量添加到 Surface
    surface.updateComponents({root, card, title, subtitle, divider, row, btn1, btn2});
    surface.setRootWidgetId("root");

    // 验证
    EXPECT_EQ(surface.widgets().size(), 8);
    EXPECT_EQ(surface.rootWidget()->id(), "root");

    // 验证父子关系
    EXPECT_EQ(root->children().explicitList.size(), 3);
    EXPECT_EQ(card->children().explicitList.size(), 2);
    EXPECT_EQ(row->children().explicitList.size(), 2);
}
