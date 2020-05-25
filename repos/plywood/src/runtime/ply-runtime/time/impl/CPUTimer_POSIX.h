/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <stddef.h>
#include <time.h>

namespace ply {

struct CPUTimer_POSIX {
    struct Duration {
        s64 ticks = 0;
        operator s64() const {
            return ticks;
        }
    };

    struct Point {
        u64 tick = 0;
        Point operator+(Duration d) const {
            return {tick + d.ticks};
        }
        Duration operator-(Point b) const {
            return {s64(tick - b.tick)};
        }
        bool operator<(Point b) const {
            return s64(tick - b.tick) < 0; // Handles wrap-around
        }
        bool operator<=(Point b) const {
            return s64(tick - b.tick) <= 0; // Handles wrap-around
        }
        bool operator>(Point b) const {
            return s64(tick - b.tick) > 0; // Handles wrap-around
        }
        bool operator>=(Point b) const {
            return s64(tick - b.tick) >= 0; // Handles wrap-around
        }
        bool operator==(Point b) const {
            return tick == b.tick;
        }
    };

    static Point get() {
        struct timespec tick;
        clock_gettime(CLOCK_MONOTONIC, &tick);
        return {(u64) tick.tv_sec * 1000000000ull + tick.tv_nsec};
    }

    struct Converter {
        float toSeconds(Duration duration) const {
            return duration * 1e-9f;
        }
        Duration toDuration(float seconds) const {
            return {s64(seconds * 1e9f)};
        }
    };
};

} // namespace ply
