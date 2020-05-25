/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/Atomic.h>

namespace ply {

class TraceGroup;

class Trace_Counters {
private:
    ply::Atomic<TraceGroup*> m_firstGroup;

public:
    PLY_DLL_ENTRY void addGroup(TraceGroup* group);
    PLY_DLL_ENTRY void dumpStats();

    PLY_DLL_ENTRY static Trace_Counters Instance; // Zero-initialized
};

class TraceGroup {
public:
    struct Counter {
        ply::Atomic<u32> count;
        const char* str;
    };

private:
    friend class Trace_Counters;
    const char* m_name;
    Counter* m_counters;
    ureg m_numCounters;
    TraceGroup* m_next;

public:
    TraceGroup(const char* name, Counter* counters, ureg numCounters)
        : m_name(name), m_counters(counters), m_numCounters(numCounters), m_next(NULL) {
        Trace_Counters::Instance.addGroup(this);
    }
    PLY_DLL_ENTRY void dump();
    PLY_DLL_ENTRY void dumpIfUsed();
};

// clang-format off
#define PLY_TRACE_DECLARE(group, count)      extern ply::TraceGroup::Counter Trace_##group[count];
#define PLY_TRACE_DEFINE_BEGIN(group, count) ply::TraceGroup::Counter Trace_##group[count] = {
#define PLY_TRACE_DEFINE(desc)                   { 0, desc },
#define PLY_TRACE_DEFINE_END(group, count)   }; \
                                              ply::TraceGroup TraceGroup_##group(#group, Trace_##group, count);
#define PLY_TRACE(group, index, msg, param1, param2) \
    Trace_##group[index].count.fetchAdd(1, ply::Relaxed)
// clang-format on

} // namespace ply
