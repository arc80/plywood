/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_APPLE && !PLY_TARGET_IOS

#include <ply-runtime/filesystem/impl/DirectoryWatcher_Mac.h>
#include <CoreServices/CoreServices.h>

namespace ply {

PLY_NO_INLINE void my_callback(ConstFSEventStreamRef stream_ref,
                               void* client_call_back_info, size_t num_events,
                               void* event_paths,
                               const FSEventStreamEventFlags event_flags[],
                               const FSEventStreamEventId event_ids[]) {
    DirectoryWatcher_Mac* watcher = (DirectoryWatcher_Mac*) client_call_back_info;
    char** paths = (char**) event_paths;
    for (int i = 0; i < num_events; i++) {
        /* flags are unsigned long, IDs are uint64_t */
        StringView p = paths[i];
        FSEventStreamEventFlags flags = event_flags[i];
        PLY_ASSERT(p.starts_with(watcher->m_root));
        p = p.sub_str(watcher->m_root.num_bytes);

        // puts(String::format("change {} in {}, flags {}/0x{}", event_ids[i],
        // String::convert(p), flags, String::to_hex(flags)).bytes());
        bool must_recurse = false;
        if ((flags & kFSEventStreamEventFlagMustScanSubDirs) != 0) {
            must_recurse = true;
        }
        if ((flags & kFSEventStreamEventFlagItemIsDir) != 0) {
            must_recurse = true;
        }
        // FIXME: check kFSEventStreamEventFlagEventIdsWrapped
        watcher->m_callback(p, must_recurse);
    }
}

PLY_NO_INLINE void DirectoryWatcher_Mac::run_watcher() {
    CFStringRef root_path =
        CFStringCreateWithCString(NULL, m_root.bytes, kCFStringEncodingASCII);
    CFArrayRef paths_to_watch = CFArrayCreate(NULL, (const void**) &root_path, 1, NULL);
    FSEventStreamContext context;
    context.version = 0;
    context.info = this;
    context.retain = NULL;
    context.release = NULL;
    context.copy_description = NULL;
    // FIXME: should use kFSEventStreamCreateFlagWatchRoot to check if the folder being
    // watched gets moved?
    FSEventStreamRef stream = FSEventStreamCreate(
        NULL, my_callback, &context, paths_to_watch, kFSEventStreamEventIdSinceNow,
        0.15, // latency
        kFSEventStreamCreateFlagFileEvents);
    CFRelease(paths_to_watch);
    CFRelease(root_path);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(),
                                     kCFRunLoopDefaultMode);
    Boolean rc = FSEventStreamStart(stream);
    PLY_ASSERT(rc == TRUE);
    PLY_UNUSED(rc);

    CFRunLoopRun();

    FSEventStreamStop(stream);
    FSEventStreamUnscheduleFromRunLoop(stream, CFRunLoopGetCurrent(),
                                       kCFRunLoopDefaultMode);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
}

PLY_NO_INLINE DirectoryWatcher_Mac::DirectoryWatcher_Mac() {
}

PLY_NO_INLINE void DirectoryWatcher_Mac::start(StringView root,
                                               Func<Callback>&& callback) {
    PLY_ASSERT(m_root.is_empty());
    PLY_ASSERT(!m_callback);
    PLY_ASSERT(!m_watcherThread.is_valid());
    m_root = root;
    m_callback = std::move(callback);
    m_watcherThread.run([this]() { run_watcher(); });
}

} // namespace ply

#endif // PLY_TARGET_APPLE && !PLY_TARGET_IOS
