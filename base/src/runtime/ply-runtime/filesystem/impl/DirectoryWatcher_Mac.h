/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>

namespace ply {

class DirectoryWatcher_Mac {
public:
    using Callback = void(StringView path, bool must_recurse);
    String m_root;
    Func<Callback> m_callback;

private:
    Thread m_watcherThread;

    // FIXME: Implement a way to stop the watcher
    void run_watcher();

public:
    DirectoryWatcher_Mac();
    void start(StringView root, Func<Callback>&& callback);
    DirectoryWatcher_Mac(StringView root, Func<Callback>&& callback)
        : DirectoryWatcher_Mac{} {
        start(root, std::move(callback));
    }
};

} // namespace ply
