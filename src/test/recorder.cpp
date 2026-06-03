/**
 * @file recorder.cpp
 * @brief InteractionRecorder 实现
 */
#include "jui/test/recorder.h"
#include <fstream>
#include <sstream>

namespace jui::test {

// ============================================================
// InteractionStep JSON 序列化
// ============================================================
nlohmann::json InteractionStep::toJson() const {
    nlohmann::json j;
    j["type"] = type;
    if (!target.empty()) j["target"] = target;
    if (!text.empty()) j["text"] = text;
    if (!check.empty()) j["check"] = check;
    if (!expected.empty()) j["expected"] = expected;
    if (value != 0) j["value"] = value;
    if (delayMs != 0) j["delayMs"] = delayMs;
    if (!comment.empty()) j["comment"] = comment;
    return j;
}

InteractionStep InteractionStep::fromJson(const nlohmann::json& j) {
    InteractionStep step;
    step.type = j.value("type", "");
    step.target = j.value("target", "");
    step.text = j.value("text", "");
    step.check = j.value("check", "");
    step.expected = j.value("expected", "");
    step.value = j.value("value", 0.0f);
    step.delayMs = j.value("delayMs", 0);
    step.comment = j.value("comment", "");
    return step;
}

// ============================================================
// InteractionScenario
// ============================================================
std::string InteractionScenario::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& s : steps_) arr.push_back(s.toJson());
    return arr.dump(2);
}

void InteractionScenario::fromJson(const std::string& json) {
    steps_.clear();
    auto arr = nlohmann::json::parse(json);
    if (arr.is_array()) {
        for (auto& j : arr) steps_.push_back(InteractionStep::fromJson(j));
    }
}

void InteractionScenario::saveToFile(const std::string& filepath) {
    std::ofstream f(filepath);
    if (f) f << toJson();
}

void InteractionScenario::loadFromFile(const std::string& filepath) {
    std::ifstream f(filepath);
    if (f) {
        std::stringstream ss; ss << f.rdbuf();
        fromJson(ss.str());
    }
}

// ============================================================
// InteractionRecorder
// ============================================================
void InteractionRecorder::startRecord() {
    recorded_.clear();
    recording_ = true;
}

InteractionScenario InteractionRecorder::stopRecord() {
    recording_ = false;
    return recorded_;
}

void InteractionRecorder::loadScenario(const InteractionScenario& scenario) {
    playback_ = scenario;
    playbackIndex_ = 0;
}

void InteractionRecorder::loadScenarioFromJson(const std::string& json) {
    playback_.fromJson(json);
    playbackIndex_ = 0;
}

bool InteractionRecorder::hasNext() const {
    return playbackIndex_ < playback_.stepCount();
}

InteractionStep InteractionRecorder::nextStep() {
    if (playbackIndex_ < playback_.stepCount()) {
        return playback_.steps()[playbackIndex_++];
    }
    return {};
}

size_t InteractionRecorder::currentStepIndex() const {
    return playbackIndex_;
}

void InteractionRecorder::reset() {
    playbackIndex_ = 0;
}

void InteractionRecorder::recordStep(const InteractionStep& step) {
    if (recording_) recorded_.addStep(step);
}

} // namespace jui::test
