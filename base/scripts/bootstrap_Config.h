#define PLY_PREFER_CPP11 0
#define PLY_WITH_EXCEPTIONS 0
#define PLY_REPLACE_OPERATOR_NEW 1
#define PLY_USE_DLMALLOC 1
#define PLY_DLMALLOC_DEBUG_CHECKS 0
#define PLY_DLMALLOC_FAST_STATS 0

// Avoid degraded performance caused by Mutex_Win32 (FIXME: Make this the default?):
#define PLY_IMPL_MUTEX_PATH "impl/Mutex_CPP11.h"
#define PLY_IMPL_MUTEX_TYPE ply::Mutex_CPP11
#define PLY_IMPL_CONDITIONVARIABLE_PATH "impl/ConditionVariable_CPP11.h"
#define PLY_IMPL_CONDITIONVARIABLE_TYPE ply::ConditionVariable_CPP11

#define PLY_WORKSPACE_FOLDER "<<<WORKSPACE_FOLDER>>>"
#define PLY_SRC_FOLDER "<<<SRC_FOLDER>>>"
#define PLY_BUILD_FOLDER "<<<BUILD_FOLDER>>>"
#define PLY_CMAKE_PATH "<<<CMAKE_PATH>>>"
#define WITH_AUDIO 1
