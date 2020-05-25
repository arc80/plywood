/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

enum class DependencyType {
    // ply reflect enum
    Target,
    Extern,
};
PLY_REFLECT_ENUM(, DependencyType)

enum class Visibility {
    Private,
    Public,
};

enum class DynamicLinkage : u8 {
    // ply reflect enum
    None,
    Import,
    Export,
};
PLY_REFLECT_ENUM(, DynamicLinkage)

struct Dependency {
    struct PreprocessorDefinition {
        String key;
        String value;
    };

    DependencyType type = DependencyType::Extern;
    Array<Tuple<Visibility, Dependency*>> dependencies;
    bool initialized = false;
    bool hasBeenPropagated = false;
    Array<Dependency*> visibleDeps;
    Array<String> includeDirs;
    Array<PreprocessorDefinition> defines;
    Array<String> libs;
    Array<String> dlls;
    Array<String> frameworks; // Used in Xcode
    Array<String> abstractFlags;

    PLY_INLINE Dependency(DependencyType type = DependencyType::Extern) : type{type} {
    }
    // Returns false if dep was already a dependency:
    PLY_BUILD_ENTRY bool addDependency(Visibility visibility, Dependency* dep);
};

PLY_NO_INLINE void propagateDependencyProperties(Dependency* dep);

} // namespace build
} // namespace ply
