/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if !PLY_DLL_IMPORTING

#include <ply-runtime/thread/impl/Trace_Counters.h>
#include <ply-runtime/io/StdIO.h>

namespace ply {

Trace_Counters Trace_Counters::Instance; // Zero-initialized

void Trace_Counters::addGroup(TraceGroup* group) {
    TraceGroup* oldHead = m_firstGroup.load(ply::Relaxed);
    do {
        group->m_next = oldHead;
    } while (!m_firstGroup.compareExchangeWeak(oldHead, group, ply::Relaxed, ply::Relaxed));
}

void Trace_Counters::dumpStats() {
    TraceGroup* group = m_firstGroup.load(ply::Relaxed);
    for (; group; group = group->m_next) {
        group->dumpIfUsed();
    }
}

void TraceGroup::dump() {
    StringWriter sw = StdOut::createStringWriter();
    sw.format("--------------- {}\n", m_name);
    for (ureg i = 0; i < m_numCounters; i++) {
        sw.format("{}: {}\n", m_counters[i].count.load(ply::Relaxed), m_counters[i].str);
    }
}

void TraceGroup::dumpIfUsed() {
    for (ureg i = 0; i < m_numCounters; i++) {
        if (m_counters[i].count.load(ply::Relaxed)) {
            dump();
            break;
        }
    }
}

} // namespace ply

#endif // !PLY_DLL_IMPORTING
