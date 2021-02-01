/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if !PLY_DLL_IMPORTING

#include <memory>
#include <ply-runtime/thread/impl/Trace_MemLog.h>
#include <stdio.h>
#include <ply-platform/Util.h>
#include <ply-runtime/io/StdIO.h>
#include <ply-runtime/filesystem/FileSystem.h>

namespace ply {

Trace_MemLog Trace_MemLog::Instance;

Trace_MemLog::Trace_MemLog() : m_head(new Page), m_tail(m_head), m_numPages(1) {
}

Trace_MemLog::~Trace_MemLog() {
    // Pages are not cleaned up
}

Trace_MemLog::Event* Trace_MemLog::allocateEventFromNewPage() {
    ply::LockGuard<ply::Mutex> lock(m_mutex);
    // Double-checked locking:
    // Check again whether the current page is full. Another thread may have called
    // allocateEventFromNewPage and created a new page by the time we get take the lock.
    Page* oldTail = m_tail.loadNonatomic();
    if (oldTail->index.load(ply::Relaxed) < EventsPerPage) {
        sreg index = oldTail->index.fetchAdd(1, ply::Relaxed);
        // Yes! We got a slot on this page.
        if (index < EventsPerPage)
            return &oldTail->events[index];
    }

    // OK, we're definitely out of space. It's up to us to allocate a new page.
    Page* newTail = new Page;
    // Reserve the first slot.
    newTail->index.storeNonatomic(1);
    // A plain non-atomic move to oldTail->next is fine because there are no other writers
    // here, and nobody is supposed to read the logged contents until all logging is complete.
    oldTail->next = newTail;
    // m_tail must be written atomically because it is read concurrently from other threads.
    // We also use release/consume semantics so that its constructed contents are visible to
    // other threads. Again, very much like the double-checked locking pattern.
    m_tail.store(newTail, ply::Release);
    if (m_numPages >= MaxNumPages) {
        Page* oldHead = m_head;
        m_head = oldHead->next;
        delete oldHead;
    } else {
        m_numPages++;
    }

    // Return the reserved slot.
    return &newTail->events[0];
}

void Trace_MemLog::dumpStats() {
    ureg numEvents = 0;
    {
        ply::LockGuard<ply::Mutex> lock(m_mutex);
        numEvents =
            (m_numPages - 1) * EventsPerPage + m_tail.load(ply::Consume)->index.load(ply::Relaxed);
    }
    StdOut::text().format("{} events logged\n", numEvents);
}

void Trace_MemLog::dumpEntireLog(const char* path, ureg startPage) {
    ply::LockGuard<ply::Mutex> lock(m_mutex);
    Owned<OutStream> outs =
        FileSystem::native()->openTextForWrite(path, TextFormat::platformPreference());
    if (!outs)
        return;
    for (Page* page = m_head; page; page = page->next) {
        if (startPage > 0) {
            startPage--;
            continue;
        }
        ureg limit = ply::min<ureg>(EventsPerPage, page->index.load(ply::Relaxed));
        for (ureg i = 0; i < limit; i++) {
            const Event& evt = page->events[i];
            outs->format("[{}] {} {} {}\n", (u64) evt.tid, evt.param1, evt.param2, evt.msg);
        }
    }
}

} // namespace ply

#endif // !PLY_DLL_IMPORTING
