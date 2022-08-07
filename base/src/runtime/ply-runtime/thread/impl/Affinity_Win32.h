/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

class Affinity_Win32 {
private:
    typedef ULONG_PTR AffinityMask;
    static const ureg MaxHWThreads = sizeof(AffinityMask) * 8;

    bool m_isAccurate;
    ureg m_numPhysicalCores;
    ureg m_numHWThreads;
    AffinityMask m_physicalCoreMasks[MaxHWThreads];

public:
    PLY_DLL_ENTRY Affinity_Win32();

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return static_cast<u32>(m_numPhysicalCores);
    }

    u32 getNumHWThreads() const {
        return static_cast<u32>(m_numHWThreads);
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        PLY_ASSERT(core < m_numPhysicalCores);
        return static_cast<u32>(countSetBits(m_physicalCoreMasks[core]));
    }

    PLY_DLL_ENTRY bool setAffinity(ureg core, ureg hwThread);
};

} // namespace ply
