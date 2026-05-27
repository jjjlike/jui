#pragma once
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace jui {

// ============================================================
// JValue — A2UI 兼容的动态值类型
// 支持: literalString, literalNumber, literalBool, path 绑定
// ============================================================

enum class ValueType {
    Null,
    String,
    Number,
    Boolean,
    Path,       // JSON Pointer 路径绑定
    Array,
    Object
};

class JValue {
public:
    JValue() : type_(ValueType::Null) {}
    explicit JValue(std::nullptr_t) : type_(ValueType::Null) {}
    explicit JValue(const char* s) : type_(ValueType::String), str_(s) {}
    explicit JValue(const std::string& s) : type_(ValueType::String), str_(s) {}
    explicit JValue(double n) : type_(ValueType::Number), num_(n) {}
    explicit JValue(int n) : type_(ValueType::Number), num_(static_cast<double>(n)) {}
    explicit JValue(bool b) : type_(ValueType::Boolean), bool_(b) {}

    static JValue fromPath(const std::string& path) {
        JValue v;
        v.type_ = ValueType::Path;
        v.str_ = path;
        return v;
    }

    static JValue fromArray(const std::vector<JValue>& arr) {
        JValue v;
        v.type_ = ValueType::Array;
        v.arr_ = std::make_shared<std::vector<JValue>>(arr);
        return v;
    }

    static JValue fromObject(const std::map<std::string, JValue>& obj) {
        JValue v;
        v.type_ = ValueType::Object;
        v.obj_ = std::make_shared<std::map<std::string, JValue>>(obj);
        return v;
    }

    ValueType type() const { return type_; }
    bool isNull() const { return type_ == ValueType::Null; }
    bool isString() const { return type_ == ValueType::String; }
    bool isNumber() const { return type_ == ValueType::Number; }
    bool isBoolean() const { return type_ == ValueType::Boolean; }
    bool isPath() const { return type_ == ValueType::Path; }
    bool isArray() const { return type_ == ValueType::Array; }
    bool isObject() const { return type_ == ValueType::Object; }

    const std::string& asString() const { return str_; }
    double asNumber() const { return num_; }
    int asInt() const { return static_cast<int>(num_); }
    bool asBool() const { return bool_; }

    const std::vector<JValue>& asArray() const { return *arr_; }
    std::vector<JValue>& asArray() { return *arr_; }

    const std::map<std::string, JValue>& asObject() const { return *obj_; }

    // 对象操作
    void set(const std::string& key, const JValue& val) {
        if (type_ != ValueType::Object) {
            type_ = ValueType::Object;
            obj_ = std::make_shared<std::map<std::string, JValue>>();
        }
        (*obj_)[key] = val;
    }

    bool has(const std::string& key) const {
        return type_ == ValueType::Object && obj_->count(key) > 0;
    }

    JValue get(const std::string& key) const {
        if (type_ == ValueType::Object) {
            auto it = obj_->find(key);
            if (it != obj_->end()) return it->second;
        }
        return JValue();
    }

    // 数组操作
    void push(const JValue& val) {
        if (type_ != ValueType::Array) {
            type_ = ValueType::Array;
            arr_ = std::make_shared<std::vector<JValue>>();
        }
        arr_->push_back(val);
    }

    size_t size() const {
        if (type_ == ValueType::Array) return arr_->size();
        if (type_ == ValueType::Object) return obj_->size();
        return 0;
    }

private:
    ValueType type_ = ValueType::Null;
    std::string str_;
    double num_ = 0;
    bool bool_ = false;
    std::shared_ptr<std::vector<JValue>> arr_;
    std::shared_ptr<std::map<std::string, JValue>> obj_;
};

} // namespace jui
