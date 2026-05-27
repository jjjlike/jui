---
name: cpp-debugging
description: C++ 系统性根因调试。当测试失败、编译报错、运行时崩溃、行为不符合预期或遇到任何意外错误时使用。当需要系统性方法而非猜测来查找和修复根因时使用。
---

# C++ 调试与错误恢复

## 概述

系统性调试+结构化分类。当出现问题——崩溃、错误、行为异常时——停止加功能，保存证据，按照结构化流程找到并修复根因。猜测浪费时间。

**自动化策略：** 自动复现问题、自动运行 Sanitizer 诊断、自动定位、自动修复并添加回归测试。仅在无法自动复现或需要你做决策时才中断。

## 何时使用

- 编译失败（链接错误、模板错误、头文件找不到）
- 运行时崩溃（段错误 SIGSEGV、非法指令 SIGILL、异常未捕获）
- 测试失败
- 行为不符合预期（逻辑 Bug、数据错误）
- 内存问题（ASan 报错、Valgrind 报错）
- 之前能正常工作的代码现在坏了

## 停止流水线规则

当任何异常发生时：

```
1. 停止添加功能或做其他改动
2. 保存证据（错误输出、ASan 报告、core dump）
3. 诊断（使用分类清单）
4. 修复根因
5. 防护（加回归测试）
6. 验证通过后才恢复工作
```

**不要跳过失败的测试或编译错误去写下一个功能。** 错误会叠加。

## C++ 诊断工具箱

### 第一道防线：编译器

```bash
# 自动启用所有合理警告
-Wall -Wextra -Wpedantic -Wshadow -Wconversion
# MSVC: /W4

# 将警告视为错误（必须）
-Werror
# MSVC: /WX
```

### 第二道防线：Sanitizer（必须启用）

```bash
# AddressSanitizer — 检测内存错误
-fsanitize=address

# UndefinedBehaviorSanitizer — 检测未定义行为
-fsanitize=undefined

# ThreadSanitizer — 检测数据竞争
-fsanitize=thread

# LeakSanitizer — 检测内存泄漏（ASan 自带）
# 运行程序后自动输出泄漏报告
```

### 第三道防线：静态分析

```bash
# clang-tidy（自动运行）
clang-tidy src/*.cpp -- -std=c++17

# 关注以下检查：
# - bugprone-*（常见 Bug 模式）
# - modernize-*（现代化建议）
# - performance-*（性能问题）
# - cppcoreguidelines-*（C++ Core Guidelines）
```

## 六步诊断清单

### 步骤 1：复现

确保能稳定复现失败。不能复现就无法自信修复。

```bash
# 自动复现策略
# 运行失败的测试
ctest --test-dir build -R "failing_test_name" --output-on-failure

# 用 ASan 复现
ASAN_OPTIONS=halt_on_error=1 ./build/bin/myapp

# 用 GDB 捕获崩溃
gdb -batch -ex run -ex bt ./build/bin/myapp

# Windows: 用 WinDbg / Visual Studio Debugger
```

### 步骤 2：定位（二分法）

对于回归 Bug，用 `git bisect` 定位引入的 commit：

```bash
git bisect start
git bisect bad HEAD
git bisect good v1.2.3
# 自动化二分
git bisect run cmake --build build && ctest --test-dir build -R "failing_test"
```

对于复杂代码的 Bug，用二分注释法：

```cpp
// 注释掉一半代码，看 Bug 是否还在
// 反复缩窄范围直到定位到具体行
```

### 步骤 3：缩减

创建最小复现用例：
- 移除无关的代码/配置，只保留 Bug 本身
- 把输入简化到最小触发条件
- 剥离测试到仅剩复现所需的最少代码

### 步骤 4：修复根因

修复底层问题，不是症状：

```
症状："程序在处理大文件时随机崩溃"

症状修复（错）：
  → 加 try-catch 吞掉异常

根因修复（对）：
  → std::vector 在 push_back 时扩容导致迭代器失效
  → 改用 reserve() 预分配，或用索引替代迭代器
```

问"为什么会发生？"直到到达真正原因。

### 步骤 5：加防护

写一个能捕获此错误的回归测试：

```cpp
// Bug: 空路径导致 ConfigLoader 崩溃
TEST(ConfigLoaderDeathTest, EmptyPathAsserts) {
    // 验证传入空路径时程序不会继续执行（assert 或异常）
    EXPECT_THROW(ConfigLoader(""), std::invalid_argument);
}
```

### 步骤 6：端到端验证

```bash
cmake --build build          # 编译通过
ctest --test-dir build       # 所有测试通过
# ASan 运行无报错
# 手动跑一遍关键路径
```

## C++ 典型错误分类

### 编译错误分类

```
编译失败：
├── 模板错误（最难读）
│   └── 看错误信息最后几行 → 找到第一个 "required from here"
├── 链接错误（undefined reference）
│   └── 检查 CMakeLists.txt 中是否链接了所需库
├── Include 找不到
│   └── 检查 include_directories 和 target_include_directories
├── 重定义错误
│   └── 检查 #pragma once 或 include guard
└── static_assert 失败
    └── 阅读 assert 信息，通常已经说明了问题
```

### 运行时崩溃分类

```
崩溃类型：
├── SIGSEGV（段错误）→ 空指针、野指针、数组越界
│   └── 先用 ASan 跑一遍，通常能直接定位到行
├── SIGABRT → assert 失败、throw 未捕获、double-free
│   └── GDB 看调用栈
├── std::bad_alloc → 内存不足
│   └── 检查是否有无限增长的容器
├── 数据竞争 → ThreadSanitizer
│   └── 检查共享状态访问是否有锁/原子操作保护
└── 死锁 → 程序卡住不退出
    └── 检查锁的获取顺序是否一致
```

## 常见借口

| 借口 | 现实 |
|------|------|
| "我知道 Bug 在哪，直接修" | 你可能 70% 是对的。剩下 30% 会浪费数小时。先复现 |
| "在我的机器上没问题" | 环境不同。用 Sanitizer + CI 统一环境 |
| "这个测试不稳定，忽略它" | 不稳定测试会掩盖真实 Bug。找到原因而非忽略 |
| "下次 commit 再修" | 现在修。下次 commit 会在这个 Bug 上叠加新 Bug |

## 红旗信号

- 跳过失败测试继续写新功能
- 不先复现就猜测修复
- 修症状不修根因
- "好了但不知道为什么"——没有理解什么变了
- Bug 修复后没加回归测试
- 调试时同时改了多个无关的东西

## 验证

修复 Bug 后：

- [ ] 根因已定位并记录
- [ ] 修复针对根因而非症状
- [ ] 回归测试存在（修复前失败，修复后通过）
- [ ] 所有已有测试通过
- [ ] 编译通过
- [ ] ASan 无报错
- [ ] 原始 Bug 场景端到端验证通过
