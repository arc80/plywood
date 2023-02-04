/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

struct u128 {
    u64 lo = 0;
    u64 hi = 0;

    PLY_INLINE u128() = default;
    PLY_INLINE u128(u64 lo) : lo{lo} {
    }
    PLY_INLINE u128(const u128&) = default;

    PLY_INLINE bool operator==(const u128& other) const {
        return this->hi == other.hi && this->lo == other.lo;
    }
    PLY_INLINE bool operator!=(const u128& other) const {
        return !(this->hi == other.hi && this->lo == other.lo);
    }
    PLY_INLINE void operator+=(const u128& other) {
        this->hi += other.hi;
        this->lo += other.lo;
        this->hi += this->lo < other.lo;
    }
    PLY_INLINE static u128 zero() {
        return {};
    }
};

} // namespace ply
