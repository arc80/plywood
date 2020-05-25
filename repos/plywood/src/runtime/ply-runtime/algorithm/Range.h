/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

struct Range {
    u32 i = 0;
    u32 limit = 0;

    struct Iterator {
        Range& range;
        PLY_INLINE u32 operator*() const {
            return range.i;
        }
        PLY_INLINE void operator++() {
            range.i++;
        }
        PLY_INLINE bool operator!=(const Iterator&) const {
            return range.i < range.limit;
        }
    };

    PLY_INLINE Iterator begin() {
        return {*this};
    }
    PLY_INLINE Iterator end() {
        return {*this};
    }
};

PLY_INLINE Range range(u32 limit) {
    return Range{0, limit};
}

PLY_INLINE Range range(u32 start, u32 limit) {
    return Range{start, limit};
}

} // namespace ply
