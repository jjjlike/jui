#pragma once
#include "ievent_bus.h"
#include <vector>
#include <map>
#include <mutex>

namespace jui::bus {

class EventBus : public IEventBus {
public:
    void subscribe(EventType type, Handler handler) override {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[type].push_back(std::move(handler));
    }

    void publish(const Event& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(event.type);
        if (it == handlers_.end()) return;
        for (auto& h : it->second) h(event);
    }

    void publish(EventType type, const std::string& surfaceId,
                 const std::string& widgetId, std::any data) override {
        Event ev{type, surfaceId, widgetId, std::move(data)};
        publish(ev);
    }

    void clear(EventType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.erase(type);
    }

    void clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.clear();
    }

    size_t handlerCount(EventType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(type);
        return (it != handlers_.end()) ? it->second.size() : 0;
    }

private:
    mutable std::mutex mutex_;
    std::map<EventType, std::vector<Handler>> handlers_;
};

} // namespace jui::bus
