/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/FlatProject.h>
#include <ply-runtime/algorithm/Find.h>

namespace buildSteps {

void inherit(Array<Node::Option>& dstOpts, const Node::Option& srcOpt, u64 enabledBits,
             u64 publicBits) {
    s32 i = find(dstOpts, [&](const Node::Option& o) { return o.opt == srcOpt.opt; });
    if (i < 0) {
        i = dstOpts.numItems();
        dstOpts.append({srcOpt.opt, 0, 0});
    }
    dstOpts[i].enabled.bits |= (srcOpt.enabled.bits & enabledBits);
    dstOpts[i].isPublic.bits |= (srcOpt.isPublic.bits & publicBits);
}

void inherit(Array<FlatNode::Dependency>& dstDeps, FlatNode* srcDep, u64 enabledBits) {
    s32 i = find(dstDeps, [&](const FlatNode::Dependency& o) { return o.dep == srcDep; });
    if (i < 0) {
        i = dstDeps.numItems();
        dstDeps.append({srcDep, 0});
    }
    dstDeps[i].enabled.bits |= enabledBits;
}

void inherit(Array<Node::LinkerInput>& dstInputs, const Node::LinkerInput& srcInput,
             u64 enabledBits) {
    s32 i = find(dstInputs,
                 [&](const Node::LinkerInput& o) { return o.path == srcInput.path; });
    if (i < 0) {
        i = dstInputs.numItems();
        dstInputs.append({srcInput.path, 0});
    }
    dstInputs[i].enabled.bits |= (srcInput.enabled.bits & enabledBits);
}

FlatNode* flatten(FlatProject* flatProj, const Node* node) {
    const Project* proj = flatProj->proj;

    Owned<FlatNode> flatNode;
    {
        auto cursor = flatProj->nodeMap.insertOrFind(node);
        if (cursor.wasFound()) {
            // FIXME: Handle more elegantly
            PLY_ASSERT((*cursor)->initialized); // Circular dependency
            return *cursor;
        }
        flatNode = new FlatNode;
        flatNode->node = node;
        PLY_ASSERT(*cursor == nullptr);
        *cursor = flatNode;
    }

    flatNode->opts = proj->opts;
    for (const Node::Option& opt : node->options) {
        inherit(flatNode->opts, opt, Limits<u64>::Max, Limits<u64>::Max);
    }
    for (const Node::Dependency& dep : node->dependencies) {
        if (dep.dep->type == Node::Type::Executable)
            continue;

        FlatNode* flatDep = flatten(flatProj, dep.dep);
        for (const Node::Option& opt : flatDep->opts) {
            inherit(flatNode->opts, opt, dep.enabled.bits, dep.isPublic.bits);
        }
        inherit(flatNode->dependencies, flatDep, dep.enabled.bits);
        for (const FlatNode::Dependency& dep2 : flatDep->dependencies) {
            inherit(flatNode->dependencies, dep2.dep, dep.enabled.bits & dep2.enabled.bits);
        }
        for (const Node::LinkerInput& prebuiltLib : flatDep->prebuiltLibs) {
            inherit(flatNode->prebuiltLibs, prebuiltLib, dep.enabled.bits);
        }
    }

    flatNode->initialized = true;
    FlatNode* result = flatNode;
    flatProj->allFlatNodes.append(std::move(flatNode));
    return result;
}

Owned<FlatProject> flatten(const Project* proj) {
    PLY_ASSERT(proj->configNames.numItems() <= 64);
    Owned<FlatProject> flatProj = new FlatProject;
    flatProj->proj = proj;

    for (const Node* root : proj->rootNodes) {
        flatten(flatProj, root);
    }

    return flatProj;
}

void destroy(FlatProject* mp) {
    delete mp;
}

} // namespace buildSteps
