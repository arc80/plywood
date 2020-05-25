/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

#if PLY_WITH_ASSERTS

class RaceDetector {
private:
    ply::Atomic<u8> m_entered = 0;

public:
    void enter() {
        if (m_entered.exchange(1, ply::Acquire) != 0) {
            PLY_FORCE_CRASH();
        }
    }
    void exit() {
        if (m_entered.exchange(0, ply::Acquire) != 1) {
            PLY_FORCE_CRASH();
        }
    }
};

class RaceDetectGuard {
private:
    RaceDetector& m_guard;

public:
    RaceDetectGuard(RaceDetector& guard) : m_guard(guard) {
        m_guard.enter();
    }
    ~RaceDetectGuard() {
        m_guard.exit();
    }
};

#define PLY_DEFINE_RACE_DETECTOR(name) ply::RaceDetector name;
#define PLY_RACE_DETECT_GUARD(name) ply::RaceDetectGuard PLY_UNIQUE_VARIABLE(raceDetectGuard)(name)
#define PLY_RACE_DETECT_ENTER(name) name.enter()
#define PLY_RACE_DETECT_EXIT(name) name.exit()

#else

// clang-format off
#define PLY_DEFINE_RACE_DETECTOR(name)
#define PLY_RACE_DETECT_GUARD(name) do {} while (0)
#define PLY_RACE_DETECT_ENTER(name) do {} while (0)
#define PLY_RACE_DETECT_EXIT(name) do {} while (0)
// clang-format on

#endif

} // namespace ply
