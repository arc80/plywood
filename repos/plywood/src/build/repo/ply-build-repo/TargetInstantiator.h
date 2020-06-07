/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/DependencySource.h>

namespace ply {
namespace build {

struct TargetInstantiatorArgs;

struct TargetInstantiator : DependencySource {
    typedef void InitializeTargetFunc(TargetInstantiatorArgs* args);

    String instantiatorPath;
    Functor<InitializeTargetFunc> targetFunc;

    PLY_INLINE TargetInstantiator() : DependencySource{DependencyType::Target} {
    }
    PLY_INLINE TargetInstantiator(StringView name, StringView instantiatorPath, const Repo* repo,
                                  InitializeTargetFunc* targetFunc)
        : DependencySource{DependencyType::Target, name, repo}, instantiatorPath{instantiatorPath},
          targetFunc{targetFunc} {
    }
};

} // namespace build
} // namespace ply
