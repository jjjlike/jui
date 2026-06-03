#pragma once
namespace jui {

struct Rect { float x = 0, y = 0, w = 0, h = 0; };
struct Size { float w = 0, h = 0; };

inline bool operator==(const Rect& a, const Rect& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}
inline bool operator!=(const Rect& a, const Rect& b) { return !(a == b); }

} // namespace jui
