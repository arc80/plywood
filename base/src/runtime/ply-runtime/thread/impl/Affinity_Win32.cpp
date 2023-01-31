/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime.h>

#if PLY_TARGET_WIN32 && !PLY_DLL_IMPORTING

namespace ply {

Affinity::Affinity() {
    m_isAccurate = false;
    m_numPhysicalCores = 0;
    m_numHWThreads = 0;
    for (ureg i = 0; i < MaxHWThreads; i++)
        m_physicalCoreMasks[i] = 0;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* startProcessorInfo = NULL;
    DWORD length = 0;
    BOOL result = GetLogicalProcessorInformation(NULL, &length);
    if (result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        startProcessorInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) Heap.alloc(length);
        result = GetLogicalProcessorInformation(startProcessorInfo, &length);
        if (result == TRUE) {
            m_isAccurate = true;
            m_numPhysicalCores = 0;
            m_numHWThreads = 0;
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION* endProcessorInfo =
                (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) PLY_PTR_OFFSET(startProcessorInfo, length);
            for (SYSTEM_LOGICAL_PROCESSOR_INFORMATION* processorInfo = startProcessorInfo;
                 processorInfo < endProcessorInfo; processorInfo++) {
                if (processorInfo->Relationship == RelationProcessorCore) {
                    ureg hwt = countSetBits(processorInfo->ProcessorMask);
                    if (hwt == 0)
                        m_isAccurate = false;
                    else if (m_numHWThreads + hwt > MaxHWThreads)
                        m_isAccurate = false;
                    else {
                        PLY_ASSERT(m_numPhysicalCores <= m_numHWThreads &&
                                   m_numHWThreads < MaxHWThreads);
                        m_physicalCoreMasks[m_numPhysicalCores] = processorInfo->ProcessorMask;
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

bool Affinity::setAffinity(ureg core, ureg hwThread) {
    PLY_ASSERT(hwThread < getNumHWThreadsForCore(core));
    AffinityMask availableMask = m_physicalCoreMasks[core];
    for (AffinityMask checkMask = 1;; checkMask <<= 1) {
        if ((availableMask & checkMask) != 0) {
            if (hwThread-- == 0) {
                DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), checkMask);
                return (result != 0);
            }
        }
    }
}

} // namespace ply

#endif // PLY_TARGET_WIN32 && !PLY_DLL_IMPORTING
