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
        LinkerSpecific
    };

    static const String Erased;

    Visibility vis = Visibility::Private;
    Type type = Type::IncludeDir;
    String key;
    String value;  // could be Erased

    PLY_INLINE ToolchainOpt(Visibility vis, Type type, StringView key, StringView value = {})
        : vis{vis}, type{type}, key{key}, value{value} {
    }
};

struct Node : RefCounted<Node> {
    struct Options {
        Array<ToolchainOpt> tcOpts;
        Array<Tuple<Visibility, Reference<Node>>> dependencies;
        Array<String> prebuiltLibs;
    };

    enum class Type {
        Executable,
        Lib,
    };

    struct SourceFiles {
        String root;
        Array<String> relFiles;
    };

    struct Config {
        String name;
        Options opts;
    };

    String name;
    Type type = Type::Lib;
    Options opts;
    Array<SourceFiles> sourceFiles;
    Array<Config> configs;

    PLY_INLINE void onRefCountZero() {
        delete this;
    }
};

struct Project {
    String name;
    Owned<pylon::Node> toolChain;
    Node::Options opts;
    Array<Node::Config> configs;
    Array<Reference<Node>> rootNodes;
};

struct MetaProject;
Owned<MetaProject> expand(const Project* project);
void writeCMakeLists(OutStream* outs, const MetaProject* metaProj);
void destroy(MetaProject* mp);

} // namespace buildSteps
