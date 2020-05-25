/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/Atomic.h>
#include <ply-runtime/thread/Mutex.h>
#include <memory.h>

namespace ply {

// A mutex with no constructor that works when zero-init at global scope.
class Mutex_LazyInit {
private:
    Atomic<bool> m_initFlag;
    Atomic<bool> m_spinLock;
    PLY_DECL_ALIGNED(u8 m_buffer[sizeof(Mutex)],
                     alignof(Mutex)); // Note: alignof requires C++11

    Mutex& getMutex() {
        return *(Mutex*) m_buffer;
    }

    PLY_NO_INLINE void lazyInit() {
        // We use the thread-safe DCLI pattern via spinlock in case threads are spawned
        // during static initialization of global C++ objects. In that case, any of them
        // could call lazyInit().
        while (m_spinLock.compareExchange(false, true, ply::Acquire)) {
            // FIXME: Implement reusable AdaptiveBackoff class and apply it here
        }
        if (!m_initFlag.loadNonatomic()) {
            new (&getMutex()) Mutex;
            m_initFlag.store(true, ply::Release);
        }
        m_spinLock.store(false, ply::Release);
    }

public:
    // Manual initialization is needed if not created at global scope:
    void zeroInit() {
        memset(this, 0, sizeof(*this));
    }

    // There should be no threads racing to lock when the destructor is called.
    // It's valid to attempt to lock after the destructor, though.
    // This permits Mutex_LazyInit to be used at global scope where destructors are called in
    // an arbitrary order.
    ~Mutex_LazyInit() {
        if (m_initFlag.loadNonatomic()) {
            getMutex().Mutex::~Mutex();
            zeroInit();
        }
    }

    void lock() {
        if (!m_initFlag.load(ply::Acquire))
            lazyInit();
        getMutex().lock();
    }

    void unlock() {
        PLY_ASSERT(m_initFlag.loadNonatomic());
        getMutex().unlock();
    }
};

} // namespace ply
