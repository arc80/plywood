/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>

namespace ply {

struct Bool2 {
    bool x;
    bool y;

    Bool2(bool x, bool y) : x{x}, y{y} {
    }
};

inline bool all(const Bool2& v) {
    return v.x && v.y;
}

inline bool any(const Bool2& v) {
    return v.x || v.y;
}

struct Bool3 {
    bool x;
    bool y;
    bool z;

    Bool3(bool x, bool y, bool z) : x{x}, y{y}, z{z} {
    }
};

inline bool all(const Bool3& v) {
    return v.x && v.y && v.z;
}

inline bool any(const Bool3& v) {
    return v.x || v.y || v.z;
}

struct Bool4 {
    bool x;
    bool y;
    bool z;
    bool w;

    Bool4(bool x, bool y, bool z, bool w) : x{x}, y{y}, z{z}, w{w} {
    }
};

inline bool all(const Bool4& v) {
    return v.x && v.y && v.z && v.w;
}

inline bool any(const Bool4& v) {
    return v.x || v.y || v.z || v.w;
}

} // namespace ply
