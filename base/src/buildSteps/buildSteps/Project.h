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

struct Node : RefCounted<Node> {
    enum class Type {
        Executable,
        Lib,
    };

    struct Option {
        ToolchainOpt opt;
        u64 activeMask = 0; // whether option is active in each config
        u64 publicMask = 0; // whether option is public/private in each config
    };

    struct Dependency {
        Reference<Node> dep;
        u64 activeMask = 0; // whether dependency is active in each config
        u64 publicMask = 0; // whether dependency is public/private in each config
    };

    struct SourceFilePath {
        String path;
        u64 activeMask = 0; // whether path is active in each config
    };

    struct LinkerInput {
        String nameOrPath;
        u64 activeMask = 0; // whether path is active in each config
    };

    String name;
    u64 configMask = 0; // whether node is active in each config
    Type type = Type::Lib;
    Array<Option> options;
    Array<Dependency> dependencies;
    Array<LinkerInput> prebuiltLibs;
    Array<SourceFilePath> sourceFilePaths;

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
