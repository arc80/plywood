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

        auto sw = StdErr::createStringWriter();
        printUsage(&sw, "module",
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

        StringWriter sw = StdOut::createStringWriter();
        Array<const Repo*> repos;
        for (const Repo* repo : RepoRegistry::get()->repos) {
            repos.append(repo);
        }
        sort(repos.view(), [](const Repo* a, const Repo* b) { //
            return a->repoName < b->repoName;
        });
        for (const Repo* repo : repos) {
            sw.format("Modules in repo '{}':\n", repo->repoName);
            Array<TargetInstantiator*> targetInsts;
            for (TargetInstantiator* targetInst : repo->targetInstantiators) {
                targetInsts.append(targetInst);
            }
            sort(targetInsts.view(), [](const TargetInstantiator* a, const TargetInstantiator* b) {
                return a->name < b->name;
            });
            for (TargetInstantiator* targetInst : targetInsts) {
                sw.format("    {}\n", targetInst->name);
            }
            sw << "\n";
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
