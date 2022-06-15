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
    static constexpr u32 IncludeDir = 0;
    static constexpr u32 PreprocessorDef = 1;
    static constexpr u32 Generic = 2;
    static constexpr u32 CompilerSpecific = 3;
    static constexpr u32 LinkerSpecific = 4;

    static const String Erased;

    Visibility vis = Visibility::Private;
    u32 type : 3;
    String key;
    String value;  // could be Erased

    PLY_INLINE ToolchainOpt(Visibility vis, u32 type, StringView key, StringView value = {})
        : vis{vis}, type{type}, key{key}, value{value} {
    }
};

struct Node : RefCounted<Node> {
    struct Options {
        Array<ToolchainOpt> tcOpts;
        Array<Tuple<Visibility, Reference<Node>>> dependencies;
        Array<String> prebuiltLibs;
    };

    enum Type {
        EXE,
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
    Type type = Lib;
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
