/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/Atomic.h>

namespace ply {

// This class is meant to be zero-initialized at global scope.
class Mutex_SpinLock {
private:
    Atomic<u32> m_spinLock;

public:
    void initialize() {
        m_spinLock.storeNonatomic(0);
    }

    void lock() {
        for (;;) {
            u32 expected = 0;
            if (m_spinLock.compareExchangeStrong(expected, 1, ply::Acquire))
                break;
            // FIXME: Implement reusable AdaptiveBackoff class and apply it here
        }
    }

    void unlock() {
        m_spinLock.store(0, ply::Release);
    }
};

} // namespace ply
