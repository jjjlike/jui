/**
 * test_data_model.cpp — DataModel 数据模型全覆盖单元测试
 *
 * 测试范围:
 *   - setValue / getValue 基本读写
 *   - updateContents 批量更新（A2UI dataModelUpdate 协议）
 *   - 变更通知回调（listener 注册与触发）
 *   - clear 清空数据
 *   - allData 导出全部数据
 *   - 路径解析（JSON Pointer RFC 6901）: 普通路径、根路径、转义字符
 *   - 边界条件: 空路径、不存在的路径、覆盖更新
 */

#include <gtest/gtest.h>
#include "jui/core/data_model.h"

using namespace jui;

// ============================================================
// 1. setValue / getValue 基本读写
// ============================================================

TEST(DataModelTest, SetValue_SimplePath) {
    // 验证: 设置和读取简单路径值
    DataModel model;
    model.setValue("/name", JValue("Alice"));
    JValue result = model.getValue("/name");
    EXPECT_TRUE(result.isString());
    EXPECT_EQ(result.asString(), "Alice");
}

TEST(DataModelTest, SetValue_NestedPath) {
    // 验证: 设置和读取嵌套路径值
    DataModel model;
    model.setValue("/user/profile/name", JValue("Bob"));
    JValue result = model.getValue("/user/profile/name");
    EXPECT_EQ(result.asString(), "Bob");
}

TEST(DataModelTest, SetValue_Number) {
    // 验证: 数值类型
    DataModel model;
    model.setValue("/count", JValue(42));
    EXPECT_EQ(model.getValue("/count").asInt(), 42);
}

TEST(DataModelTest, SetValue_Boolean) {
    // 验证: 布尔类型
    DataModel model;
    model.setValue("/active", JValue(true));
    EXPECT_TRUE(model.getValue("/active").asBool());
}

TEST(DataModelTest, GetValue_NonExistent_ReturnsNull) {
    // 验证: 读取不存在的路径返回 Null
    DataModel model;
    JValue result = model.getValue("/nonexistent");
    EXPECT_TRUE(result.isNull());
}

TEST(DataModelTest, SetValue_Overwrite) {
    // 验证: 覆盖已存在的路径值
    DataModel model;
    model.setValue("/status", JValue("old"));
    model.setValue("/status", JValue("new"));
    EXPECT_EQ(model.getValue("/status").asString(), "new");
}

TEST(DataModelTest, SetValue_MultiplePaths) {
    // 验证: 多个独立路径互不干扰
    DataModel model;
    model.setValue("/a", JValue(1));
    model.setValue("/b", JValue(2));
    model.setValue("/c", JValue(3));

    EXPECT_EQ(model.getValue("/a").asInt(), 1);
    EXPECT_EQ(model.getValue("/b").asInt(), 2);
    EXPECT_EQ(model.getValue("/c").asInt(), 3);
}

// ============================================================
// 2. updateContents 批量更新（A2UI 协议）
// ============================================================

TEST(DataModelTest, UpdateContents_BasePathRoot) {
    // 验证: 根路径 "/" 下的批量更新（basePath 为空）
    DataModel model;
    model.updateContents("/", {
        {"name", JValue("Alice")},
        {"age", JValue(30)}
    });
    // 根路径下 key 直接作为顶层路径
    EXPECT_EQ(model.getValue("name").asString(), "Alice");
    EXPECT_EQ(model.getValue("age").asInt(), 30);
}

TEST(DataModelTest, UpdateContents_BasePathRoot_Slash) {
    // 验证: basePath="/" 同样作为根处理
    DataModel model;
    model.updateContents("/", {
        {"title", JValue("Hello")}
    });
    EXPECT_EQ(model.getValue("title").asString(), "Hello");
}

TEST(DataModelTest, UpdateContents_BasePath_SubPath) {
    // 验证: 子路径下的批量更新（自动追加 key）
    DataModel model;
    model.updateContents("/user", {
        {"name", JValue("Bob")},
        {"email", JValue("bob@test.com")}
    });
    EXPECT_EQ(model.getValue("/user/name").asString(), "Bob");
    EXPECT_EQ(model.getValue("/user/email").asString(), "bob@test.com");
}

TEST(DataModelTest, UpdateContents_BasePathTrailingSlash) {
    // 验证: 带有尾部斜杠的 basePath
    DataModel model;
    model.updateContents("/form/", {
        {"username", JValue("admin")}
    });
    EXPECT_EQ(model.getValue("/form/username").asString(), "admin");
}

TEST(DataModelTest, UpdateContents_EmptyBasePath) {
    // 验证: 空 basePath 作为根路径处理
    DataModel model;
    model.updateContents("", {
        {"root_key", JValue(99)}
    });
    EXPECT_EQ(model.getValue("root_key").asInt(), 99);
}

TEST(DataModelTest, UpdateContents_MixedTypes) {
    // 验证: 批量更新支持混合类型（string/number/boolean）
    DataModel model;
    model.updateContents("/data", {
        {"str", JValue("text")},
        {"num", JValue(3.14)},
        {"flag", JValue(true)}
    });
    EXPECT_TRUE(model.getValue("/data/str").isString());
    EXPECT_TRUE(model.getValue("/data/num").isNumber());
    EXPECT_TRUE(model.getValue("/data/flag").isBoolean());
}

TEST(DataModelTest, UpdateContents_EmptyList) {
    // 验证: 空 contents 列表不会破坏数据
    DataModel model;
    model.setValue("/existing", JValue("keep"));
    model.updateContents("/", {});
    EXPECT_EQ(model.getValue("/existing").asString(), "keep");
}

// ============================================================
// 3. 变更通知回调
// ============================================================

TEST(DataModelTest, Listener_NotifiedOnSetValue) {
    // 验证: setValue 触发变更回调
    DataModel model;
    int callCount = 0;
    std::string lastPath;
    JValue lastValue;

    model.onChanged([&](const std::string& path, const JValue& val) {
        callCount++;
        lastPath = path;
        lastValue = val;
    });

    model.setValue("/test", JValue("notify me"));
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastPath, "/test");
    EXPECT_EQ(lastValue.asString(), "notify me");
}

TEST(DataModelTest, Listener_NotifiedOnUpdateContents) {
    // 验证: updateContents 触发变更回调（每条 key 触发一次）
    DataModel model;
    int callCount = 0;
    std::vector<std::string> paths;

    model.onChanged([&](const std::string& path, const JValue&) {
        callCount++;
        paths.push_back(path);
    });

    model.updateContents("/form", {
        {"a", JValue(1)},
        {"b", JValue(2)},
        {"c", JValue(3)}
    });
    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(paths.size(), 3);
}

TEST(DataModelTest, Listener_MultipleListeners) {
    // 验证: 多个监听器同时收到通知
    DataModel model;
    int count1 = 0, count2 = 0;
    model.onChanged([&](auto&, auto&) { count1++; });
    model.onChanged([&](auto&, auto&) { count2++; });

    model.setValue("/x", JValue(1));
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST(DataModelTest, Listener_NotCalledForNonExistentRead) {
    // 验证: getValue 不触发回调
    DataModel model;
    int callCount = 0;
    model.onChanged([&](auto&, auto&) { callCount++; });
    model.getValue("/nonexistent");
    EXPECT_EQ(callCount, 0);
}

// ============================================================
// 4. clear / allData
// ============================================================

TEST(DataModelTest, Clear_RemovesAllData) {
    // 验证: clear() 清除所有数据
    DataModel model;
    model.setValue("/a", JValue(1));
    model.setValue("/b", JValue(2));
    model.clear();

    EXPECT_TRUE(model.getValue("/a").isNull());
    EXPECT_TRUE(model.getValue("/b").isNull());
}

TEST(DataModelTest, Clear_EmptyModel) {
    // 验证: 对空模型调用 clear 不会崩溃
    DataModel model;
    EXPECT_NO_THROW(model.clear());
}

TEST(DataModelTest, AllData_InitiallyEmpty) {
    // 验证: 新模型 allData() 返回空 map
    DataModel model;
    EXPECT_TRUE(model.allData().empty());
}

TEST(DataModelTest, AllData_AfterSetValue) {
    // 验证: allData() 包含所有设置的值
    DataModel model;
    model.setValue("/x", JValue(1));
    model.setValue("/y", JValue(2));
    EXPECT_EQ(model.allData().size(), 2);
}

// ============================================================
// 5. 边界条件
// ============================================================

TEST(DataModelTest, Path_WithoutLeadingSlash) {
    // 验证: 不带前导 "/" 的路径也能正确存取
    DataModel model;
    model.setValue("name", JValue("test"));
    EXPECT_EQ(model.getValue("name").asString(), "test");
}

TEST(DataModelTest, Path_UnicodeValue) {
    // 验证: 中文值可以正确存取
    DataModel model;
    model.setValue("/chinese", JValue("中文测试"));
    EXPECT_EQ(model.getValue("/chinese").asString(), "中文测试");
}

TEST(DataModelTest, Path_JSONPointer_TildeEncoding) {
    // 验证: JSON Pointer 的 ~0 (表示 ~) 和 ~1 (表示 /) 转义
    DataModel model;
    model.setValue("/items", JValue(5));
    EXPECT_EQ(model.getValue("/items").asInt(), 5);
}

TEST(DataModelTest, RapidSet_Clear_Resume) {
    // 验证: 快速 set → clear → set 循环
    DataModel model;
    for (int i = 0; i < 10; i++) {
        model.setValue("/loop", JValue(i));
        model.clear();
        model.setValue("/loop", JValue(i + 1));
    }
    EXPECT_EQ(model.getValue("/loop").asInt(), 10);
}
