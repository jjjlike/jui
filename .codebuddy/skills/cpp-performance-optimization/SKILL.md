---
name: cpp-performance-optimization
description: C++ 性能优化。当有性能需求、怀疑性能退化、或 profiling 发现瓶颈时使用。核心原则：先测量，再优化。不凭猜测做性能优化。
---

# C++ 性能优化

## 概述

先测量，再优化。不测量的性能工作就是猜谜——猜谜会导致过早优化，增加复杂度却不解决真正的问题。用 profiling 工具找到真正的瓶颈，修复它，再测量。

**自动化策略：** 察觉到性能问题时自动运行 profiling、自动分析热点、自动实施低风险优化（算法替换、编译器优化标志）。仅在不熟悉的热点需要你决策时才中断。

## 何时使用

- 性能需求明确存在（帧率要求、延迟预算、启动时间要求）
- 用户/测试报告慢行为
- 怀疑某次改动引入了性能退化
- 处理大数据量或高频率调用的功能

**不使用：** 没有性能证据之前不要优化。过早优化增加复杂度，收益却不确定。

## C++ 性能优化工作流

```
1. 测量     → 用真实数据建立基准
2. 定位瓶颈 → 找到真正的热点，不是猜的
3. 修复     → 针对具体瓶颈
4. 验证     → 重新测量，确认改进
5. 防护     → 加基准测试防止退化
```

### 步骤 1：测量

**CPU Profiling（Linux）：**
```bash
# perf（首选）
perf record -g ./build/bin/myapp
perf report  # 查看热点函数

# 火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

**CPU Profiling（Windows）：**
```bash
# Visual Studio 性能探查器
# 或 WPR (Windows Performance Recorder) + WPA
```

**内存 Profiling：**
```bash
# heaptrack（Linux）
heaptrack ./build/bin/myapp
heaptrack_gui heaptrack.myapp.xxx.gz  # 可视化

# massif（Valgrind 的一部分）
valgrind --tool=massif ./build/bin/myapp
ms_print massif.out.xxx
```

**微基准测试（Google Benchmark）：**
```cpp
#include <benchmark/benchmark.h>

static void BM_StringFind(benchmark::State& state) {
    std::string s = "hello world hello cpp";
    for (auto _ : state) {
        benchmark::DoNotOptimize(s.find("cpp"));
    }
}
BENCHMARK(BM_StringFind);
BENCHMARK_MAIN();
```

### 步骤 2：根据症状分类

```
什么慢？
├── 帧渲染慢 / UI 卡顿
│   └── 检查主线程的长任务 → 测量每个函数的 CPU 时间
├── 启动慢
│   └── 检查初始化代码 → 是否有不必要的同步 I/O？
├── 数据处理慢
│   └── 检查算法复杂度 → O(N²) 在大量数据上 = 灾难
├── 内存持续增长
│   └── 检查是否有无限增长的容器 → 用 heaptrack 找泄漏
└── 多线程性能不如预期
    └── 检查锁竞争 → perf lock contention
```

### 步骤 3：修复常见 C++ 性能反模式

#### 反模式 1：不必要的拷贝

```cpp
// ✗ 隐式拷贝
void processItems(std::vector<Item> items) {  // 传值 → 拷贝整个 vector！
    for (const auto& item : items) { /* ... */ }
}

// ✓ 传 const 引用
void processItems(const std::vector<Item>& items) {
    for (const auto& item : items) { /* ... */ }
}

// ✗ auto 导致拷贝
for (auto item : items) { /* item 是拷贝 */ }

// ✓ 明确指定引用
for (const auto& item : items) { /* item 是引用 */ }
```

#### 反模式 2：string 滥用

```cpp
// ✗ 不必要的 string 构造
bool hasExtension(const std::string& path, const std::string& ext) {
    return path.ends_with(ext);
}
hasExtension("/path/to/file.txt", ".txt");  // 构造了 std::string
hasExtension(myString, ".txt");             // 也构造了！

// ✓ C++20: std::string_view
bool hasExtension(std::string_view path, std::string_view ext) {
    return path.ends_with(ext);
}
hasExtension("/path/to/file.txt", ".txt");  // 零开销
hasExtension(myString, ".txt");             // 零开销
```

#### 反模式 3：错误的容器选择

```cpp
// ✗ std::map（红黑树，O(log N)）但不需要有序
std::map<int, User> users;
auto it = users.find(id);  // O(log N)

// ✓ std::unordered_map（哈希表，O(1) 平均）
std::unordered_map<int, User> users;
auto it = users.find(id);  // O(1) 平均

// ✗ 如果键是连续整数
std::unordered_map<int, User> users;

// ✓ 直接用 vector + 索引
std::vector<User> users(maxId + 1);
auto& user = users[id];    // O(1)，无哈希开销

// ✗ 频繁随机插入/删除用 vector
std::vector<int> items;
items.insert(items.begin(), 42);  // O(N)

// ✓ list 或 deque
std::deque<int> items;
items.push_front(42);  // O(1)
```

#### 反模式 4：热路径中的堆分配

```cpp
// ✗ 热循环中分配堆内存
for (int i = 0; i < 1000000; ++i) {
    auto data = std::make_unique<Buffer>(1024);  // 百万次堆分配！
    process(data.get());
}

// ✓ 预分配并复用
auto data = std::make_unique<Buffer>(1024);
for (int i = 0; i < 1000000; ++i) {
    data->reset();  // 复用同一个 buffer
    process(data.get());
}

// ✓ 或者用栈分配（小对象）
for (int i = 0; i < 1000000; ++i) {
    std::array<char, 1024> data{};  // 栈分配
    process(data.data());
}
```

#### 反模式 5：虚函数在热路径中

```cpp
// ✗ 每帧调用虚函数百万次
class Renderable {
public:
    virtual void draw() = 0;  // 虚函数调用开销大
};
for (auto& obj : objects) {
    obj->draw();  // 虚函数分派 + 缓存未命中
}

// ✓ 按类型分组批量处理，或用 std::variant
std::vector<Triangle> triangles;
std::vector<Circle> circles;
for (const auto& t : triangles) drawTriangle(t);  // 无虚函数开销
for (const auto& c : circles) drawCircle(c);
```

#### 反模式 6：抑制编译器优化（别名分析）

```cpp
// ✗ restrict/noalias 问题 —— 编译器假设指针可能重叠
void addArrays(float* a, float* b, float* c, int n) {
    for (int i = 0; i < n; i++) {
        a[i] = b[i] + c[i];  // 编译器每次迭代都重新读取 b[i] 和 c[i]
    }                          // 因为它不确定 a 是否与 b 或 c 重叠
}

// ✓ C++ 中，使用 __restrict（非标准但广泛支持）或确保局部性
void addArrays(float* __restrict a, const float* __restrict b, 
               const float* __restrict c, int n) {
    for (int i = 0; i < n; i++) {
        a[i] = b[i] + c[i];  // 编译器现在可以向量化
    }
}

// ✓ 更好的做法：用 std::valarray 或 std::transform
std::transform(b, b + n, c, a, std::plus<>{});
```

## 编译器优化建议

```cmake
# CMake Release 构建配置（Agent 自动设置）
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto" CACHE STRING "")

# -O3: 最高优化等级（比 -O2 更激进的内联和向量化）
# -march=native: 生成针对当前 CPU 的最优指令（SSE/AVX）
# -flto: 链接时优化（跨翻译单元内联）
```

**注意：** `-march=native` 生成的二进制可能不兼容其他 CPU。如果不确定部署到哪些机器，用 `-march=x86-64-v3` 作为更兼容的选择。

## 常见借口

| 借口 | 现实 |
|------|------|
| "以后优化" | 明显的反模式（O(N²) 算法）现在就要修。微优化可以以后做 |
| "我机器上很快" | 你的机器不是用户的。在代表性硬件和负载下测试 |
| "这个优化很显然" | 没测量就是不知道。先 profiling |
| "编译器会帮我优化" | 编译器能优化掉一些低效，但不能修复 O(N²) 算法 |

## 红旗信号

- 没有 profiling 数据就开始优化
- 热路径中有不必要的堆分配
- O(N²) 算法处理可能很大的 N
- `std::map` 用在不需要排序的场景
- 按值传递大对象（vector、string、自定义大类型）
- `auto` 导致的隐式拷贝

## 验证

- [ ] 修改前后的测量数据存在（具体数字，不是"感觉快了"）
- [ ] 具体瓶颈已定位并修复
- [ ] 基准测试显示显著改进
- [ ] 已有测试仍然通过（优化没破坏正确性）
- [ ] ASan 无报错
