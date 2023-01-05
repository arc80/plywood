/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/log/Log.h>
#include <ply-runtime/thread/Thread.h>
#include <ply-runtime/container/Func.h>

namespace ply {

class DirectoryWatcher_Win32 {
public:
    using Callback = void(StringView path, bool mustRecurse);

private:
    SLOG_CHANNEL(m_log, "DirectoryWatcher_Win32")

    Thread m_watcherThread;
    String m_root;
    Func<Callback> m_callback;
    HANDLE m_endEvent = INVALID_HANDLE_VALUE;

    void runWatcher();

public:
    PLY_DLL_ENTRY DirectoryWatcher_Win32();
    PLY_DLL_ENTRY void start(StringView root, Func<Callback>&& callback);
    PLY_INLINE DirectoryWatcher_Win32(StringView root, Func<Callback>&& callback)
        : DirectoryWatcher_Win32{} {
        start(root, std::move(callback));
    }
    PLY_DLL_ENTRY ~DirectoryWatcher_Win32();
};

} // namespace ply
