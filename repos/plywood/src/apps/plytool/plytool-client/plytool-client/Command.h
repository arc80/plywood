/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <plytool-client/Core.h>

namespace ply {
namespace tool {

struct Command {
    struct Dependency {
        enum class Type {
            // ply reflect enum
            Target,
            External,
        };

        PLY_REFLECT()
        String repoName;
        Type depType = Type::Target;
        String depName;
        // ply reflect off
    };

    struct Type {
        // ply make reflected switch
        struct Run {
            PLY_REFLECT()
            Array<String> sourceFiles;
            Array<Dependency> dependencies;
            // ply reflect off
        };
#include "codegen/switch-ply-tool-Command-Type.inl" //@@ply
    };

    PLY_REFLECT();
    Type type;
    // ply reflect off
};

PLY_DECLARE_TYPE_DESCRIPTOR(Command::Dependency::Type)

} // namespace tool
} // namespace ply
