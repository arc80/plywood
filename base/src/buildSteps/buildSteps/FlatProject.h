/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <buildSteps/Core.h>
#include <buildSteps/Project.h>
#include <buildSteps/ToolChain.h>

namespace buildSteps {

struct FlatNode {
    const Node* node = nullptr;
    bool initialized = false;
    Array<Node::Option> opts;
    Array<Node::LinkerInput> dependencies;
    Array<Node::LinkerInput> prebuiltLibs;
};

struct FlatProject {
    struct Traits {
        using Key = const Node*;
        using Item = FlatNode*;
        static PLY_INLINE bool match(const FlatNode* item, const Node* key) {
            return item->node == key;
        }
    };

    ToolChain* tc = nullptr;
    const Project* proj = nullptr;
    HashMap<Traits> nodeMap;
    Array<Owned<FlatNode>> allFlatNodes; // dependencies listed before dependents
};

} // namespace buildSteps
