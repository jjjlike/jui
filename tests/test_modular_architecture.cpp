/**
 * @file test_modular_architecture.cpp
 * @brief 验证新模块化架构的基础设施正确性
 *
 * 验证:
 *   - EventBus 发布/订阅
 *   - ServiceLocator 注册/解析
 *   - 接口定义可编译
 *   - 模块间无直接依赖
 */

#include <gtest/gtest.h>
#include "jui/bus/jui_interfaces.h"
#include "jui/core/geometry.h"
#include "jui/core/widget_types.h"
#include <atomic>
#include <string>

using namespace jui;
using namespace jui::bus;

// ──── EventBus 功能测试 ────

TEST(ModularArch, EventBus_SubscribeAndPublish) {
    EventBus bus;
    std::atomic<int> count{0};

    bus.subscribe(EventType::ActionFired, [&](const Event& e) {
        count++;
        EXPECT_EQ(e.surfaceId, "main");
        EXPECT_EQ(e.widgetId, "btn1");
    });

    bus.publish(EventType::ActionFired, "main", "btn1", {});
    EXPECT_EQ(count, 1);

    bus.publish(EventType::ActionFired, "main", "btn1", {});
    EXPECT_EQ(count, 2);
}

TEST(ModularArch, EventBus_MultipleHandlers) {
    EventBus bus;
    int a = 0, b = 0;

    bus.subscribe(EventType::LayoutCompleted, [&](const Event&) { a++; });
    bus.subscribe(EventType::LayoutCompleted, [&](const Event&) { b++; });

    bus.publish(Event{EventType::LayoutCompleted});
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
}

TEST(ModularArch, EventBus_DifferentTypesIsolated) {
    EventBus bus;
    int paint = 0, layout = 0;

    bus.subscribe(EventType::PaintBegin, [&](const Event&) { paint++; });
    bus.subscribe(EventType::LayoutCompleted, [&](const Event&) { layout++; });

    bus.publish(Event{EventType::PaintBegin});
    EXPECT_EQ(paint, 1);
    EXPECT_EQ(layout, 0);
}

TEST(ModularArch, EventBus_Clear) {
    EventBus bus;
    int count = 0;
    bus.subscribe(EventType::ActionFired, [&](const Event&) { count++; });

    bus.publish(Event{EventType::ActionFired});
    EXPECT_EQ(count, 1);

    bus.clear(EventType::ActionFired);
    bus.publish(Event{EventType::ActionFired});
    EXPECT_EQ(count, 1); // 不应增加
}

TEST(ModularArch, EventBus_HandlerCount) {
    EventBus bus;
    EXPECT_EQ(bus.handlerCount(EventType::ActionFired), 0);

    bus.subscribe(EventType::ActionFired, [](const Event&) {});
    bus.subscribe(EventType::ActionFired, [](const Event&) {});
    EXPECT_EQ(bus.handlerCount(EventType::ActionFired), 2);
}

// ──── ServiceLocator 功能测试 ────

class ITestService {
public:
    virtual ~ITestService() = default;
    virtual int value() const = 0;
};

class TestServiceImpl : public ITestService {
    int v_;
public:
    explicit TestServiceImpl(int v) : v_(v) {}
    int value() const override { return v_; }
};

TEST(ModularArch, ServiceLocator_BindAndResolve) {
    ServiceLocator::bind<ITestService>(std::make_shared<TestServiceImpl>(42));
    ASSERT_TRUE(ServiceLocator::has<ITestService>());

    auto svc = ServiceLocator::resolve<ITestService>();
    ASSERT_NE(svc, nullptr);
    EXPECT_EQ(svc->value(), 42);

    ServiceLocator::clear();
    EXPECT_FALSE(ServiceLocator::has<ITestService>());
}

TEST(ModularArch, ServiceLocator_NotRegisteredThrows) {
    ServiceLocator::clear();
    EXPECT_FALSE(ServiceLocator::has<ITestService>());
    EXPECT_THROW(ServiceLocator::resolve<ITestService>(), std::runtime_error);
}

// ──── 基础类型测试 ────

TEST(ModularArch, Geometry_RectEquality) {
    Rect a{0, 0, 100, 50};
    Rect b{0, 0, 100, 50};
    Rect c{1, 0, 100, 50};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(ModularArch, WidgetTypes_Enum) {
    EXPECT_NE(static_cast<int>(WidgetType::Text), static_cast<int>(WidgetType::Button));
    EXPECT_NE(static_cast<int>(WidgetType::Column), static_cast<int>(WidgetType::Row));
}

// ──── 接口实例化测试（验证可编译） ────

TEST(ModularArch, Interfaces_CompileCheck) {
    // 验证所有接口可以 #include 且类型定义正确
    // 不实例化，仅编译期检查

    // IEventBus
    static_assert(std::is_abstract_v<IEventBus>);

    // IWidgetNode / IWidgetTree
    static_assert(std::is_abstract_v<core::IWidgetNode>);
    static_assert(std::is_abstract_v<core::IWidgetTree>);

    // IDataModel
    static_assert(std::is_abstract_v<core::IDataModel>);

    // IRenderTarget
    static_assert(std::is_abstract_v<render::IRenderTarget>);

    // ILayoutEngine
    static_assert(std::is_abstract_v<layout::ILayoutEngine>);

    // IInputDispatcher
    static_assert(std::is_abstract_v<input::IInputDispatcher>);

    // IWidgetState
    static_assert(std::is_abstract_v<render::IWidgetState>);

    SUCCEED();
}
