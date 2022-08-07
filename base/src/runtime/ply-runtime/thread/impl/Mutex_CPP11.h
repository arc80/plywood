/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <mutex>

namespace ply {

class Mutex_CPP11 : protected std::recursive_mutex {
private:
    friend class LockGuard<Mutex_CPP11>;

public:
    Mutex_CPP11() : std::recursive_mutex() {
    }

    void lock() {
        std::recursive_mutex::lock();
    }

    bool tryLock() {
        return std::recursive_mutex::try_lock();
    }

    void unlock() {
        std::recursive_mutex::unlock();
    }
};

// Specialize LockGuard<Mutex_CPP11> so that ConditionVariable_CPP11 can use it:
template <>
class LockGuard<Mutex_CPP11> : public std::unique_lock<std::recursive_mutex> {
public:
    LockGuard(Mutex_CPP11& mutex) : std::unique_lock<std::recursive_mutex>(mutex) {
    }
};

} // namespace ply
