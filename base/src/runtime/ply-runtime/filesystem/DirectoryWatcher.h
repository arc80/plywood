/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

#if !defined(PLY_IMPL_DIRECTORYWATCHER_PATH)
#if PLY_TARGET_WIN32
#define PLY_IMPL_DIRECTORYWATCHER_PATH "impl/DirectoryWatcher_Win32.h"
#define PLY_IMPL_DIRECTORYWATCHER_TYPE DirectoryWatcher_Win32
#elif PLY_TARGET_APPLE && !PLY_TARGET_IOS
#define PLY_IMPL_DIRECTORYWATCHER_PATH "impl/DirectoryWatcher_Mac.h"
#define PLY_IMPL_DIRECTORYWATCHER_TYPE DirectoryWatcher_Mac
#elif PLY_KERNEL_LINUX
// FIXME: Implement proper Linux support using inotify or similar
#define PLY_IMPL_DIRECTORYWATCHER_PATH "impl/DirectoryWatcher_Null.h"
#define PLY_IMPL_DIRECTORYWATCHER_TYPE DirectoryWatcher_Null
#else
#define PLY_IMPL_DIRECTORYWATCHER_PATH \
    "*** Unable to select a default DirectoryWatcher implementation ***"
#endif
#endif

#include PLY_IMPL_DIRECTORYWATCHER_PATH

namespace ply {
using DirectoryWatcher = PLY_IMPL_DIRECTORYWATCHER_TYPE;
} // namespace ply
