/**
 * @file debug_logger.h
 * @brief JUI 自动化测试专用调试日志系统
 * 
 * 功能特性：
 *   - 全局开关控制日志输出
 *   - 按控件ID过滤日志
 *   - 统一坐标系记录（屏幕坐标系）
 *   - 自动捕获函数名、文件名、行号
 * 
 * 使用示例：
 *   JUI_DEBUG_LOG("txt", "Layout bounds: ({},{}),{}x{}", x, y, w, h);
 *   JUI_DEBUG_LOG_IF("txt", bounds.w > 0, "Valid width: {}", bounds.w);
 */

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include <mutex>

namespace jui {
namespace test {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug,      ///< 详细调试信息
    Info,       ///< 一般信息
    Warning,    ///< 警告
    Error       ///< 错误
};

/**
 * @brief 单条日志记录结构
 */
struct LogEntry {
    std::string timestamp;      ///< 时间戳
    LogLevel level;             ///< 日志级别
    std::string widgetId;       ///< 关联控件ID（空表示全局）
    std::string stage;          ///< 阶段标识（Layout/Paint/HitTest等）
    std::string message;        ///< 日志消息
    std::string file;           ///< 源文件
    int line;                   ///< 行号
    std::string function;       ///< 函数名
    
    std::string toString() const;
};

/**
 * @brief 调试日志配置
 */
struct DebugLogConfig {
    bool enabled = false;                       ///< 全局开关
    LogLevel minLevel = LogLevel::Debug;        ///< 最低日志级别
    std::vector<std::string> widgetFilters;     ///< 控件ID白名单（空=全部）
    bool includeTimestamp = true;               ///< 包含时间戳
    bool includeLocation = true;                ///< 包含文件位置
    std::function<void(const LogEntry&)> sink;  ///< 自定义输出目标
    
    bool shouldLog(const std::string& widgetId, LogLevel level) const;
};

/**
 * @brief 调试日志管理器（单例）
 */
class DebugLogger {
public:
    static DebugLogger& instance();
    
    // 配置管理
    void setConfig(const DebugLogConfig& config);
    const DebugLogConfig& config() const { return config_; }
    
    // 开关控制
    void enable(bool on = true);
    bool isEnabled() const { return config_.enabled; }
    
    // 控件过滤
    void setWidgetFilter(const std::vector<std::string>& widgetIds);
    void addWidgetFilter(const std::string& widgetId);
    void clearWidgetFilter();
    
    // 日志记录（内部使用宏调用）
    void log(const std::string& widgetId, LogLevel level, const std::string& stage,
             const std::string& message, const char* file, int line, const char* func);
    
    // 格式化辅助（静态方法）
    template<typename... Args>
    static std::string format(const char* fmt, Args... args);
    
    // 检索日志
    std::vector<LogEntry> getLogs(const std::string& widgetId = "") const;
    std::vector<LogEntry> getLogsByStage(const std::string& stage) const;
    void clearLogs();
    
    // 导出
    std::string exportToString() const;
    bool exportToFile(const std::string& path) const;
    
    // 断言辅助
    bool verifyBoundsConsistency(const std::string& widgetId,
                                  float lx, float ly, float lw, float lh,
                                  float px, float py, float pw, float ph) const;

private:
    DebugLogger() = default;
    ~DebugLogger() = default;
    DebugLogger(const DebugLogger&) = delete;
    DebugLogger& operator=(const DebugLogger&) = delete;
    
    DebugLogConfig config_;
    std::vector<LogEntry> logs_;
    mutable std::mutex mutex_;
    
    std::string getTimestamp() const;
    void defaultSink(const LogEntry& entry);
};

// 宏定义（自动捕获位置信息）
#define JUI_DEBUG_LOG(widgetId, stage, ...) \
    jui::test::DebugLogger::instance().log(widgetId, jui::test::LogLevel::Debug, stage, \
        jui::test::DebugLogger::instance().format(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

#define JUI_DEBUG_LOG_IF(widgetId, stage, condition, ...) \
    do { if (condition) JUI_DEBUG_LOG(widgetId, stage, __VA_ARGS__); } while(0)

#define JUI_INFO_LOG(widgetId, stage, ...) \
    jui::test::DebugLogger::instance().log(widgetId, jui::test::LogLevel::Info, stage, \
        jui::test::DebugLogger::instance().format(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

#define JUI_WARN_LOG(widgetId, stage, ...) \
    jui::test::DebugLogger::instance().log(widgetId, jui::test::LogLevel::Warning, stage, \
        jui::test::DebugLogger::instance().format(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

#define JUI_ERROR_LOG(widgetId, stage, ...) \
    jui::test::DebugLogger::instance().log(widgetId, jui::test::LogLevel::Error, stage, \
        jui::test::DebugLogger::instance().format(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

// 布局/渲染专用宏（带坐标系标记）
#define JUI_LOG_LAYOUT(widgetId, bounds, expected) \
    JUI_DEBUG_LOG(widgetId, "Layout", "LAYOUT[Screen] id={} bounds=({},{})-({}x{}) expected=({},{})-({}x{})", \
        widgetId, bounds.x, bounds.y, bounds.w, bounds.h, expected.x, expected.y, expected.w, expected.h)

#define JUI_LOG_PAINT(widgetId, bounds, clipRect) \
    JUI_DEBUG_LOG(widgetId, "Paint", "PAINT[Screen] id={} bounds=({},{})-({}x{}) clip=({},{})-({}x{})", \
        widgetId, bounds.x, bounds.y, bounds.w, bounds.h, clipRect.x, clipRect.y, clipRect.w, clipRect.h)

#define JUI_LOG_HITTEST(widgetId, x, y, hit) \
    JUI_DEBUG_LOG(widgetId, "HitTest", "HITTEST[Screen] id={} point=({},{}) hit={}", widgetId, x, y, hit)

} // namespace test
} // namespace jui

// 模板实现
namespace jui {
namespace test {

template<typename... Args>
std::string DebugLogger::format(const char* fmt, Args... args) {
    char buffer[1024];
    int n = snprintf(buffer, sizeof(buffer), fmt, args...);
    if (n < 0 || n >= (int)sizeof(buffer)) {
        return std::string("[format error/truncated]");
    }
    return std::string(buffer);
}

} // namespace test
} // namespace jui
