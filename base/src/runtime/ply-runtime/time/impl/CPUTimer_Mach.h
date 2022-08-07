/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <mach/mach_time.h>

namespace ply {

struct CPUTimer_Mach {
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
        return {mach_absolute_time()};
    }

    struct Converter {
        float ticksPerSecond;
        float secondsPerTick;
        Converter() {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            ticksPerSecond = 1e9f * info.denom / info.numer;
            secondsPerTick = 1.0f / ticksPerSecond;
        }
        float toSeconds(Duration duration) const {
            return duration.ticks * secondsPerTick;
        }
        Duration toDuration(float seconds) const {
            return {s64(seconds * ticksPerSecond)};
        }
    };
};

} // namespace ply
