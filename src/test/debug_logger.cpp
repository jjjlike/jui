/**
 * @file debug_logger.cpp
 * @brief 调试日志管理器实现
 */

#include "jui/test/debug_logger.h"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>

namespace jui {
namespace test {

// ──────────────────────────────────────────────
// LogEntry
// ──────────────────────────────────────────────

std::string LogEntry::toString() const {
    std::ostringstream ss;
    if (!widgetId.empty()) ss << "[" << widgetId << "] ";
    ss << "[" << stage << "] ";
    ss << message;
    if (!file.empty()) ss << "  (" << file << ":" << line << ")";
    return ss.str();
}

// ──────────────────────────────────────────────
// DebugLogConfig
// ──────────────────────────────────────────────

bool DebugLogConfig::shouldLog(const std::string& widgetId, LogLevel level) const {
    if (!enabled) return false;
    if (static_cast<int>(level) < static_cast<int>(minLevel)) return false;
    if (!widgetFilters.empty()) {
        auto it = std::find(widgetFilters.begin(), widgetFilters.end(), widgetId);
        if (it == widgetFilters.end()) return false;
    }
    return true;
}

// ──────────────────────────────────────────────
// DebugLogger 单例
// ──────────────────────────────────────────────

DebugLogger& DebugLogger::instance() {
    static DebugLogger inst;
    return inst;
}

void DebugLogger::setConfig(const DebugLogConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void DebugLogger::enable(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = on;
}

void DebugLogger::setWidgetFilter(const std::vector<std::string>& widgetIds) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.widgetFilters = widgetIds;
}

void DebugLogger::addWidgetFilter(const std::string& widgetId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(config_.widgetFilters.begin(), config_.widgetFilters.end(), widgetId);
    if (it == config_.widgetFilters.end()) {
        config_.widgetFilters.push_back(widgetId);
    }
}

void DebugLogger::clearWidgetFilter() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.widgetFilters.clear();
}

void DebugLogger::log(const std::string& widgetId, LogLevel level, const std::string& stage,
                       const std::string& message, const char* file, int line, const char* func) {
    if (!config_.shouldLog(widgetId, level)) return;

    LogEntry entry;
    entry.timestamp = getTimestamp();
    entry.level = level;
    entry.widgetId = widgetId;
    entry.stage = stage;
    entry.message = message;
    if (config_.includeLocation) {
        entry.file = file ? file : "";
        entry.line = line;
        entry.function = func ? func : "";
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        logs_.push_back(entry);
    }

    if (config_.sink) {
        config_.sink(entry);
    } else {
        defaultSink(entry);
    }
}

void DebugLogger::defaultSink(const LogEntry& entry) {
    char prefix = ' ';
    switch (entry.level) {
        case LogLevel::Debug:   prefix = 'D'; break;
        case LogLevel::Info:    prefix = 'I'; break;
        case LogLevel::Warning: prefix = 'W'; break;
        case LogLevel::Error:   prefix = 'E'; break;
    }

    std::cerr << "[" << prefix << "]"
              << "[" << entry.stage << "]";
    if (!entry.widgetId.empty()) {
        std::cerr << "[" << entry.widgetId << "]";
    }
    std::cerr << " " << entry.message;
    if (!entry.file.empty()) {
        std::cerr << "  (" << entry.file << ":" << entry.line << ")";
    }
    std::cerr << std::endl;
}

std::vector<LogEntry> DebugLogger::getLogs(const std::string& widgetId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (widgetId.empty()) return logs_;
    std::vector<LogEntry> filtered;
    for (const auto& entry : logs_) {
        if (entry.widgetId == widgetId) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

std::vector<LogEntry> DebugLogger::getLogsByStage(const std::string& stage) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> filtered;
    for (const auto& entry : logs_) {
        if (entry.stage == stage) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

void DebugLogger::clearLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
}

std::string DebugLogger::exportToString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    for (const auto& entry : logs_) {
        ss << entry.toString() << "\n";
    }
    return ss.str();
}

bool DebugLogger::exportToFile(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(path);
    if (!out.is_open()) return false;
    for (const auto& entry : logs_) {
        out << entry.toString() << "\n";
    }
    return true;
}

bool DebugLogger::verifyBoundsConsistency(const std::string& widgetId,
                                           float lx, float ly, float lw, float lh,
                                           float px, float py, float pw, float ph) const {
    // 检查 layout bounds 与 paint bounds 是否一致
    if (std::abs(lx - px) > 0.5f || std::abs(ly - py) > 0.5f ||
        std::abs(lw - pw) > 0.5f || std::abs(lh - ph) > 0.5f) {
        // 通过 instance() 获取非 const 引用以记录日志
        auto& self = const_cast<DebugLogger&>(*this);
        self.log(widgetId, LogLevel::Error, "Verify",
                 format("BOUNDS_MISMATCH layout=(%.0f,%.0f)-(%.0fx%.0f) paint=(%.0f,%.0f)-(%.0fx%.0f)",
                        lx, ly, lw, lh, px, py, pw, ph),
                 __FILE__, __LINE__, __FUNCTION__);
        return false;
    }
    return true;
}

std::string DebugLogger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    std::ostringstream ss;
    ss << std::put_time(&tm_now, "%H:%M:%S")
       << "." << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}

} // namespace test
} // namespace jui
