#pragma once
#include <string>

namespace jui::input {

class IInputDispatcher {
public:
    virtual ~IInputDispatcher() = default;
    virtual void dispatchMouseDown(int x, int y, int button) = 0;
    virtual void dispatchMouseUp(int x, int y, int button) = 0;
    virtual void dispatchMouseMove(int x, int y) = 0;
    virtual void dispatchMouseWheel(float delta) = 0;
    virtual void dispatchKeyDown(int vk) = 0;
    virtual void dispatchKeyUp(int vk) = 0;
    virtual void dispatchChar(uint32_t ch) = 0;
};

} // namespace jui::input
