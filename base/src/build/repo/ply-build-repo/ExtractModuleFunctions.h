/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct ModuleDefinitionFile {
    struct ModuleFunc {
        String moduleName;
        String funcName;
    };
    struct ExternProviderFunc {
        String externName;
        String providerName;
        String funcName;
    };

    String absPath;
    Array<ModuleFunc> moduleFuncs;
    Array<ExternProviderFunc> externProviderFuncs;
};

bool extractInstantiatorFunctions(ModuleDefinitionFile* modDefFile);

} // namespace build
} // namespace ply
