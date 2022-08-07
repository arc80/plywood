/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once

namespace ply {

struct CPUTimer_Win32 {
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
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return {u64(now.QuadPart)};
    }

    struct Converter {
        float ticksPerSecond;
        float secondsPerTick;
        Converter() {
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            ticksPerSecond = (float) freq.QuadPart;
            secondsPerTick = 1.0f / ticksPerSecond;
        }
        float toSeconds(Duration duration) const {
            return duration.ticks * secondsPerTick;
        }
        Duration toDuration(float seconds) const {
            return {u64(seconds * ticksPerSecond)};
        }
    };
};

} // namespace ply
