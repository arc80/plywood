/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/TID.h>
#include <ply-runtime/thread/Mutex.h>
#include <memory>
#include <ply-runtime/thread/Atomic.h>

namespace ply {

//---------------------------------------------------------
// Logs PLY_TRACE events to memory.
// Iterator should only be used after logging is complete.
// Useful for post-mortem debugging and for validating tests.
//---------------------------------------------------------
class Trace_MemLog {
public:
    struct Event {
        ply::TID::TID tid;
        const char* msg;
        uptr param1;
        uptr param2;

        Event() : tid(0), msg(NULL), param1(0), param2(0) {
        }
    };

private:
    static const ureg MaxNumPages = 4;
    static const sreg EventsPerPage = 16384;

    struct Page {
        Page* next;
        ply::Atomic<sreg> index; // This can exceed EVENTS_PER_PAGE, but it's harmless. Just
                                 // means page is full.
        Event events[EventsPerPage];

        Page() : next(NULL), index(0) {
        }
    };

    // Store events in a linked list of pages.
    // Mutex is only locked when it's time to allocate a new page.
    ply::Mutex m_mutex;
    Page* m_head;
    ply::Atomic<Page*> m_tail;
    ureg m_numPages; // Protected by m_mutex

    PLY_DLL_ENTRY Event* allocateEventFromNewPage();

public:
    PLY_DLL_ENTRY Trace_MemLog();
    PLY_DLL_ENTRY ~Trace_MemLog();

    void log(const char* msg, uptr param1, uptr param2) {
        ply::signalFenceSeqCst(); // Compiler barrier
        Page* page = m_tail.load(ply::Consume);
        Event* evt;
        sreg index = page->index.fetchAdd(1, ply::Relaxed);
        if (index < EventsPerPage)
            evt = &page->events[index];
        else
            evt = allocateEventFromNewPage(); // Double-checked locking is performed inside here.
        evt->tid = ply::TID::getCurrentThreadID();
        evt->msg = msg;
        evt->param1 = param1;
        evt->param2 = param2;
        ply::signalFenceSeqCst(); // Compiler barrier
    }

    // Iterators are meant to be used only after all logging is complete.
    friend class Iterator;
    class Iterator {
    private:
        Page* m_page;
        sreg m_index;

    public:
        Iterator(Page* p, sreg i) : m_page(p), m_index(i) {
        }

        Iterator& operator++() {
            m_index++;
            if (m_index >= EventsPerPage) {
                Page* next = m_page->next;
                if (next) {
                    m_page = next;
                    m_index = 0;
                } else {
                    m_index = m_page->index.loadNonatomic();
                }
            }
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return (m_page != other.m_page) || (m_index != other.m_index);
        }

        const Event& operator*() const {
            return m_page->events[m_index];
        }
    };

    Iterator begin() {
        return Iterator(m_head, 0);
    }

    Iterator end() {
        Page* tail = m_tail.load(ply::Relaxed);
        return Iterator(tail, tail->index.loadNonatomic());
    }

    PLY_DLL_ENTRY void dumpStats();
    PLY_DLL_ENTRY void dumpEntireLog(const char* path, ureg startPage = 0);

    PLY_DLL_ENTRY static Trace_MemLog Instance;
};

} // namespace ply

#define PLY_TRACE_DECLARE(group, count)
#define PLY_TRACE_DEFINE_BEGIN(group, count)
#define PLY_TRACE_DEFINE(desc)
#define PLY_TRACE_DEFINE_END(group, count)
#define PLY_TRACE(group, id, msg, param1, param2) \
    ply::Trace_MemLog::Instance.log(msg, param1, param2)
