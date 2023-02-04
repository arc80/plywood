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

PLY_NO_INLINE void myCallback(ConstFSEventStreamRef streamRef, void* clientCallBackInfo,
                              size_t numEvents, void* eventPaths,
                              const FSEventStreamEventFlags eventFlags[],
                              const FSEventStreamEventId eventIds[]) {
    DirectoryWatcher_Mac* watcher = (DirectoryWatcher_Mac*) clientCallBackInfo;
    char** paths = (char**) eventPaths;
    for (int i = 0; i < numEvents; i++) {
        /* flags are unsigned long, IDs are uint64_t */
        StringView p = paths[i];
        FSEventStreamEventFlags flags = eventFlags[i];
        PLY_ASSERT(p.startsWith(watcher->m_root));
        p = p.subStr(watcher->m_root.numBytes);

        // puts(String::format("change {} in {}, flags {}/0x{}", eventIds[i],
        // String::convert(p), flags, String::toHex(flags)).bytes());
        bool mustRecurse = false;
        if ((flags & kFSEventStreamEventFlagMustScanSubDirs) != 0) {
            mustRecurse = true;
        }
        if ((flags & kFSEventStreamEventFlagItemIsDir) != 0) {
            mustRecurse = true;
        }
        // FIXME: check kFSEventStreamEventFlagEventIdsWrapped
        watcher->m_callback(p, mustRecurse);
    }
}

PLY_NO_INLINE void DirectoryWatcher_Mac::runWatcher() {
    CFStringRef rootPath = CFStringCreateWithCString(NULL, m_root.bytes, kCFStringEncodingASCII);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void**) &rootPath, 1, NULL);
    FSEventStreamContext context;
    context.version = 0;
    context.info = this;
    context.retain = NULL;
    context.release = NULL;
    context.copyDescription = NULL;
    // FIXME: should use kFSEventStreamCreateFlagWatchRoot to check if the folder being watched
    // gets moved?
    FSEventStreamRef stream =
        FSEventStreamCreate(NULL, myCallback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow,
                            0.15, // latency
                            kFSEventStreamCreateFlagFileEvents);
    CFRelease(pathsToWatch);
    CFRelease(rootPath);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    Boolean rc = FSEventStreamStart(stream);
    PLY_ASSERT(rc == TRUE);
    PLY_UNUSED(rc);

    CFRunLoopRun();

    FSEventStreamStop(stream);
    FSEventStreamUnscheduleFromRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
}

PLY_NO_INLINE DirectoryWatcher_Mac::DirectoryWatcher_Mac() {
}

PLY_NO_INLINE void DirectoryWatcher_Mac::start(StringView root, Func<Callback>&& callback) {
    PLY_ASSERT(m_root.isEmpty());
    PLY_ASSERT(!m_callback);
    PLY_ASSERT(!m_watcherThread.isValid());
    m_root = root;
    m_callback = std::move(callback);
    m_watcherThread.run([this]() { runWatcher(); });
}

} // namespace ply

#endif // PLY_TARGET_APPLE && !PLY_TARGET_IOS
