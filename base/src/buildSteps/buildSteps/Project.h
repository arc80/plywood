/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <buildSteps/Core.h>
#include <pylon/Node.h>

namespace buildSteps {

struct Node;

enum class Visibility {
    Private,
    Public,
};

struct ToolchainOpt {
    enum class Type {
        IncludeDir,
        PreprocessorDef,
        Generic,
        CompilerSpecific,
        LinkerSpecific,
    };

    Type type = Type::IncludeDir;
    String key;
    String value;

    PLY_INLINE ToolchainOpt(Type type, StringView key, StringView value = {})
        : type{type}, key{key}, value{value} {
    }
    PLY_INLINE bool operator==(const ToolchainOpt& other) const {
        return (this->type == other.type) && (this->key == other.key) &&
               (this->value == other.value);
    }
};

struct ConfigMask {
    u64 bits = 0;
    PLY_INLINE bool hasAllBitsIn(u64 mask) const {
        return (this->bits & mask) == mask;
    }
    PLY_INLINE bool hasAnyBit() const {
        return this->bits;
    }
    PLY_INLINE bool hasBitAtIndex(u32 idx) const {
        return (this->bits & (u64{1} << idx)) != 0;
    }
};

struct Node : RefCounted<Node> {
    enum class Type {
        Executable,
        Lib,
    };

    struct Option {
        ToolchainOpt opt;
        ConfigMask enabled;  // whether option is enabled for each config
        ConfigMask isPublic; // whether option is public/private for each config
    };

    struct Dependency {
        Reference<Node> dep;
        ConfigMask enabled;  // whether dependency is enabled for each config
        ConfigMask isPublic; // whether dependency is public/private for each config
    };

    struct SourceFile {
        String relPath;
        ConfigMask enabled; // whether path is enabled for each config
    };

    struct SourceGroup {
        String absPath;
        Array<SourceFile> files;
    };

    struct LinkerInput {
        String path;
        ConfigMask enabled; // whether linker input is enabled for each config
    };

    String name;
    ConfigMask enabled;      // ie. must be built and/or has a dependent, per-config
    ConfigMask hasBuildStep; // ie. contains .cpp files, per-config
    Type type = Type::Lib;
    Array<Option> options;
    Array<Dependency> dependencies;
    Array<LinkerInput> prebuiltLibs;
    Array<SourceGroup> sourceGroups;

    PLY_INLINE void onRefCountZero() {
        delete this;
    }
};

struct Project {
    String name;
    Array<String> configNames;
    Array<Node::Option> opts;
    Array<Reference<Node>> rootNodes;
};

struct FlatProject;
Owned<FlatProject> flatten(const Project* proj);
void writeCMakeLists(OutStream* outs, const FlatProject* flatProj);
void destroy(FlatProject* flatProj);

} // namespace buildSteps
