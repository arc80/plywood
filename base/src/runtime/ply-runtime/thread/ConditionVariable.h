/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/Mutex.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_CONDITIONVARIABLE_PATH)
    #if PLY_PREFER_CPP11
        #define PLY_IMPL_CONDITIONVARIABLE_PATH "impl/ConditionVariable_CPP11.h"
        #define PLY_IMPL_CONDITIONVARIABLE_TYPE ply::ConditionVariable_CPP11
    #elif PLY_TARGET_WIN32
        #define PLY_IMPL_CONDITIONVARIABLE_PATH "impl/ConditionVariable_Win32.h"
        #define PLY_IMPL_CONDITIONVARIABLE_TYPE ply::ConditionVariable_Win32
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_CONDITIONVARIABLE_PATH "impl/ConditionVariable_POSIX.h"
        #define PLY_IMPL_CONDITIONVARIABLE_TYPE ply::ConditionVariable_POSIX
    #else
        #define PLY_IMPL_CONDITIONVARIABLE_PATH "*** Unable to select a default ConditionVariable implementation ***"
    #endif
#endif

// Include the implementation:
#include PLY_IMPL_CONDITIONVARIABLE_PATH

// Alias it:
namespace ply {
typedef PLY_IMPL_CONDITIONVARIABLE_TYPE ConditionVariable;
}
