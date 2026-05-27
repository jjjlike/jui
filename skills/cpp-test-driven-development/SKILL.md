---
name: cpp-test-driven-development
description: C++ 测试驱动开发。先写失败的测试再写实现代码。当实现任何逻辑、修复任何 Bug、修改任何行为时使用。当需要证明代码正确时使用。默认使用 Google Test，如项目用 Catch2/doctest 则适配。
---

# C++ 测试驱动开发

## 概述

先写测试，看到它失败，再写最少代码让它通过，最后重构。对于 Bug 修复，先用测试复现 Bug，再修复。测试是证据——"看起来没问题"不算完成。

**自动化策略：** 自动创建测试文件、添加 CMakeLists.txt 测试目标、运行测试。测试失败时自动分析原因并修复。所有测试在 AddressSanitizer 下自动运行。

## 何时使用

- 实现任何新逻辑或行为
- 修复任何 Bug（证明模式）
- 修改已有功能
- 添加边界条件处理
- 任何可能破坏现有行为的改动

**不使用：** 纯配置变更、注释更新、代码格式化。

## TDD 循环

```
    红                 绿                   重构
 写失败的测试    写最少代码让它通过       清理实现
     │                 │                    │
     ▼                 ▼                    ▼
  测试失败          测试通过            测试仍然通过
```

对 C++ 来说额外一步：**编译**。测试必须先编译通过才能运行——编译失败等于"红"。

### 步骤 1：红 —— 写失败的测试

```cpp
// 文件: tests/config/ConfigLoader_test.cpp
#include <gtest/gtest.h>
#include "config/ConfigLoader.h"

// 红: ConfigLoader 还不存在，此测试编译失败
TEST(ConfigLoaderTest, LoadsValidJsonConfig) {
    ConfigLoader loader("test_data/valid_config.json");
    auto config = loader.load();

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->serverPort, 8080);
    EXPECT_EQ(config->logLevel, "info");
}

TEST(ConfigLoaderTest, ReturnsNulloptForMissingFile) {
    ConfigLoader loader("test_data/nonexistent.json");
    auto config = loader.load();

    EXPECT_FALSE(config.has_value());
}
```

### 步骤 2：绿 —— 最少代码通过

```cpp
// 文件: include/config/ConfigLoader.h
#pragma once
#include <optional>
#include <string>
#include <string_view>

struct AppConfig {
    int serverPort = 8080;
    std::string logLevel = "info";
};

class ConfigLoader {
public:
    explicit ConfigLoader(std::string_view path);
    std::optional<AppConfig> load() const;
private:
    std::string m_path;
};
```

### 步骤 3：重构

测试通过后，优化实现而不改行为：
- 提取重复逻辑
- 改进命名
- 用 RAII 管理资源
- 优化但不改变行为

每次重构后重新编译测试确认。

## 证明模式（Bug 修复）

Bug 来了，**不要先修**。先写测试复现它：

```
Bug 报告 → 写复现测试 → 测试失败（确认 Bug）→ 修复 → 测试通过 → 全量回归
```

```cpp
// Bug: "加载空 JSON 文件时程序崩溃"

// 步骤 1: 复现测试（应该失败/崩溃）
TEST(ConfigLoaderTest, HandlesEmptyJsonGracefully) {
    ConfigLoader loader("test_data/empty.json");
    // 不应崩溃
    auto config = loader.load();
    EXPECT_FALSE(config.has_value());
}

// 步骤 2: 修复 —— 在 load() 中检查 JSON 是否为空
// 步骤 3: 测试通过 → Bug 已修复且有回归防护
```

## C++ 测试框架选择

| 框架 | 特点 | 何时选用 |
|------|------|----------|
| **Google Test** | 最流行，功能全，与 CMake 集成好 | **默认选择** |
| **Catch2** | 单头文件，语法更自然 | 项目已在使用时 |
| **doctest** | 编译最快，轻量级 | 需要极快编译的测试时 |

**Agent 会自动检测项目使用的框架并适配语法。**

## CMake 测试配置

```cmake
# 自动添加测试目标
enable_testing()
find_package(GTest REQUIRED)

function(add_cpp_test test_name test_file)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE
        GTest::gtest_main
        ${PROJECT_NAME}_lib   # 被测库
    )
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    gtest_discover_tests(${test_name})
endfunction()

add_cpp_test(ConfigLoader_test tests/config/ConfigLoader_test.cpp)
```

## 写好 C++ 测试

### 测试状态，不测试实现细节

```cpp
// ✓ 好：测的是结果（基于状态）
TEST(ConfigLoaderTest, LoadsCorrectValues) {
    ConfigLoader loader("test.json");
    auto config = loader.load();
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->serverPort, 8080);
    EXPECT_EQ(config->logLevel, "info");
}

// ✗ 差：测的是实现细节（基于交互——没必要 Mock 这里的内部方法）
TEST(ConfigLoaderTest, CallsParserCorrectly) {
    auto mockParser = std::make_unique<MockParser>();
    EXPECT_CALL(*mockParser, parse("test.json")).Times(1); // 这种测试与实现耦合
}
```

### Arrange-Act-Assert

```cpp
TEST(LoggerTest, FormatsMessageWithTimestamp) {
    // Arrange: 设置测试场景
    Logger logger(Logger::Level::Debug);

    // Act: 执行被测操作
    auto message = logger.format(Logger::Level::Info, "hello");

    // Assert: 验证结果
    EXPECT_NE(message.find("[INFO]"), std::string::npos);
    EXPECT_NE(message.find("hello"), std::string::npos);
}
```

### 每个测试只验证一个行为

```cpp
TEST(ConfigLoaderTest, RejectsEmptyFilePath) { /* ... */ }
TEST(ConfigLoaderTest, LoadsValidJson) { /* ... */ }
TEST(ConfigLoaderTest, HandlesMissingFields) { /* ... */ }
// 不要一个测试里验证三种情况
```

### 测试命名要描述行为

```cpp
// ✓ 好：读起来像规格说明
TEST(NetworkManagerTest, HandlesConnectionTimeoutGracefully) { }
TEST(NetworkManagerTest, RetriesThreeTimesByDefault) { }
TEST(NetworkManagerTest, ReturnsNulloptWhenServerUnreachable) { }

// ✗ 差：名字无意义
TEST(NetworkManagerTest, Test1) { }
TEST(NetworkManagerTest, Works) { }
```

## 内存安全检查（必须）

所有测试默认在 AddressSanitizer 下运行：

```bash
# 自动在 ASan 下编译和运行测试
cmake -S . -B build_asan -DENABLE_ASAN=ON
cmake --build build_asan
cmake --build build_asan --target test
```

```cmake
# CMake ASan 配置（Agent 自动添加）
option(ENABLE_ASAN "Enable AddressSanitizer" ON)
if(ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
```

## 常见借口

| 借口 | 现实 |
|------|------|
| "写完代码再补测试" | 你做不到。事后补的测试只测实现细节不测行为 |
| "这个太简单不用测" | 简单的代码会变复杂。测试是活的文档 |
| "编译时间本来就长，加测试更慢" | 用 doctest（编译最快）或分离测试为独立目标 |
| "手动测过了" | 手动测试不持久。明天改其他代码时可能悄悄破坏它 |

## 红旗信号

- 写了代码却没有对应的测试
- 第一次跑测试就全部通过（可能没测到真正的东西）
- Bug 修复没有复现测试
- 测试文件名不描述被测行为
- 跳过测试让套件通过

## 验证

- [ ] 每个新行为都有对应的测试
- [ ] 所有测试通过：`cmake --build build --target test`
- [ ] Bug 修复包含复现测试（修复前失败，修复后通过）
- [ ] 测试名称描述被验证的行为
- [ ] 无跳过或禁用的测试
- [ ] AddressSanitizer 无报错
