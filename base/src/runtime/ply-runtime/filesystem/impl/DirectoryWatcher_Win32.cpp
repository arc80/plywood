/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/filesystem/impl/DirectoryWatcher_Win32.h>
#include <ply-runtime/string/WString.h>

namespace ply {

void DirectoryWatcher_Win32::run_watcher() {
    // FIXME: prepend \\?\ to the path to get past MAX_PATH limitation
    HANDLE h_directory = CreateFileW(
        win32_path_arg(m_root), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    PLY_ASSERT(h_directory != INVALID_HANDLE_VALUE);
    HANDLE h_change_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    PLY_ASSERT(h_change_event != INVALID_HANDLE_VALUE);
    static const DWORD notify_info_size = 65536;
    FILE_NOTIFY_INFORMATION* notify_info =
        (FILE_NOTIFY_INFORMATION*) Heap.alloc(notify_info_size);
    for (;;) {
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = h_change_event;
        BOOL rc = ReadDirectoryChangesW(
            h_directory, notify_info, notify_info_size, TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
            NULL, &overlapped, NULL);
        // FIXME: Handle ERROR_NOTIFY_ENUM_DIR
        HANDLE events[2] = {m_endEvent, h_change_event};
        DWORD wait_result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        PLY_ASSERT(wait_result >= WAIT_OBJECT_0 && wait_result <= WAIT_OBJECT_0 + 1);
        if (wait_result == WAIT_OBJECT_0)
            break;
        FILE_NOTIFY_INFORMATION* r = notify_info;
        for (;;) {
            // "The file name is in the Unicode character format and is not
            // null-terminated."
            // https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-_file_notify_information
            String path =
                from_wstring({r->FileName, r->FileNameLength / sizeof(WCHAR)});
            bool is_directory = false;
            DWORD attribs;
            {
                // FIXME: Avoid some of the UTF-8 <--> UTF-16 conversions done here
                String full_path = WindowsPath.join(m_root, path);
                attribs = GetFileAttributesW(win32_path_arg(full_path));
            }
            if (attribs == INVALID_FILE_ATTRIBUTES) {
                PLY_ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
            } else {
                is_directory = (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
            }
            SLOG(m_log, "{} \"{}\" changed", is_directory ? "Directory" : "File", path);
            if (is_directory) {
                if (r->Action == FILE_ACTION_REMOVED ||
                    r->Action == FILE_ACTION_RENAMED_OLD_NAME ||
                    r->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                    m_callback(path, true);
                }
            } else {
                m_callback(path, false);
            }
            if (r->NextEntryOffset == 0)
                break;
            r = (FILE_NOTIFY_INFORMATION*) PLY_PTR_OFFSET(r, r->NextEntryOffset);
        }
    }
    Heap.free(notify_info);
    CloseHandle(h_change_event);
    CloseHandle(h_directory);
}

DirectoryWatcher_Win32::DirectoryWatcher_Win32() {
}

void DirectoryWatcher_Win32::start(StringView root, Func<Callback>&& callback) {
    PLY_ASSERT(m_root.is_empty());
    PLY_ASSERT(!m_callback);
    PLY_ASSERT(m_endEvent = INVALID_HANDLE_VALUE);
    PLY_ASSERT(!m_watcherThread.is_valid());
    m_root = root;
    m_callback = std::move(callback);
    m_endEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_watcherThread.run([this]() { run_watcher(); });
}

DirectoryWatcher_Win32::~DirectoryWatcher_Win32() {
    if (m_watcherThread.is_valid()) {
        SetEvent(m_endEvent);
        m_watcherThread.join();
        CloseHandle(m_endEvent);
    }
}

} // namespace ply

#endif // PLY_TARGET_WIN32
