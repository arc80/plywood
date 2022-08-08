/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/algorithm/Sort.h>
#include <ply-build-repo/BuildInstantiatorDLLs.h>

namespace ply {

void command_module(PlyToolCommandEnv* env) {
    using namespace build;

    StringView cmd = env->cl->readToken();
    if (cmd.isEmpty()) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        auto outs = StdErr::text();
        printUsage(&outs, "module",
                   {
                       {"list", "list description"},
                       {"update", "update description"},
                   });

        return;
    }

    if (prefixMatch(cmd, "list")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());

        OutStream outs = StdOut::text();
        {
            outs << "Available modules:\n";
            Array<TargetInstantiator*> targetInsts;
            for (TargetInstantiator* targetInst : RepoRegistry::get()->repo.targetInstantiators) {
                targetInsts.append(targetInst);
            }
            sort(targetInsts, [](const TargetInstantiator* a, const TargetInstantiator* b) {
                return a->name < b->name;
            });
            for (TargetInstantiator* targetInst : targetInsts) {
                outs.format("    {}\n", targetInst->name);
            }
            outs << "\n";
        }
    } else if (prefixMatch(cmd, "update")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        buildInstantiatorDLLs(true);
    } else {
        fatalError(String::format("Unrecognized module command \"{}\"", cmd));
    }
}

} // namespace ply
