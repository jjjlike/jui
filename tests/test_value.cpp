/**
 * test_value.cpp — JValue 动态值类型全覆盖单元测试
 *
 * 测试范围:
 *   - 所有构造函数（默认/null/string/number/bool）
 *   - 所有 is*() 类型检测方法
 *   - 所有 as*() 值访问方法
 *   - 静态工厂方法 fromPath/fromArray/fromObject
 *   - 对象操作: set/get/has/size
 *   - 数组操作: push/size
 *   - 边界条件: 空值、默认值
 */

#include <gtest/gtest.h>
#include "jui/core/value.h"

using namespace jui;

// ============================================================
// 1. 默认构造 & Null 构造
// ============================================================

TEST(JValueTest, DefaultConstructor_IsNull) {
    // 验证: 默认构造返回 Null 类型
    JValue v;
    EXPECT_TRUE(v.isNull());
    EXPECT_EQ(v.type(), ValueType::Null);
    EXPECT_FALSE(v.isString());
    EXPECT_FALSE(v.isNumber());
    EXPECT_FALSE(v.isBoolean());
}

TEST(JValueTest, NullptrConstructor_IsNull) {
    // 验证: nullptr 构造返回 Null 类型
    JValue v(nullptr);
    EXPECT_TRUE(v.isNull());
    EXPECT_EQ(v.type(), ValueType::Null);
}

// ============================================================
// 2. 字符串构造
// ============================================================

TEST(JValueTest, StringConstructor_CRawString) {
    // 验证: const char* 构造返回 String 类型
    JValue v("hello");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.type(), ValueType::String);
    EXPECT_EQ(v.asString(), "hello");
}

TEST(JValueTest, StringConstructor_StdString) {
    // 验证: std::string 构造返回 String 类型
    JValue v(std::string("world"));
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "world");
}

TEST(JValueTest, StringConstructor_Empty) {
    // 验证: 空字符串也能正常保存
    JValue v("");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "");
}

TEST(JValueTest, StringConstructor_UTF8) {
    // 验证: 中文字符串
    JValue v("你好世界");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "你好世界");
}

// ============================================================
// 3. 数值构造
// ============================================================

TEST(JValueTest, NumberConstructor_Double) {
    // 验证: double 构造返回 Number 类型
    JValue v(3.14);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.type(), ValueType::Number);
    EXPECT_DOUBLE_EQ(v.asNumber(), 3.14);
}

TEST(JValueTest, NumberConstructor_Int) {
    // 验证: int 构造返回 Number 类型，内部转为 double
    JValue v(42);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asInt(), 42);
    EXPECT_DOUBLE_EQ(v.asNumber(), 42.0);
}

TEST(JValueTest, NumberConstructor_AsInt_Truncation) {
    // 验证: asInt() 会截断小数
    JValue v(3.99);
    EXPECT_EQ(v.asInt(), 3);
}

TEST(JValueTest, NumberConstructor_Negative) {
    // 验证: 负数
    JValue v(-10);
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asInt(), -10);
}

TEST(JValueTest, NumberConstructor_Zero) {
    // 验证: 零值
    JValue v(0.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_DOUBLE_EQ(v.asNumber(), 0.0);
}

// ============================================================
// 4. 布尔构造
// ============================================================

TEST(JValueTest, BooleanConstructor_True) {
    // 验证: true 值
    JValue v(true);
    EXPECT_TRUE(v.isBoolean());
    EXPECT_EQ(v.type(), ValueType::Boolean);
    EXPECT_TRUE(v.asBool());
}

TEST(JValueTest, BooleanConstructor_False) {
    // 验证: false 值
    JValue v(false);
    EXPECT_TRUE(v.isBoolean());
    EXPECT_FALSE(v.asBool());
}

// ============================================================
// 5. 静态工厂方法
// ============================================================

TEST(JValueTest, FromPath_IsPath) {
    // 验证: fromPath 创建 Path 类型值
    JValue v = JValue::fromPath("/user/name");
    EXPECT_TRUE(v.isPath());
    EXPECT_EQ(v.type(), ValueType::Path);
    EXPECT_EQ(v.asString(), "/user/name");
}

TEST(JValueTest, FromPath_EmptyPath) {
    // 验证: 空路径
    JValue v = JValue::fromPath("");
    EXPECT_TRUE(v.isPath());
    EXPECT_EQ(v.asString(), "");
}

TEST(JValueTest, FromArray_Empty) {
    // 验证: fromArray 创建空数组
    auto v = JValue::fromArray({});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.type(), ValueType::Array);
    EXPECT_EQ(v.size(), 0);
}

TEST(JValueTest, FromArray_WithElements) {
    // 验证: fromArray 包含多个元素
    auto v = JValue::fromArray({JValue("a"), JValue(1), JValue(true)});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.asArray()[0].asString(), "a");
    EXPECT_EQ(v.asArray()[1].asInt(), 1);
    EXPECT_TRUE(v.asArray()[2].asBool());
}

TEST(JValueTest, FromArray_Mutable) {
    // 验证: 非 const asArray() 返回可修改引用
    auto v = JValue::fromArray({JValue(0)});
    v.asArray()[0] = JValue(99);
    EXPECT_EQ(v.asArray()[0].asInt(), 99);
}

TEST(JValueTest, FromObject_Empty) {
    // 验证: fromObject 创建空对象
    auto v = JValue::fromObject({});
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.type(), ValueType::Object);
    EXPECT_EQ(v.size(), 0);
}

TEST(JValueTest, FromObject_WithFields) {
    // 验证: fromObject 包含多个字段
    std::map<std::string, JValue> obj;
    obj["name"] = JValue("Alice");
    obj["age"] = JValue(30);
    obj["active"] = JValue(true);

    auto v = JValue::fromObject(obj);
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.size(), 3);
    EXPECT_TRUE(v.has("name"));
    EXPECT_TRUE(v.has("age"));
    EXPECT_TRUE(v.has("active"));
    EXPECT_EQ(v.get("name").asString(), "Alice");
}

// ============================================================
// 6. 对象操作
// ============================================================

TEST(JValueTest, Object_Set_OnNonObject) {
    // 验证: 在非 Object 值上调用 set 会自动转为 Object
    JValue v(42); // 初始是 Number
    v.set("key", JValue("value"));
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v.get("key").asString(), "value");
}

TEST(JValueTest, Object_Set_MultipleKeys) {
    // 验证: 可以设置多个键
    JValue v = JValue::fromObject({});
    v.set("a", JValue(1));
    v.set("b", JValue(2));
    v.set("c", JValue(3));
    EXPECT_EQ(v.size(), 3);
}

TEST(JValueTest, Object_Set_Overwrite) {
    // 验证: 覆盖已有键
    JValue v = JValue::fromObject({});
    v.set("x", JValue(1));
    v.set("x", JValue(100));
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v.get("x").asInt(), 100);
}

TEST(JValueTest, Object_Has_Existent) {
    // 验证: has() 返回 true 对于存在的键
    auto v = JValue::fromObject({{"foo", JValue("bar")}});
    EXPECT_TRUE(v.has("foo"));
}

TEST(JValueTest, Object_Has_NonExistent) {
    // 验证: has() 返回 false 对于不存在的键
    auto v = JValue::fromObject({{"foo", JValue("bar")}});
    EXPECT_FALSE(v.has("nonexistent"));
}

TEST(JValueTest, Object_Has_OnNonObject) {
    // 验证: 非 Object 类型 has() 永远返回 false
    JValue v(42);
    EXPECT_FALSE(v.has("anything"));
}

TEST(JValueTest, Object_Get_Existent) {
    // 验证: get() 返回正确的值
    auto v = JValue::fromObject({{"score", JValue(100.0)}});
    EXPECT_DOUBLE_EQ(v.get("score").asNumber(), 100.0);
}

TEST(JValueTest, Object_Get_NonExistent_ReturnsNull) {
    // 验证: get() 不存在的键返回 Null
    auto v = JValue::fromObject({{"a", JValue(1)}});
    JValue result = v.get("b");
    EXPECT_TRUE(result.isNull());
}

TEST(JValueTest, Object_Get_OnNonObject_ReturnsNull) {
    // 验证: 非 Object 类型 get() 返回 Null
    JValue v("string_value");
    JValue result = v.get("any");
    EXPECT_TRUE(result.isNull());
}

TEST(JValueTest, Object_Size_Empty) {
    // 验证: 空对象的 size() 为 0
    auto v = JValue::fromObject({});
    EXPECT_EQ(v.size(), 0);
}

TEST(JValueTest, Object_Size_NonEmpty) {
    // 验证: 正确返回对象键数量
    auto v = JValue::fromObject({{"a", JValue(1)}, {"b", JValue(2)}});
    EXPECT_EQ(v.size(), 2);
}

// ============================================================
// 7. 数组操作
// ============================================================

TEST(JValueTest, Array_Push_OnNonArray) {
    // 验证: 在非 Array 值上 push 会自动转为 Array
    JValue v(false); // 初始是 Boolean
    v.push(JValue(1));
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.size(), 1);
}

TEST(JValueTest, Array_Push_Multiple) {
    // 验证: 连续 push 多个元素
    JValue v = JValue::fromArray({});
    v.push(JValue("first"));
    v.push(JValue("second"));
    v.push(JValue("third"));
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.asArray()[0].asString(), "first");
    EXPECT_EQ(v.asArray()[2].asString(), "third");
}

TEST(JValueTest, Array_Size_Empty) {
    // 验证: 空数组 size() 为 0
    auto v = JValue::fromArray({});
    EXPECT_EQ(v.size(), 0);
}

TEST(JValueTest, Array_Size_NonEmpty) {
    // 验证: 非空数组正确返回 size
    auto v = JValue::fromArray({JValue(1), JValue(2), JValue(3), JValue(4)});
    EXPECT_EQ(v.size(), 4);
}

TEST(JValueTest, Size_OnNonContainer) {
    // 验证: 非数组/非对象的 size() 返回 0
    JValue v("text");
    EXPECT_EQ(v.size(), 0);

    JValue n(3.14);
    EXPECT_EQ(v.size(), 0);
}

// ============================================================
// 8. 类型互斥验证
// ============================================================

TEST(JValueTest, Null_IsOnlyNull) {
    // 验证: Null 值只报 isNull() 为 true
    JValue v;
    EXPECT_TRUE(v.isNull());
    EXPECT_FALSE(v.isString());
    EXPECT_FALSE(v.isNumber());
    EXPECT_FALSE(v.isBoolean());
    EXPECT_FALSE(v.isPath());
    EXPECT_FALSE(v.isArray());
    EXPECT_FALSE(v.isObject());
}

TEST(JValueTest, String_IsOnlyString) {
    // 验证: String 值只报 isString() 为 true
    JValue v("hello");
    EXPECT_FALSE(v.isNull());
    EXPECT_TRUE(v.isString());
    EXPECT_FALSE(v.isNumber());
    EXPECT_FALSE(v.isBoolean());
    EXPECT_FALSE(v.isPath());
    EXPECT_FALSE(v.isArray());
    EXPECT_FALSE(v.isObject());
}

TEST(JValueTest, Number_IsOnlyNumber) {
    // 验证: Number 值只报 isNumber() 为 true
    JValue v(42);
    EXPECT_FALSE(v.isNull());
    EXPECT_FALSE(v.isString());
    EXPECT_TRUE(v.isNumber());
    EXPECT_FALSE(v.isBoolean());
}

TEST(JValueTest, Boolean_IsOnlyBoolean) {
    // 验证: Boolean 值只报 isBoolean() 为 true
    JValue v(true);
    EXPECT_FALSE(v.isNull());
    EXPECT_FALSE(v.isString());
    EXPECT_FALSE(v.isNumber());
    EXPECT_TRUE(v.isBoolean());
}

TEST(JValueTest, Path_IsOnlyPath) {
    // 验证: Path 值只报 isPath() 为 true
    JValue v = JValue::fromPath("/a/b");
    EXPECT_FALSE(v.isNull());
    EXPECT_FALSE(v.isString());
    EXPECT_FALSE(v.isNumber());
    EXPECT_TRUE(v.isPath());
}
