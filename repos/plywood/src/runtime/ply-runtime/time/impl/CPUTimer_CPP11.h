/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <chrono>

namespace ply {

struct CPUTimer_CPP11 {
    struct Duration {
        using Ticks = std::chrono::high_resolution_clock::duration;
        Ticks ticks;
        PLY_INLINE operator s64() const {
            return s64(ticks.count());
        }
    };

    struct Point {
        using Tick = std::chrono::high_resolution_clock::time_point;
        Tick tick;
        PLY_INLINE Point(s64 v = 0) : tick{Duration::Ticks{v}} {
        }
        PLY_INLINE Point(const Tick& tick) : tick{tick} {
        }
        PLY_INLINE Point operator+(Duration d) const {
            return {tick + d.ticks};
        }
        PLY_INLINE Duration operator-(Point b) const {
            return {tick - b.tick};
        }
        PLY_INLINE bool operator<(Point b) const {
            return tick < b.tick;
        }
        PLY_INLINE bool operator<=(Point b) const {
            return tick <= b.tick;
        }
        PLY_INLINE bool operator>(Point b) const {
            return tick > b.tick;
        }
        PLY_INLINE bool operator>=(Point b) const {
            return tick >= b.tick;
        }
        PLY_INLINE bool operator==(Point b) const {
            return tick == b.tick;
        }
    };

    PLY_INLINE static Point get() {
        return {std::chrono::high_resolution_clock::now()};
    }

    struct Converter {
        PLY_INLINE Converter() {
        }
        static PLY_INLINE float toSeconds(Duration duration) {
            return std::chrono::duration_cast<std::chrono::duration<float>>(duration.ticks).count();
        }
        Duration toDuration(float seconds) const {
            return {
                std::chrono::duration_cast<Duration::Ticks>(std::chrono::duration<float>{seconds})};
        }
    };
};

} // namespace ply
