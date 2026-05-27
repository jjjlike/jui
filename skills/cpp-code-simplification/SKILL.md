---
name: cpp-code-simplification
description: C++ 代码简化。当感觉代码过于复杂、模板层级太深、抽象不必要、或有更简单直接的实现方式时使用。消灭过度设计，回归简洁。
---

# C++ 代码简化

## 概述

C++ 极其强大，也因此极其容易被过度设计。模板元编程、继承层级、设计模式——这些工具是好的，但在不需要时就成了负债。本技能帮助你把代码从"聪明"拉回"简单"。

**自动化策略：** 遇到复杂代码时自动应用简化规则，无需确认。简化后自动编译测试验证。仅在需要删除你写的代码时才提醒（避免误删你认为有价值的部分）。

## 何时使用

- 一个功能可以用更少代码实现
- 模板层级超过 2 层
- 继承层级超过 3 层
- 一个函数超过 50 行
- 出现了"为了以后扩展"的未使用抽象
- 代码需要注释才能理解它在干嘛

## C++ 简化黄金法则

### 法则 1：先问"标准库能搞定吗？"

```cpp
// ✗ 手写排序
void sortItems(std::vector<int>& items) {
    for (size_t i = 0; i < items.size(); i++) {
        for (size_t j = i + 1; j < items.size(); j++) {
            if (items[i] > items[j]) std::swap(items[i], items[j]);
        }
    }
}

// ✓ 标准库
#include <algorithm>
std::sort(items.begin(), items.end());
```

```cpp
// ✗ 手写查找
bool contains(const std::vector<std::string>& v, const std::string& s) {
    for (const auto& item : v) {
        if (item == s) return true;
    }
    return false;
}

// ✓ 标准库
#include <algorithm>
bool contains(const auto& v, const auto& s) {
    return std::ranges::find(v, s) != v.end();
}
```

### 法则 2：能用函数就不用模板

```cpp
// ✗ 不必要的模板（两种类型实际行为相同）
template<typename T>
T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// ✓ 需要时才用，不需要就先写具体的
int clampInt(int value, int min, int max);
double clampDouble(double value, double min, double max);
// 如果确实有 5+ 种类型都需要，再提炼为模板
```

### 法则 3：类层次不要超过 3 层

```cpp
// ✗ 过度继承
class Animal { virtual void eat() = 0; };
class Mammal : public Animal { virtual void nurse() = 0; };
class Carnivore : public Mammal { virtual void hunt() = 0; };
class Canine : public Carnivore { virtual void howl() = 0; };
class Dog : public Canine { void eat() override { /* ... */ } };
// 5 层继承！没人能一眼看懂 Dog 到底能做什么

// ✓ 扁平化
class Dog {
    void eat();
    void nurse();
    void hunt();
    void howl();
};
// 清晰、直接、没有虚函数开销
```

### 法则 4：Lambda 不超过 5 行

```cpp
// ✗ Lambda 太复杂
auto handler = [this, &state, &cache](const Request& req) -> Response {
    if (!req.isValid()) {
        Logger::warn("Invalid request from {}", req.source());
        return Response::error(400);
    }
    auto cached = cache.get(req.id());
    if (cached) { return *cached; }
    auto result = this->process(req);
    if (result.isOk()) {
        cache.put(req.id(), result);
    }
    return result.toResponse();
};

// ✓ 提取为普通成员函数
Response handleRequest(const Request& req) {
    if (!req.isValid()) {
        Logger::warn("Invalid request from {}", req.source());
        return Response::error(400);
    }
    auto cached = m_cache.get(req.id());
    if (cached) { return *cached; }
    auto result = process(req);
    if (result.isOk()) { m_cache.put(req.id(), result); }
    return result.toResponse();
}
```

### 法则 5：用 RAII 替代手动资源管理

```cpp
// ✗ 手动 new/delete
void processData(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return;
    char* buffer = new char[1024];
    fread(buffer, 1, 1024, f);
    // ... 处理 ...
    delete[] buffer;  // 如果中间抛异常，这行不会执行 → 泄漏
    fclose(f);        // 同样可能泄漏
}

// ✓ RAII
void processData(const std::string& filename) {
    auto file = std::unique_ptr<FILE, decltype(&fclose)>{
        fopen(filename.c_str(), "r"), fclose
    };
    if (!file) return;
    auto buffer = std::make_unique<char[]>(1024);
    fread(buffer.get(), 1, 1024, file.get());
    // ... 处理 ...
    // 异常安全，自动清理
}
```

### 法则 6：prefer `std::optional` 替代输出参数

```cpp
// ✗ 输出参数 + bool 返回
bool findUser(int id, UserInfo& out) {
    auto it = users.find(id);
    if (it == users.end()) return false;
    out = it->second;
    return true;
}

// ✓ std::optional
std::optional<UserInfo> findUser(int id) {
    auto it = users.find(id);
    if (it == users.end()) return std::nullopt;
    return it->second;
}
```

### 法则 7：消除未使用的抽象

```cpp
// ✗ 只有一个实现的接口
class IDataProvider {
public:
    virtual ~IDataProvider() = default;
    virtual Data fetch(int id) = 0;
};

class SqlDataProvider : public IDataProvider {
    Data fetch(int id) override { /* ... */ }
};

// ✓ 如果没有第二个实现，接口就是多余的
class DataProvider {
public:
    Data fetch(int id);
    // 如果以后需要多态，再加接口不迟
};
```

## 简化检查清单

每完成一段代码后自动检查：

- [ ] 标准库是否已有现成的？（`<algorithm>`、`<numeric>`、`<ranges>`）
- [ ] 这个模板是否可以用具体类型替代？
- [ ] 这个继承是否可以用组合替代？
- [ ] 这个 `new`/`delete` 是否可以用智能指针替代？
- [ ] 这个 Lambda 是否超过 5 行？（应提取为函数）
- [ ] 这个函数是否超过 50 行？（应拆分）
- [ ] 有只被调用一次的辅助函数吗？（内联它）
- [ ] 有"为了将来的需要"预留的接口吗？（删掉）

## 常见借口

| 借口 | 现实 |
|------|------|
| "以后可能会需要这个抽象" | YAGNI。等真正需要时再加，成本更低 |
| "模板让代码更通用" | 通用是有成本的。为当前需求写代码，不要为想象的需求写 |
| "标准库太慢" | 先测量。标准库实现通常比你手写的快且无 Bug |
| "性能第一，可读性第二" | 不可读的代码没法优化。先写对，再写快 |

## 红旗信号

- 一个功能文件超过 500 行
- 模板嵌套超过 2 层（T1<T2<T3>> 及更深）
- 出现了 CRTP、SFINAE、Policy-based design 但不是必需的
- 一半的代码是 `virtual` 函数
- 头文件中出现过度的 `#include`（能前向声明的不前向声明）

## 验证

简化后：
- [ ] `cmake --build build` 编译通过
- [ ] `ctest --test-dir build` 测试全部通过
- [ ] 代码行数减少（通常目标：减少 20%+ 但不损失功能）
- [ ] 无需额外注释就能理解意图
