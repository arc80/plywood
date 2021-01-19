/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

// Used as the return value of ThreadLocal::setInScope()
template <template <typename> typename TL, typename T>
class ThreadLocalScope {
private:
    TL<T>* var;
    T oldValue;

public:
    PLY_INLINE ThreadLocalScope(TL<T>* var, T newValue) : var{var} {
        this->oldValue = var->load();
        var->store(newValue);
    }

    ThreadLocalScope(const ThreadLocalScope&) = delete;
    PLY_INLINE ThreadLocalScope(ThreadLocalScope&& other) {
        this->var = other->var;
        this->oldValue = std::move(other.oldValue);
        other->var = nullptr;
    }

    ~ThreadLocalScope() {
        if (this->var) {
            this->var->store(this->oldValue);
        }
    }
};

} // namespace ply

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_THREADLOCAL_PATH)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_THREADLOCAL_PATH "impl/ThreadLocal_Win32.h"
        #define PLY_IMPL_THREADLOCAL_TYPE ply::ThreadLocal_Win32
        #define PLY_IMPL_THREADLOCALSCOPE_TYPE ply::ThreadLocalScope_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_THREADLOCAL_PATH "impl/ThreadLocal_POSIX.h"
        #define PLY_IMPL_THREADLOCAL_TYPE ply::ThreadLocal_POSIX
        #define PLY_IMPL_THREADLOCALSCOPE_TYPE ply::ThreadLocalScope_POSIX
    #else
        #define PLY_IMPL_THREADLOCAL_PATH "*** Unable to select a default ThreadLocal implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_THREADLOCAL_PATH

// Alias it:
namespace ply {

template <typename T>
using ThreadLocal = PLY_IMPL_THREADLOCAL_TYPE<T>;

} // namespace ply
