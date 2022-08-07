/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

class Affinity_Null {
public:
    bool isAccurate() const {
        return false;
    }

    u32 getNumPhysicalCores() const {
        return 1;
    }

    u32 getNumHWThreads() const {
        return 1;
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        PLY_UNUSED(core);
        return 1;
    }

    bool setAffinity(ureg core, ureg hwThread) {
        PLY_UNUSED(core);
        PLY_UNUSED(hwThread);
        return true;
    }
};

} // namespace ply
