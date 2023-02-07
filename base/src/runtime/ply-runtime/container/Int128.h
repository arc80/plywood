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

    u128() = default;
    u128(u64 lo) : lo{lo} {
    }
    u128(const u128&) = default;

    bool operator==(const u128& other) const {
        return this->hi == other.hi && this->lo == other.lo;
    }
    bool operator!=(const u128& other) const {
        return !(this->hi == other.hi && this->lo == other.lo);
    }
    void operator+=(const u128& other) {
        this->hi += other.hi;
        this->lo += other.lo;
        this->hi += this->lo < other.lo;
    }
    static u128 zero() {
        return {};
    }
};

} // namespace ply
