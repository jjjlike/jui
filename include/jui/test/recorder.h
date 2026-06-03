/**
 * @file recorder.h
 * @brief 交互录制/回放 — 记录用户操作序列，支持JSON序列化与AI生成场景
 * 
 * 场景JSON格式示例：
 *   [
 *     {"type":"click","target":"btn_submit","comment":"点击提交按钮"},
 *     {"type":"keyinput","target":"tf1","text":"admin","comment":"输入用户名"},
 *     {"type":"assert","target":"txt_result","check":"hasText","expected":"成功"}
 *   ]
 * 
 * AI可直接根据JSON中的 "comment" 字段理解每步操作意图。
 */
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <json.hpp>

namespace jui::test {

// ============================================================
// 单步操作记录
// ============================================================
struct InteractionStep {
    std::string type;          // 操作类型：click, keyinput, scroll, drag, assert, wait
    std::string target;        // 目标控件ID
    std::string text;          // 输入文本（keyinput时使用）
    std::string check;         // 断言检查项（assert时使用）
    std::string expected;      // 断言期望值
    float value = 0;           // 数值（scroll/drag时使用）
    int delayMs = 0;           // 延迟（ms）
    std::string comment;       // AI可读的操作意图说明

    nlohmann::json toJson() const;
    static InteractionStep fromJson(const nlohmann::json& j);
};

// ============================================================
// 交互场景
// ============================================================
class InteractionScenario {
public:
    void addStep(const InteractionStep& step) { steps_.push_back(step); }

    const std::vector<InteractionStep>& steps() const { return steps_; }
    size_t stepCount() const { return steps_.size(); }

    // JSON序列化
    std::string toJson() const;
    void fromJson(const std::string& json);

    // 保存/加载
    void saveToFile(const std::string& filepath);
    void loadFromFile(const std::string& filepath);

    // 清空
    void clear() { steps_.clear(); }

private:
    std::vector<InteractionStep> steps_;
};

// ============================================================
// InteractionRecorder — 录制/回放引擎
// ============================================================
class InteractionRecorder {
public:
    // 开始录制（模式1：实时录制）
    void startRecord();
    // 停止录制并返回场景
    InteractionScenario stopRecord();

    // 加载场景并准备回放
    void loadScenario(const InteractionScenario& scenario);
    // 加载场景JSON
    void loadScenarioFromJson(const std::string& json);

    // 回放控制
    bool hasNext() const;                    // 是否还有未播放步骤
    InteractionStep nextStep();              // 获取下一步操作
    size_t currentStepIndex() const;         // 当前步骤索引
    void reset();                            // 复位到第一步

    // 录制过程中添加步骤
    void recordStep(const InteractionStep& step);

    // 检查是否在录制中
    bool isRecording() const { return recording_; }

private:
    bool recording_ = false;
    InteractionScenario recorded_;
    InteractionScenario playback_;
    size_t playbackIndex_ = 0;
};

} // namespace jui::test
