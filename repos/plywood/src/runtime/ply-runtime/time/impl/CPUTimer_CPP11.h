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
        typedef std::chrono::high_resolution_clock::duration Ticks;
        Ticks ticks;
        operator s64() const {
            return s64(ticks.count());
        }
    };

    struct Point {
        typedef std::chrono::high_resolution_clock::time_point Tick;
        Tick tick;
        Point operator+(Duration d) const {
            return {tick + d.ticks};
        }
        Duration operator-(Point b) const {
            return {tick - b.tick};
        }
        bool operator<(Point b) const {
            return tick < b.tick;
        }
        bool operator<=(Point b) const {
            return tick <= b.tick;
        }
        bool operator>(Point b) const {
            return tick > b.tick;
        }
        bool operator>=(Point b) const {
            return tick >= b.tick;
        }
        bool operator==(Point b) const {
            return tick == b.tick;
        }
    };

    static Point get() {
        return {std::chrono::high_resolution_clock::now()};
    }

    struct Converter {
        Converter() {
        }
        static float toSeconds(Duration duration) {
            return std::chrono::duration_cast<std::chrono::duration<float>>(duration.ticks).count();
        }
    };
};

} // namespace ply
