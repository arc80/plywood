/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <CommandHelpers.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-repo/ProjectInstantiator.h>

namespace ply {

struct DepTreeIndent {
    String node;
    String children;
};

PLY_NO_INLINE void dumpDepTree(OutStream* outs, const build::DependencyTree* depTreeNode,
                               const DepTreeIndent& indent) {
    outs->format("{}{}\n", indent.node, depTreeNode->desc);
    for (u32 i = 0; i < depTreeNode->children.numItems(); i++) {
        DepTreeIndent childIndent;
        if (i + 1 < depTreeNode->children.numItems()) {
            childIndent.node = indent.children + "+-- ";
            childIndent.children = indent.children + "|   ";
        } else {
            childIndent.node = indent.children + "`-- ";
            childIndent.children = indent.children + "    ";
        }
        dumpDepTree(outs, &depTreeNode->children[i], childIndent);
    }
}

void command_target(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

    StringView cmd = env->cl->readToken();
    if (cmd.isEmpty()) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        auto outs = StdErr::text();
        printUsage(&outs, "target",
                   {
                       {"list", "list description"},
                       {"add", "add description"},
                       {"remove", "remove description"},
                       {"graph", "graph description"},
                   });

        return;
    }

    if (prefixMatch(cmd, "list")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        OutStream outs = StdOut::text();
        outs.format("List of root targets in build folder '{}':\n",
                  env->currentBuildFolder->buildFolderName);
        for (StringView targetName : env->currentBuildFolder->rootTargets) {
            const TargetInstantiator* targetInst =
                RepoRegistry::get()->findTargetInstantiator(targetName);
            if (!targetInst) {
                outs.format("    {} (not found)\n", targetName);
            } else {
                String activeText;
                if (targetName == env->currentBuildFolder->activeTarget) {
                    activeText = " (active)";
                }
                outs.format("    {}{}\n", RepoRegistry::get()->getShortDepSourceName(targetInst),
                          activeText);
            }
        }
    } else if (prefixMatch(cmd, "add")) {
        StringView targetName = env->cl->readToken();
        if (targetName.isEmpty()) {
            fatalError("Expected target name");
        }

        ensureTerminated(env->cl);
        AddParams addParams;
        addParams.extractOptions(env->cl);
        env->cl->finalize();

        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            fatalError(String::format("Can't find target '{}'", targetName));
        }
        String fullTargetName = targetInst->getFullyQualifiedName();

        addParams.exec(env->currentBuildFolder, fullTargetName);

        env->currentBuildFolder->save();
        StdOut::text().format("Added root target '{}' to build folder '{}'.\n",
                                            RepoRegistry::get()->getShortDepSourceName(targetInst),
                                            env->currentBuildFolder->buildFolderName);
    } else if (prefixMatch(cmd, "remove")) {
        StringView targetName = env->cl->readToken();
        if (targetName.isEmpty()) {
            fatalError("Expected target name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            exit(1);
        }

        String fullTargetName = targetInst->getFullyQualifiedName();
        s32 j = find(env->currentBuildFolder->rootTargets, fullTargetName);
        if (j < 0) {
            fatalError(String::format("Folder '{}' does not have root target '{}'",
                                      env->currentBuildFolder->buildFolderName,
                                      RepoRegistry::get()->getShortDepSourceName(targetInst)));
        }
        env->currentBuildFolder->rootTargets.erase(j);
        env->currentBuildFolder->save();
        StdOut::text().format("Removed root target '{}' from build folder '{}'.\n",
                                            RepoRegistry::get()->getShortDepSourceName(targetInst),
                                            env->currentBuildFolder->buildFolderName);
    } else if (prefixMatch(cmd, "set")) {
        StringView targetName = env->cl->readToken();
        if (targetName.isEmpty()) {
            fatalError("Expected target name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        // FIXME: Factor this out into a common function with command_generate so that the reason
        // for instantiation failure is logged.
        PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
        PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());
        ProjectInstantiationResult instResult =
            env->currentBuildFolder->instantiateAllTargets(false);
        if (!instResult.isValid) {
            fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                      env->currentBuildFolder->buildFolderName));
        }

        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            exit(1);
        }

        auto targetIdx = instResult.dependencyMap.find(targetInst, &instResult.dependencies);
        if (!targetIdx.wasFound()) {
            fatalError(String::format("Target '{}' is not instantiated in folder '{}'\n",
                                      targetName, env->currentBuildFolder->buildFolderName));
        }

        env->currentBuildFolder->activeTarget = targetInst->getFullyQualifiedName();
        env->currentBuildFolder->save();
        StdOut::text().format("Active target is now '{}' in folder '{}'.\n",
                                            RepoRegistry::get()->getShortDepSourceName(targetInst),
                                            env->currentBuildFolder->buildFolderName);
    } else if (prefixMatch(cmd, "graph")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
        PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());
        DependencyTree depTree = env->currentBuildFolder->buildDepTree();
        OutStream outs = StdOut::text();
        outs.format("Dependency graph for folder '{}':\n", env->currentBuildFolder->buildFolderName);
        DepTreeIndent indent;
        indent.node = "    ";
        indent.children = "    ";
        for (const DependencyTree& treeNode : depTree.children) {
            dumpDepTree(&outs, &treeNode, indent);
        }
    } else {
        fatalError(String::format("Unrecognized target command '{}'", cmd));
    }
}

} // namespace ply
