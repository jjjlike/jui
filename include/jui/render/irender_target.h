#pragma once
#include "jui/core/geometry.h"
#include <string>

namespace jui::render {

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual void beginDraw() = 0;
    virtual void endDraw() = 0;
    virtual void clear(float r, float g, float b) = 0;
    virtual void drawText(const std::string& text, const Rect& rect,
                          float fontSize, const std::string& fontWeight,
                          float r, float g, float b, float a = 1.0f) = 0;
    virtual void drawRect(const Rect& rect, float r, float g, float b,
                          float radius = 0, float a = 1.0f) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2,
                          float r, float g, float b) = 0;
    virtual Size measureText(const std::string& text, float fontSize,
                             const std::string& fontWeight) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual bool isValid() const = 0;
};

} // namespace jui::render
