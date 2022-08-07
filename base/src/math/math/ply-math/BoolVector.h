/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>

namespace ply {

struct Bool2 {
    bool x;
    bool y;

    PLY_INLINE Bool2(bool x, bool y) : x{x}, y{y} {
    }
};

PLY_INLINE bool all(const Bool2& v) {
    return v.x && v.y;
}

PLY_INLINE bool any(const Bool2& v) {
    return v.x || v.y;
}

struct Bool3 {
    bool x;
    bool y;
    bool z;

    PLY_INLINE Bool3(bool x, bool y, bool z) : x{x}, y{y}, z{z} {
    }
};

PLY_INLINE bool all(const Bool3& v) {
    return v.x && v.y && v.z;
}

PLY_INLINE bool any(const Bool3& v) {
    return v.x || v.y || v.z;
}

struct Bool4 {
    bool x;
    bool y;
    bool z;
    bool w;

    PLY_INLINE Bool4(bool x, bool y, bool z, bool w) : x{x}, y{y}, z{z}, w{w} {
    }
};

PLY_INLINE bool all(const Bool4& v) {
    return v.x && v.y && v.z && v.w;
}

PLY_INLINE bool any(const Bool4& v) {
    return v.x || v.y || v.z || v.w;
}

} // namespace ply
