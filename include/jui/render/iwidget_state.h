#pragma once
#include <functional>
#include <string>

namespace jui::render {

class IWidgetState {
public:
    virtual ~IWidgetState() = default;
    virtual bool hovered() const = 0;
    virtual void setHovered(bool h) = 0;
    virtual bool enabled() const = 0;
    virtual void setEnabled(bool e) = 0;

    using ChangeCallback = std::function<void()>;
    virtual void setOnChange(ChangeCallback cb) = 0;
};

} // namespace jui::render
