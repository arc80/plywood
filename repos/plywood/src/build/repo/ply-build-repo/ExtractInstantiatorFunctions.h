/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct InstantiatorInlFile {
    struct TargetFunc {
        String targetName;
        String funcName;
        String dynamicLinkPrefix;
    };
    struct ExternFunc {
        String providerName;
        String externFunc;
    };

    String absPath;
    Array<TargetFunc> targetFuncs;
    Array<ExternFunc> externFuncs;
};

bool extractInstantiatorFunctions(InstantiatorInlFile* inlFile);

} // namespace build
} // namespace ply
