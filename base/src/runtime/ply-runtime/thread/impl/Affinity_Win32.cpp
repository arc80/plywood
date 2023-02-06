/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

#if PLY_TARGET_WIN32 && !PLY_DLL_IMPORTING

namespace ply {

Affinity::Affinity() {
    m_isAccurate = false;
    m_numPhysicalCores = 0;
    m_numHWThreads = 0;
    for (ureg i = 0; i < MaxHWThreads; i++)
        m_physicalCoreMasks[i] = 0;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* start_processor_info = NULL;
    DWORD length = 0;
    BOOL result = GetLogicalProcessorInformation(NULL, &length);
    if (result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        start_processor_info =
            (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) Heap.alloc(length);
        result = GetLogicalProcessorInformation(start_processor_info, &length);
        if (result == TRUE) {
            m_isAccurate = true;
            m_numPhysicalCores = 0;
            m_numHWThreads = 0;
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION* end_processor_info =
                (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) PLY_PTR_OFFSET(
                    start_processor_info, length);
            for (SYSTEM_LOGICAL_PROCESSOR_INFORMATION* processor_info =
                     start_processor_info;
                 processor_info < end_processor_info; processor_info++) {
                if (processor_info->Relationship == RelationProcessorCore) {
                    ureg hwt = count_set_bits(processor_info->ProcessorMask);
                    if (hwt == 0)
                        m_isAccurate = false;
                    else if (m_numHWThreads + hwt > MaxHWThreads)
                        m_isAccurate = false;
                    else {
                        PLY_ASSERT(m_numPhysicalCores <= m_numHWThreads &&
                                   m_numHWThreads < MaxHWThreads);
                        m_physicalCoreMasks[m_numPhysicalCores] =
                            processor_info->ProcessorMask;
                        m_numPhysicalCores++;
                        m_numHWThreads += hwt;
                    }
                }
            }
        }
    }

    PLY_ASSERT(m_numPhysicalCores <= m_numHWThreads);
    if (m_numHWThreads == 0) {
        m_isAccurate = false;
        m_numPhysicalCores = 1;
        m_numHWThreads = 1;
        m_physicalCoreMasks[0] = 1;
    }
}

bool Affinity::set_affinity(ureg core, ureg hw_thread) {
    PLY_ASSERT(hw_thread < get_num_hwthreads_for_core(core));
    AffinityMask available_mask = m_physicalCoreMasks[core];
    for (AffinityMask check_mask = 1;; check_mask <<= 1) {
        if ((available_mask & check_mask) != 0) {
            if (hw_thread-- == 0) {
                DWORD_PTR result =
                    SetThreadAffinityMask(GetCurrentThread(), check_mask);
                return (result != 0);
            }
        }
    }
}

} // namespace ply

#endif // PLY_TARGET_WIN32 && !PLY_DLL_IMPORTING
