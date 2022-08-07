/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once

namespace ply {

struct CPUTimer_GCC {
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

    // clang-format off
	static Point get() {
		#if PLY_CPU_X64 || PLY_CPU_X86
			#if PLY_USE_RDTSCP
			    u64 tickl, tickh;
			    asm volatile("rdtscp" : "=a"(tickl), "=d"(tickh) :: "memory", "%rcx");
                return {(tickh << 32) | tickl};
			#else
			    //ply_instructionFence();  // FIXME: Insert a CPUID instruction here for more stable measurements
			    u32 tickl, tickh;
			    asm volatile("rdtsc" : "=a"(tickl), "=d"(tickh) :: "memory");
                return {(u64(tickh) << 32) | tickl)};
			#endif
		#elif PLY_CPU_ARM
			#error "Not implemented yet!"
		#else
			#error "Unsupported platform!"	    
		#endif
	}
    // clang-format on

    struct Converter {
        float ticksPerSecond;
        float secondsPerTick;
        Converter();
        float toSeconds(Duration duration) const {
            return duration.ticks * secondsPerTick;
        }
        Duration toDuration(float seconds) const {
            return {s64(seconds * ticksPerSecond)};
        }
    };
};

} // namespace ply
