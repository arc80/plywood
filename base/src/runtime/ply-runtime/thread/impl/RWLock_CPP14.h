/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class RWLock_CPP14 : protected std::shared_mutex {
public:
    RWLock_CPP14() : std::shared_mutex() {
    }

    void lockExclusive() {
        std::shared_mutex::lock();
    }

    void unlockExclusive() {
        std::shared_mutex::unlock();
    }

    void lockShared() {
        std::shared_mutex::lockShared();
    }

    void unlockShared() {
        std::shared_mutex::unlockShared();
    }
};

} // namespace ply
