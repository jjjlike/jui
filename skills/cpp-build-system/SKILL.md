---
name: cpp-build-system
description: C++ 构建系统管理。当遇到编译/链接错误、需要添加依赖、配置 CMake、添加测试目标、或优化编译时间时使用。管理从源码到可执行文件的整个构建链路。
---

# C++ 构建系统管理

## 概述

C++ 的构建系统是外界看来最混乱的部分之一。本技能确保 Agent 正确处理 CMake 配置、依赖管理、编译目标和跨平台构建设置。

**自动化策略：** Agent 自动检测项目构建系统、自动配置 CMake、自动管理 vcpkg/Conan 依赖。仅在引入新第三方库时请求确认。编译错误自动分析修复。

## 何时使用

- 新项目初始化（生成 CMakeLists.txt）
- 添加新的源文件/头文件
- 添加第三方依赖
- 编译/链接失败
- 添加测试/基准测试目标
- 编译太慢需要优化
- 跨平台构建配置
- 中文编码配置 / 乱码排查

## 构建系统检测

Agent 会自动检测并适配：

```bash
# 检测规则
find . -name "CMakeLists.txt" | head -1   → CMake
find . -name "*.vcxproj" | head -1         → MSBuild/Visual Studio
find . -name "meson.build" | head -1       → Meson
find . -name "BUILD" | head -1             → Bazel
find . -name "Makefile" | head -1          → Make
```

**默认使用 CMake**，除非项目已有其他系统。

## CMake 现代最佳实践

### 项目模板

```cmake
# CMakeLists.txt（顶层）
cmake_minimum_required(VERSION 3.20)
project(MyApp VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录统一
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 导出 compile_commands.json（供 clang-tidy 等工具使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 子目录
add_subdirectory(src)
add_subdirectory(tests)

# 安装规则
install(TARGETS MyApp RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
```

### 库组织

```cmake
# src/CMakeLists.txt - 源码库
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(${PROJECT_NAME}_lib PUBLIC cxx_std_20)

# 可执行文件
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)
```

**注意：** `GLOB_RECURSE` 配 `CONFIGURE_DEPENDS` 可以自动发现新文件，但正式发布时建议显式列出文件以获得确定性。

### 编译器警告

```cmake
# 始终启用严格警告
if(MSVC)
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME}_lib PRIVATE 
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
    )
endif()
```

### Sanitizer 自动化

```cmake
option(ENABLE_ASAN "Enable AddressSanitizer" ON)

if(ENABLE_ASAN AND NOT MSVC)
    target_compile_options(${PROJECT_NAME}_lib PUBLIC 
        -fsanitize=address -fno-omit-frame-pointer
    )
    target_link_options(${PROJECT_NAME}_lib PUBLIC 
        -fsanitize=address
    )
endif()
```

## 依赖管理

### 方案 1：vcpkg（推荐）

```bash
# Agent 自动集成 vcpkg
git clone https://github.com/Microsoft/vcpkg.git third_party/vcpkg
third_party/vcpkg/bootstrap-vcpkg.sh

# CMakeLists.txt
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake)
```

```bash
# 安装依赖（Agent 自动执行，新依赖需确认）
./vcpkg install gtest:x64-windows    # Google Test
./vcpkg install nlohmann-json:x64-windows  # JSON 库
./vcpkg install spdlog:x64-windows   # 日志库
./vcpkg install fmt:x64-windows      # 格式化库

# Find 和链接
find_package(GTest REQUIRED)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC GTest::gtest_main)
```

### 方案 2：Conan

```bash
# conanfile.txt
[requires]
gtest/1.14.0
fmt/10.1.0

[generators]
CMakeDeps
CMakeToolchain
```

### 方案 3：FetchContent（零预装依赖）

```cmake
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC gtest_main)
```

### 依赖引入原则

用到以下库时尽量不引入新依赖：
- **不引入 JSON 库** → C++ 标准库 + 简单的自定义解析器（如果 JSON 格式简单）
- **不引入 HTTP 库** → Windows 用 WinHTTP，Linux 用 libcurl（系统自带）
- **不引入 GUI 框架** → 优先 Win32 原生 API + Direct2D，封装轻量内部 UI 库；如项目已全面使用 Qt 则继续使用 Qt
- **不引入日志库** → Windows 用 `OutputDebugStringW` + 控制台，或封装简单日志类

## 中文编码配置（Windows 必备）

### 源码文件编码

MSVC 要求源码文件使用 **UTF-8 with BOM** 才能正确识别中文：

```cmake
# CMake 强制 MSVC 使用 UTF-8（C++20 起 MSVC 支持 /utf-8）
if(MSVC)
    # 指示 MSVC 将源码当作 UTF-8 处理（无需 BOM 也能识别中文）
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /utf-8)
    # 验证源文件无非法字符
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /validate-charset)
endif()
```

**Agent 默认行为：** 所有 `.cpp`/`.h` 文件保存为 UTF-8 with BOM；CMake 中自动添加 `/utf-8` 编译选项。

### 运行时编码（main 入口必须）

```cpp
// 每个 main()/WinMain() 函数入口处必须包含以下编码设置：

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    // 1. 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);           // 输入编码也设为 UTF-8
    
    // 2. 所有用户可见文本使用宽字符
    const wchar_t* kAppTitle = L"我的 C++ 应用程序";        // 宽字符常量
    std::wstring userName = L"用户名";                      // 宽字符串
    
    // 3. 所有 Windows API 调用使用 W 版本
    MessageBoxW(nullptr, L"操作成功完成", kAppTitle, MB_OK);
    
    // 4. 文件路径统一使用宽字符
    std::filesystem::path configDir = L"C:/用户/文档/MyApp";
    // ...
}
```

### 文件 I/O 编码安全

```cpp
// ✗ 危险：窄字符路径在非中文 Windows 上可能乱码
std::ifstream file("C:/用户/文档/config.json");

// ✓ 安全：宽字符路径 + filesystem（C++17 自动转 Unicode）
namespace fs = std::filesystem;
fs::path configPath = L"C:/用户/文档/config.json";
std::ifstream file(configPath);
```

### 常用编译问题（中文相关）

| 错误 | 原因 | 自动修复 |
|------|------|----------|
| C2001: 常量中有换行符 | 中文源码未用 UTF-8 BOM | 自动转换文件编码为 UTF-8 BOM |
| 中文注释显示乱码 | 编译器未识别 UTF-8 | 添加 `/utf-8` 编译选项 |
| MessageBox 中文乱码 | 用了 MessageBoxA | 自动改为 MessageBoxW + 宽字符串 |
| 文件打开失败 (中文路径) | 用了窄字符路径 | 自动改为 `std::filesystem::path` |
| 控制台输出乱码 | 未设置控制台编码 | 自动在 main 入口添加编码设置 |

## 编译时间优化

```cmake
# 1. 预编译头 (PCH)
target_precompile_headers(${PROJECT_NAME}_lib PRIVATE 
    <vector>
    <string>
    <memory>
    <algorithm>
    <filesystem>
)

# 2. 统一构建（Unity Build）
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)

# 3. 并行编译
# 命令行：cmake --build build -j $(nproc)
```

```bash
# 4. 按需编译（只编译改动的目标）
cmake --build build --target my_changed_lib
```

**编译时间规则：** 如果增量编译超过 30 秒，考虑：
- 减少头文件中的模板代码
- 使用 pimpl 模式隔离实现
- 拆分过大的翻译单元（`.cpp` > 1000 行）

## 跨平台构建配置

```cmake
if(WIN32)
    target_compile_definitions(${PROJECT_NAME}_lib PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME}_lib PRIVATE ws2_32)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME}_lib PRIVATE PLATFORM_MACOS)
else()
    target_compile_definitions(${PROJECT_NAME}_lib PRIVATE PLATFORM_LINUX)
    target_link_libraries(${PROJECT_NAME}_lib PRIVATE pthread)
endif()
```

## 常见编译问题速查

| 错误 | 原因 | 自动修复 |
|------|------|----------|
| `undefined reference` | 未链接库 | 自动添加 `target_link_libraries` |
| `fatal error: xxx.h: No such file` | Include 路径不对 | 自动添加 `target_include_directories` |
| `multiple definition` | 函数在头文件中定义但未加 inline | 自动加 `inline` 或移到 `.cpp` |
| `undefined symbol: vtable` | 虚函数声明了但未定义 | 自动补全缺失的实现 |
| 模板编译错误 | 模板实例化失败 | 看错误最后几行，定位到具体行 |

## 常见借口

| 借口 | 现实 |
|------|------|
| "我用 Makefile 就行了" | CMake 不完美，但它是跨平台 C++ 生态的事实标准 |
| "依赖手动管理就好" | 手动管理依赖是 C++ "依赖地狱" 的根源。用 vcpkg/Conan |
| "编译慢是正常的" | 不完全是。PCH、unity build、前向声明都能显著改善 |
| "警告不重要" | 今天的警告是明天的 Bug。务必 `-Werror` |

## 红旗信号

- 编译警告被忽略（特别是 `-Wreturn-type`、`-Wuninitialized`）
- CMakeLists.txt 中硬编码了绝对路径
- 没有使用 `target_*` 命令（仍用全局 `include_directories`/`link_libraries`）
- 第三方库的源码直接拷入项目而非用包管理器

## 验证

- [ ] `cmake -S . -B build` 配置无错误
- [ ] `cmake --build build` 编译零警告（`-Werror`）
- [ ] `ctest --test-dir build` 所有测试通过
- [ ] 新依赖通过 vcpkg/Conan/FetchContent 管理
- [ ] 无硬编码路径
- [ ] `compile_commands.json` 已生成（供工具链使用）
- [ ] 所有源文件 UTF-8 with BOM（`file *.cpp *.h` 确认编码）
- [ ] CMake 中已添加 `/utf-8` 编译选项（MSVC）
- [ ] main 入口已设置控制台编码（如使用控制台输出）
