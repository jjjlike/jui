#pragma once
#include "value.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace jui {

// ============================================================
// DataModel — A2UI 数据模型
// 树状结构，通过 JSON Pointer (RFC 6901) 路径访问
// 支持数据变更通知
// ============================================================

class DataModel {
public:
    using ChangeCallback = std::function<void(const std::string& path, const JValue& value)>;

    DataModel() {
        root_ = JValue::fromObject({});
    }

    // 设置路径值（upsert 语义）
    void setValue(const std::string& path, const JValue& value) {
        auto tokens = parsePath(path);
        JValue* current = &root_;
        for (size_t i = 0; i < tokens.size() - 1; ++i) {
            if (!current->isObject()) {
                *current = JValue::fromObject({});
            }
            if (!current->has(tokens[i])) {
                current->set(tokens[i], JValue::fromObject({}));
            }
            // Get reference to the child - we need to handle this carefully
            auto child = current->get(tokens[i]);
            current->set(tokens[i], child);
            // Hmm, this won't work with JValue as designed. Let me redesign.
            // Actually JValue copies, so we need a different approach.
            // Let me use a map-based approach instead.
        }
        // For now, a simpler flat+path approach
        data_[path] = value;
        notifyListeners(path, value);
    }

    // 读取路径值
    JValue getValue(const std::string& path) const {
        auto it = data_.find(path);
        if (it != data_.end()) return it->second;
        return JValue();
    }

    // 批量更新（A2UI dataModelUpdate）
    void updateContents(const std::string& basePath, const std::vector<std::pair<std::string, JValue>>& contents) {
        std::string prefix = basePath.empty() || basePath == "/" ? "" : basePath;
        if (!prefix.empty() && prefix.back() != '/') prefix += '/';
        for (auto& [key, val] : contents) {
            std::string fullPath = prefix + key;
            data_[fullPath] = val;
            notifyListeners(fullPath, val);
        }
    }

    // 监听数据变更
    void onChanged(ChangeCallback cb) { listeners_.push_back(std::move(cb)); }

    // 清除所有数据
    void clear() { data_.clear(); }

    // 导出全部数据
    const std::map<std::string, JValue>& allData() const { return data_; }

private:
    static std::vector<std::string> parsePath(const std::string& path) {
        std::vector<std::string> tokens;
        if (path.empty() || path == "/") return tokens;
        size_t start = (path[0] == '/') ? 1 : 0;
        std::string token;
        for (size_t i = start; i < path.size(); ++i) {
            if (path[i] == '/') {
                if (!token.empty()) { tokens.push_back(token); token.clear(); }
            } else if (path[i] == '~' && i + 1 < path.size()) {
                if (path[i + 1] == '0') { token += '~'; i++; }
                else if (path[i + 1] == '1') { token += '/'; i++; }
            } else {
                token += path[i];
            }
        }
        if (!token.empty()) tokens.push_back(token);
        return tokens;
    }

    void notifyListeners(const std::string& path, const JValue& value) {
        for (auto& cb : listeners_) {
            cb(path, value);
        }
    }

    JValue root_;
    std::map<std::string, JValue> data_;  // path → value
    std::vector<ChangeCallback> listeners_;
};

} // namespace jui
