---
name: cpp-memory-safety
description: C++ 内存安全检查。当完成代码后、怀疑有内存泄漏、ASan 报错、遇到崩溃、或审查指针/资源管理代码时使用。确保所有内存和资源用 RAII 管理、无悬空指针、无泄漏。
---

# C++ 内存安全

## 概述

C++ 赋予开发者对内存的完全控制，同时也把内存错误的全部责任交给了开发者。本技能确保代码中所有内存和资源安全——正确使用 RAII、智能指针、无泄漏、无悬空指针、无越界访问。

**自动化策略：** 每次编译和测试后自动运行 AddressSanitizer 检查。自动检测并修复常见内存问题（裸 new/delete → 智能指针，原始数组 → std::vector/std::array）。仅在需要重新设计所有权模型的复杂场景下才中断。

## 何时使用

- 写完任何涉及指针或资源管理的代码后
- ASan/Valgrind 报错时
- 程序偶尔崩溃（尤其 SIGSEGV）
- 怀疑内存泄漏
- PR 审查中看到裸指针时
- 涉及多线程共享数据的代码

## 内存安全黄金法则

### 法则 1：永远不要显式 new / delete

```cpp
// ✗ 绝对禁止
MyClass* obj = new MyClass();
// ... 
delete obj;  // 如果中间抛异常就泄漏

// ✓ 总是用智能指针
auto obj = std::make_unique<MyClass>();
// 作用域结束自动释放，异常安全

// ✓ 共享所有权时
auto obj = std::make_shared<MyClass>();
```

**唯一例外：** 实现自定义容器的底层内存管理（如手写 allocator）。

### 法则 2：RAII 管理一切资源

```cpp
// ✗ 手动管理非内存资源
class FileHolder {
    FILE* m_file;
public:
    void open(const char* path) { m_file = fopen(path, "r"); }
    void close() { fclose(m_file); }  // 调用者可能忘记调用
};

// ✓ RAII 封装
class FileHolder {
    std::unique_ptr<FILE, decltype(&fclose)> m_file{nullptr, fclose};
public:
    bool open(const char* path) {
        auto* f = fopen(path, "r");
        m_file.reset(f);
        return f != nullptr;
    }
    // 析构自动 fclose
};
```

RAII 适用于一切需要"获取-释放"配对的资源：文件、锁、套接字、GDI 对象、数据库连接。

### 法则 3：智能指针选择指南

| 场景 | 使用 | 示例 |
|------|------|------|
| 独占所有权 | `std::unique_ptr` | 工厂函数返回值 |
| 共享所有权 | `std::shared_ptr` | 多线程共享的缓存 |
| 不拥有，可能为空 | 裸指针 `T*` | 函数参数（不转移所有权） |
| 不拥有，不可能为空 | `T&` 引用 | 必需的依赖 |
| 弱引用（避免循环） | `std::weak_ptr` | 观察者模式中的 subject |
| 可选值 | `std::optional<T>` | 可能失败的查询结果 |

```cpp
// 函数参数的所有权语义要明确

// 只读取，不拥有
void processUser(const User& user);
void processUser(const User* user);  // 可能为空

// 接管所有权
void storeUser(std::unique_ptr<User> user);

// 共享所有权
void cacheUser(std::shared_ptr<User> user);
```

### 法则 4：防止悬空指针

```cpp
// ✗ 危险：返回局部变量引用
const std::string& getDefaultName() {
    auto name = std::string("default");  // 局部变量
    return name;  // 返回时 name 被销毁 → 悬空引用！
}

// ✓ 安全：返回值
std::string getDefaultName() {
    return "default";
}

// ✗ 危险：容器扩容后使用旧迭代器
std::vector<int> v{1, 2, 3};
auto it = v.begin();
v.push_back(4);  // 可能触发扩容 → it 悬空
v.push_back(5);  // 可能触发扩容 → it 悬空
*it = 10;        // 悬空迭代器 → 未定义行为！

// ✓ 安全：先 reserve 或在修改后重新获取
std::vector<int> v{1, 2, 3};
v.reserve(100);  // 预留足够空间
auto it = v.begin();
v.push_back(4);  // 不会扩容
*it = 10;        // 安全
```

### 法则 5：警惕 Lambda 捕获

```cpp
// ✗ 危险：按引用捕获局部变量
auto makeCallback() {
    int local = 42;
    return [&]() { return local; };  // local 在函数返回后销毁！
}

// ✓ 安全：按值捕获
auto makeCallback() {
    int local = 42;
    return [=]() { return local; };
}

// ✓ 安全：捕获 shared_ptr 延长生命周期
auto makeCallback(std::shared_ptr<Data> data) {
    return [data]() { return data->value; };  // shared_ptr 保证 data 存活
}
```

### 法则 6：初始化一切

```cpp
// ✗ 危险：未初始化
int x;          // x 的值是垃圾
int* p;         // p 是野指针
Config cfg;     // cfg 成员都是垃圾值

// ✓ 总是初始化
int x = 0;
int* p = nullptr;
Config cfg{};   // 零初始化所有成员
```

### 法则 7：了解 Rule of Five

如果一个类管理资源，明确定义或显式删除五大函数：

```cpp
class Buffer {
    std::unique_ptr<char[]> m_data;
    size_t m_size;

public:
    // 构造函数
    explicit Buffer(size_t size) : m_data(std::make_unique<char[]>(size)), m_size(size) {}
    
    // 移动语义
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;
    
    // 禁止拷贝（unique_ptr 不能拷贝）
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    // 析构由 unique_ptr 自动处理
    ~Buffer() = default;
};
```

**或者，直接让成员都是 RAII 类型，不需要手动定义五大函数（Rule of Zero）。**

## 自动化检查流程

Agent 在每次编译/测试后自动执行：

```bash
# 1. AddressSanitizer 编译
cmake -S . -B build_asan -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build_asan
ctest --test-dir build_asan

# 2. UndefinedBehaviorSanitizer
cmake -S . -B build_ubsan -DENABLE_UBSAN=ON
cmake --build build_ubsan
ctest --test-dir build_ubsan

# 3. 如果代码涉及多线程
cmake -S . -B build_tsan -DENABLE_TSAN=ON
cmake --build build_tsan
ctest --test-dir build_tsan

# 4. clang-tidy 内存相关检查
clang-tidy src/**/*.cpp --checks='cppcoreguidelines-owning-memory, \
    cppcoreguidelines-no-malloc, \
    modernize-use-smart-pointers, \
    bugprone-suspicious-*'
```

## 常见借口

| 借口 | 现实 |
|------|------|
| "智能指针有性能开销" | `unique_ptr` 零开销。`shared_ptr` 有引用计数开销，但比手动管理泄漏的代价小得多 |
| "这段代码很简单，不需要 RAII" | 简单的代码明天会变复杂。现在不用 RAII 就埋下了隐患 |
| "我不需要拷贝，所以裸指针够用" | 那也请用 `unique_ptr`。它不比裸指针多任何开销，但语义更清晰 |
| "事后用 Valgrind 检查" | Valgrind 只能告诉你已经存在的问题。RAII 从一开始就防止问题 |

## 红旗信号

- 代码中出现 `new` 或 `delete`
- 返回引用/指针指向局部变量
- 类有裸指针成员但没有析构函数
- `std::shared_ptr` 可能导致循环引用（通常用 `weak_ptr`）
- Lambda 按引用捕获后传递给异步操作
- 容器扩容后使用之前的迭代器

## 验证

- [ ] 代码中无裸 `new`/`delete`（除非实现底层容器）
- [ ] 所有资源都用 RAII 管理
- [ ] 指针/引用参数的所有权语义明确
- [ ] 无悬空指针/引用风险
- [ ] Lambda 捕获安全（异步场景尤其注意）
- [ ] AddressSanitizer 无报错
- [ ] UndefinedBehaviorSanitizer 无报错
- [ ] clang-tidy 内存相关检查无严重警告
