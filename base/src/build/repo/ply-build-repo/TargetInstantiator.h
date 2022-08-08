/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/DependencySource.h>

namespace ply {
namespace build {

struct ModuleArgs;

struct TargetInstantiator : DependencySource {
    typedef void ModuleFunc(ModuleArgs* args);

    String instantiatorPath;
    Functor<ModuleFunc> moduleFunc;

    PLY_INLINE TargetInstantiator() : DependencySource{Target} {
    }
    PLY_INLINE TargetInstantiator(StringView name, StringView instantiatorPath,
                                  ModuleFunc* moduleFunc)
        : DependencySource{Target, name}, instantiatorPath{instantiatorPath}, moduleFunc{
                                                                                  moduleFunc} {
    }
};

} // namespace build
} // namespace ply
