/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/Dependency.h>

namespace ply {
namespace build {

struct Repo;

// Repos contain DependencySources.
// A DependencySource can be a TargetInstantiator or an extern name.
// (Extern names still need to be mapped to providers.)
struct DependencySource {
    enum Type {
        Extern,
        Target,
    };

    Type type = Extern;
    String name; // unqualified

    DependencySource(Type type) : type{type} {
    }
    PLY_INLINE DependencySource(Type type, StringView name)
        : type{type}, name{name} {
    }
};

} // namespace build
} // namespace ply
