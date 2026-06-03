/**
 * @file validator.cpp
 * @brief UiValidator 实现
 */
#include "jui/test/validator.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace jui::test {

UiValidator& UiValidator::expectWidget(const std::string& id) {
    currentId_ = id;
    return *this;
}

// ---- 存在性 ----
UiValidator& UiValidator::exists() {
    auto s = snapshot(currentId_);
    addResult(s.exists, "exists", "true", s.exists ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::notExists() {
    auto s = snapshot(currentId_);
    addResult(!s.exists, "notExists", "false", s.exists ? "true" : "false");
    return *this;
}

// ---- 可见性 ----
UiValidator& UiValidator::visible() {
    auto s = snapshot(currentId_);
    addResult(s.visible, "visible", "true", s.visible ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::notVisible() {
    auto s = snapshot(currentId_);
    addResult(!s.visible, "notVisible", "false", s.visible ? "true" : "false");
    return *this;
}

// ---- 启用 ----
UiValidator& UiValidator::enabled() {
    auto s = snapshot(currentId_);
    addResult(s.enabled, "enabled", "true", s.enabled ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::disabled() {
    auto s = snapshot(currentId_);
    addResult(!s.enabled, "disabled", "true", s.enabled ? "true" : "false");
    return *this;
}

// ---- 文本 ----
UiValidator& UiValidator::hasText(const std::string& expected) {
    auto s = snapshot(currentId_);
    addResult(s.text == expected, "hasText", expected, s.text);
    return *this;
}

UiValidator& UiValidator::hasTextContaining(const std::string& substring) {
    auto s = snapshot(currentId_);
    addResult(s.text.find(substring) != std::string::npos,
              "hasTextContaining", substring, s.text);
    return *this;
}

UiValidator& UiValidator::isEmpty() {
    auto s = snapshot(currentId_);
    addResult(s.text.empty(), "isEmpty", "true", s.text);
    return *this;
}

UiValidator& UiValidator::notEmpty() {
    auto s = snapshot(currentId_);
    addResult(!s.text.empty(), "notEmpty", "true", s.text);
    return *this;
}

// ---- 勾选 ----
UiValidator& UiValidator::isChecked() {
    auto s = snapshot(currentId_);
    addResult(s.checked, "isChecked", "true", s.checked ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::notChecked() {
    auto s = snapshot(currentId_);
    addResult(!s.checked, "notChecked", "true", s.checked ? "true" : "false");
    return *this;
}

// ---- 焦点 ----
UiValidator& UiValidator::focused() {
    auto s = snapshot(currentId_);
    addResult(s.focused, "focused", "true", s.focused ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::notFocused() {
    auto s = snapshot(currentId_);
    addResult(!s.focused, "notFocused", "true", s.focused ? "true" : "false");
    return *this;
}

// ---- 位置/尺寸 ----
UiValidator& UiValidator::hasBounds(float x, float y, float w, float h) {
    auto s = snapshot(currentId_);
    std::stringstream ss; ss << s.bounds.x << "," << s.bounds.y << "," << s.bounds.w << "," << s.bounds.h;
    std::stringstream es; es << x << "," << y << "," << w << "," << h;
    bool ok = std::abs(s.bounds.x - x) < 1.0f && std::abs(s.bounds.y - y) < 1.0f
           && std::abs(s.bounds.w - w) < 1.0f && std::abs(s.bounds.h - h) < 1.0f;
    addResult(ok, "hasBounds", es.str(), ss.str());
    return *this;
}

UiValidator& UiValidator::hasWidth(float expected) {
    auto s = snapshot(currentId_);
    addResult(std::abs(s.bounds.w - expected) < 1.0f,
              "hasWidth", std::to_string(expected), std::to_string(s.bounds.w));
    return *this;
}

UiValidator& UiValidator::hasWidthGreaterThan(float minW) {
    auto s = snapshot(currentId_);
    addResult(s.bounds.w > minW, "width>", std::to_string(minW), std::to_string(s.bounds.w));
    return *this;
}

UiValidator& UiValidator::hasWidthLessThan(float maxW) {
    auto s = snapshot(currentId_);
    addResult(s.bounds.w < maxW, "width<", std::to_string(maxW), std::to_string(s.bounds.w));
    return *this;
}

UiValidator& UiValidator::hasHeight(float expected) {
    auto s = snapshot(currentId_);
    addResult(std::abs(s.bounds.h - expected) < 1.0f,
              "hasHeight", std::to_string(expected), std::to_string(s.bounds.h));
    return *this;
}

UiValidator& UiValidator::hasHeightGreaterThan(float minH) {
    auto s = snapshot(currentId_);
    addResult(s.bounds.h > minH, "height>", std::to_string(minH), std::to_string(s.bounds.h));
    return *this;
}

UiValidator& UiValidator::hasHeightLessThan(float maxH) {
    auto s = snapshot(currentId_);
    addResult(s.bounds.h < maxH, "height<", std::to_string(maxH), std::to_string(s.bounds.h));
    return *this;
}

UiValidator& UiValidator::hasX(float expected) {
    auto s = snapshot(currentId_);
    addResult(std::abs(s.bounds.x - expected) < 1.0f,
              "hasX", std::to_string(expected), std::to_string(s.bounds.x));
    return *this;
}

UiValidator& UiValidator::hasY(float expected) {
    auto s = snapshot(currentId_);
    addResult(std::abs(s.bounds.y - expected) < 1.0f,
              "hasY", std::to_string(expected), std::to_string(s.bounds.y));
    return *this;
}

UiValidator& UiValidator::hasYGreaterThan(float minY) {
    auto s = snapshot(currentId_);
    addResult(s.bounds.y > minY, "y>", std::to_string(minY), std::to_string(s.bounds.y));
    return *this;
}

UiValidator& UiValidator::notOverlapping(const std::string& otherId) {
    auto sa = snapshot(currentId_);
    auto sb = snapshot(otherId);
    bool overlap = !(sa.bounds.x + sa.bounds.w <= sb.bounds.x ||
                     sb.bounds.x + sb.bounds.w <= sa.bounds.x ||
                     sa.bounds.y + sa.bounds.h <= sb.bounds.y ||
                     sb.bounds.y + sb.bounds.h <= sa.bounds.y);
    addResult(!overlap, "notOverlapping(" + otherId + ")", "no overlap", overlap ? "overlaps" : "no overlap");
    return *this;
}

UiValidator& UiValidator::containsWidget(const std::string& otherId) {
    auto rp = snapshot(currentId_).bounds;
    auto rc = snapshot(otherId).bounds;
    bool ok = rc.x >= rp.x && rc.y >= rp.y &&
              rc.x + rc.w <= rp.x + rp.w &&
              rc.y + rc.h <= rp.y + rp.h;
    addResult(ok, "contains(" + otherId + ")", "true", ok ? "true" : "false");
    return *this;
}

UiValidator& UiValidator::hasValue(float expected) {
    auto s = snapshot(currentId_);
    addResult(std::abs(s.value - expected) < 0.1f,
              "hasValue", std::to_string(expected), std::to_string(s.value));
    return *this;
}

UiValidator& UiValidator::hasValueInRange(float min, float max) {
    auto s = snapshot(currentId_);
    bool ok = s.value >= min && s.value <= max;
    std::stringstream se; se << "[" << min << "," << max << "]";
    addResult(ok, "valueInRange", se.str(), std::to_string(s.value));
    return *this;
}

UiValidator& UiValidator::hasItemCount(int expected) {
    auto s = snapshot(currentId_);
    addResult(s.itemCount == expected, "itemCount",
              std::to_string(expected), std::to_string(s.itemCount));
    return *this;
}

UiValidator& UiValidator::hasVisibleItemsAtLeast(int minCount) {
    auto s = snapshot(currentId_);
    // 从 itemCount 推断可见项数（简化：假设所有项可见或至少 minCount）
    addResult(s.itemCount >= minCount, "visibleItems>=",
              std::to_string(minCount), std::to_string(s.itemCount));
    return *this;
}

UiValidator& UiValidator::hasSelectedIndex(int expected) {
    auto s = snapshot(currentId_);
    addResult(s.selectedIndex == expected, "selectedIndex",
              std::to_string(expected), std::to_string(s.selectedIndex));
    return *this;
}

UiValidator& UiValidator::hasActiveIndex(int expected) {
    auto s = snapshot(currentId_);
    addResult(s.activeIndex == expected, "activeIndex",
              std::to_string(expected), std::to_string(s.activeIndex));
    return *this;
}

UiValidator& UiValidator::hasType(const std::string& typeName) {
    auto s = snapshot(currentId_);
    addResult(s.typeName == typeName, "type", typeName, s.typeName);
    return *this;
}

// ---- 结果 ----
void UiValidator::addResult(bool pass, const std::string& check,
                             const std::string& expected, const std::string& actual) {
    results_.push_back({pass, currentId_, check, expected, actual});
}

UiValidator::WidgetSnapshot UiValidator::snapshot(const std::string& id) const {
    if (provider_) return provider_(id);
    return {};  // 无 provider 时返回空快照
}

bool UiValidator::allPassed() const {
    for (auto& r : results_) if (!r.pass) return false;
    return true;
}

int UiValidator::failCount() const {
    int n = 0;
    for (auto& r : results_) if (!r.pass) n++;
    return n;
}

std::string UiValidator::report() const {
    std::stringstream ss;
    int passed = 0, failed = 0;
    for (auto& r : results_) { if (r.pass) passed++; else failed++; }
    ss << "=== UI Validation Report ===" << std::endl;
    ss << "Total: " << results_.size() << " | Passed: " << passed << " | Failed: " << failed << std::endl;
    for (auto& r : results_) {
        ss << r.message() << std::endl;
    }
    return ss.str();
}

void UiValidator::reset() {
    results_.clear();
    currentId_.clear();
}

} // namespace jui::test
