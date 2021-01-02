/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>

namespace ply {

struct BoolVector2 {
    bool x;
    bool y;

    PLY_INLINE BoolVector2(bool x, bool y) : x{x}, y{y} {
    }
};

PLY_INLINE bool all(const BoolVector2& v) {
    return v.x && v.y;
}

PLY_INLINE bool any(const BoolVector2& v) {
    return v.x || v.y;
}

struct BoolVector3 {
    bool x;
    bool y;
    bool z;

    PLY_INLINE BoolVector3(bool x, bool y, bool z) : x{x}, y{y}, z{z} {
    }
};

PLY_INLINE bool all(const BoolVector3& v) {
    return v.x && v.y && v.z;
}

PLY_INLINE bool any(const BoolVector3& v) {
    return v.x || v.y || v.z;
}

struct BoolVector4 {
    bool x;
    bool y;
    bool z;
    bool w;

    PLY_INLINE BoolVector4(bool x, bool y, bool z, bool w) : x{x}, y{y}, z{z}, w{w} {
    }
};

PLY_INLINE bool all(const BoolVector4& v) {
    return v.x && v.y && v.z && v.w;
}

PLY_INLINE bool any(const BoolVector4& v) {
    return v.x || v.y || v.z || v.w;
}

} // namespace ply
