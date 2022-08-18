/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/FlatProject.h>
#include <ply-runtime/algorithm/Find.h>

namespace buildSteps {

void inherit(Array<Node::Option>& dstOpts, const Node::Option& srcOpt, u64 activeMask,
             u64 publicMask) {
    s32 i = find(dstOpts, [&](const Node::Option& o) { return o.opt == srcOpt.opt; });
    if (i < 0) {
        i = dstOpts.numItems();
        dstOpts.append({srcOpt.opt, 0, 0});
    }
    dstOpts[i].activeMask |= (srcOpt.activeMask & activeMask);
    dstOpts[i].publicMask |= (srcOpt.publicMask & publicMask);
}

void inherit(Array<Node::LinkerInput>& dstInputs, const Node::LinkerInput& srcInput, u64 activeMask) {
    s32 i = find(dstInputs,
                 [&](const Node::LinkerInput& o) { return o.nameOrPath == srcInput.nameOrPath; });
    if (i < 0) {
        i = dstInputs.numItems();
        dstInputs.append({srcInput.nameOrPath, 0});
    }
    dstInputs[i].activeMask |= (srcInput.activeMask & activeMask);
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
        FlatNode* flatDep = flatten(flatProj, dep.dep);
        for (const Node::Option& opt : flatDep->opts) {
            inherit(flatNode->opts, opt, dep.activeMask, dep.publicMask);
        }
        inherit(flatNode->dependencies, {dep.dep->name, Limits<u64>::Max}, dep.activeMask);
        for (const Node::LinkerInput& dep2 : flatDep->dependencies) {
            inherit(flatNode->dependencies, dep2, dep.activeMask);
        }
        for (const Node::LinkerInput& prebuiltLib : flatDep->prebuiltLibs) {
            inherit(flatNode->prebuiltLibs, prebuiltLib, dep.activeMask);
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
#if PLY_TARGET_WIN32
    flatProj->tc = getMSVC();
#else
    flatProj->tc = getGCC();
#endif
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
