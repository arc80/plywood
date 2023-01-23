/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/filesystem/impl/DirectoryWatcher_Win32.h>
#include <ply-runtime/Path.h>
#include <ply-runtime/string/WString.h>

namespace ply {

PLY_NO_INLINE void DirectoryWatcher_Win32::runWatcher() {
    // FIXME: prepend \\?\ to the path to get past MAX_PATH limitation
    HANDLE hDirectory =
        CreateFileW(win32PathArg(m_root), FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    PLY_ASSERT(hDirectory != INVALID_HANDLE_VALUE);
    HANDLE hChangeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    PLY_ASSERT(hChangeEvent != INVALID_HANDLE_VALUE);
    static const DWORD notifyInfoSize = 65536;
    FILE_NOTIFY_INFORMATION* notifyInfo = (FILE_NOTIFY_INFORMATION*) Heap.alloc(notifyInfoSize);
    for (;;) {
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = hChangeEvent;
        BOOL rc = ReadDirectoryChangesW(hDirectory, notifyInfo, notifyInfoSize, TRUE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                            FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION |
                                            FILE_NOTIFY_CHANGE_LAST_WRITE,
                                        NULL, &overlapped, NULL);
        // FIXME: Handle ERROR_NOTIFY_ENUM_DIR
        HANDLE events[2] = {m_endEvent, hChangeEvent};
        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        PLY_ASSERT(waitResult >= WAIT_OBJECT_0 && waitResult <= WAIT_OBJECT_0 + 1);
        if (waitResult == WAIT_OBJECT_0)
            break;
        FILE_NOTIFY_INFORMATION* r = notifyInfo;
        for (;;) {
            // "The file name is in the Unicode character format and is not null-terminated."
            // https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-_file_notify_information
            String path = fromWString({r->FileName, r->FileNameLength / sizeof(WCHAR)});
            bool isDirectory = false;
            DWORD attribs;
            {
                // FIXME: Avoid some of the UTF-8 <--> UTF-16 conversions done here
                String fullPath = WindowsPath.join(m_root, path);
                attribs = GetFileAttributesW(win32PathArg(fullPath));
            }
            if (attribs == INVALID_FILE_ATTRIBUTES) {
                PLY_ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
            } else {
                isDirectory = (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
            }
            SLOG(m_log, "{} \"{}\" changed", isDirectory ? "Directory" : "File", path);
            if (isDirectory) {
                if (r->Action == FILE_ACTION_REMOVED || r->Action == FILE_ACTION_RENAMED_OLD_NAME ||
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
    Heap.free(notifyInfo);
    CloseHandle(hChangeEvent);
    CloseHandle(hDirectory);
}

PLY_NO_INLINE DirectoryWatcher_Win32::DirectoryWatcher_Win32() {
}

PLY_NO_INLINE void DirectoryWatcher_Win32::start(StringView root, Func<Callback>&& callback) {
    PLY_ASSERT(m_root.isEmpty());
    PLY_ASSERT(!m_callback);
    PLY_ASSERT(m_endEvent = INVALID_HANDLE_VALUE);
    PLY_ASSERT(!m_watcherThread.isValid());
    m_root = root;
    m_callback = std::move(callback);
    m_endEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_watcherThread.run([this]() { runWatcher(); });
}

DirectoryWatcher_Win32::~DirectoryWatcher_Win32() {
    if (m_watcherThread.isValid()) {
        SetEvent(m_endEvent);
        m_watcherThread.join();
        CloseHandle(m_endEvent);
    }
}

} // namespace ply

#endif // PLY_TARGET_WIN32
