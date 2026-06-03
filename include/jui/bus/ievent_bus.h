#pragma once
#include <string>
#include <functional>
#include <any>

namespace jui::bus {

enum class EventType : int {
    SurfaceCreated, SurfaceDeleted,
    WidgetAdded, WidgetRemoved,
    LayoutCompleted,
    PaintBegin, PaintEnd,
    ActionFired,
    DataModelChanged,
    Resized,
    InputEvent,
    FocusChanged,
    EngineInitialized
};

struct Event {
    EventType type;
    std::string surfaceId;
    std::string widgetId;
    std::any data;
};

class IEventBus {
public:
    virtual ~IEventBus() = default;
    using Handler = std::function<void(const Event&)>;
    virtual void subscribe(EventType type, Handler handler) = 0;
    virtual void publish(const Event& event) = 0;
    virtual void publish(EventType type, const std::string& surfaceId = "",
                         const std::string& widgetId = "", std::any data = {}) = 0;
};

} // namespace jui::bus
