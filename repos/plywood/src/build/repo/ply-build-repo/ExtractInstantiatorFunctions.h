/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct ModuleDefinitionFile {
    struct TargetFunc {
        String targetName;
        String funcName;
        String dynamicLinkPrefix;
    };
    struct ExternProviderFunc {
        String providerName;
        String funcName;
    };

    String absPath;
    Array<TargetFunc> targetFuncs;
    Array<ExternProviderFunc> externProviderFuncs;
};

bool extractInstantiatorFunctions(ModuleDefinitionFile* modDefFile);

} // namespace build
} // namespace ply
