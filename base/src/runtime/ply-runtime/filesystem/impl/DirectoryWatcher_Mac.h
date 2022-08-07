/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/thread/Thread.h>
#include <ply-runtime/container/Functor.h>

namespace ply {

class DirectoryWatcher_Mac {
public:
    using Callback = void(StringView path, bool mustRecurse);
    String m_root;
    Functor<Callback> m_callback;

private:
    Thread m_watcherThread;

    // FIXME: Implement a way to stop the watcher
    void runWatcher();

public:
    PLY_DLL_ENTRY DirectoryWatcher_Mac();
    PLY_DLL_ENTRY void start(StringView root, Functor<Callback>&& callback);
    PLY_INLINE DirectoryWatcher_Mac(StringView root, Functor<Callback>&& callback)
        : DirectoryWatcher_Mac{} {
        start(root, std::move(callback));
    }
};

} // namespace ply
