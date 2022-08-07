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
    const Repo* repo = nullptr;

    DependencySource(Type type) : type{type} {
    }
    PLY_INLINE DependencySource(Type type, StringView name, const Repo* repo)
        : type{type}, name{name}, repo{repo} {
    }
    PLY_NO_INLINE String getFullyQualifiedName() const;
};

// Logs an error and returns empty if nameWithDots is invalid
Array<StringView> splitName(StringView nameWithDots, StringView typeForErrorMsg);

} // namespace build
} // namespace ply
