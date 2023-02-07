/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/log/Log.h>

namespace ply {

class DirectoryWatcher_Win32 {
public:
    using Callback = void(StringView path, bool must_recurse);

private:
    SLOG_CHANNEL(m_log, "DirectoryWatcher_Win32")

    Thread m_watcherThread;
    String m_root;
    Func<Callback> m_callback;
    HANDLE m_endEvent = INVALID_HANDLE_VALUE;

    void run_watcher();

public:
    DirectoryWatcher_Win32();
    void start(StringView root, Func<Callback>&& callback);
    DirectoryWatcher_Win32(StringView root, Func<Callback>&& callback)
        : DirectoryWatcher_Win32{} {
        start(root, std::move(callback));
    }
    ~DirectoryWatcher_Win32();
};

} // namespace ply
