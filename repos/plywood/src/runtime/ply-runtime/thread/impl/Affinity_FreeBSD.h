/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <pthread.h>
#include <sched.h>
#include <vector>

namespace ply {

class Affinity_FreeBSD {
private:
    struct CoreInfo {
        std::vector<u32> hwThreadIndexToLogicalProcessor;
    };
    bool m_isAccurate;
    std::vector<CoreInfo> m_coreIndexToInfo;
    u32 m_numHWThreads;

public:
    Affinity_FreeBSD();

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return m_coreIndexToInfo.size();
    }

    u32 getNumHWThreads() const {
        return m_numHWThreads;
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        return m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor.size();
    }

    bool setAffinity(ureg core, ureg hwThread);
};

} // namespace ply
