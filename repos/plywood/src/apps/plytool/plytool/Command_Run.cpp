/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/ToolchainInfo.h>
#include <ply-build-target/BuildTarget.h>

namespace ply {

s32 command_run(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    env->cl->finalize();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    // FIXME: Factor this out into a common function with command_generate so that the reason for
    // instantiation failure is logged.
    ProjectInstantiationResult instResult = env->currentBuildFolder->instantiateAllTargets(false);
    if (!instResult.isValid) {
        fatalError(String::format("Can't instantiate dependencies for folder '{}'\n",
                                  env->currentBuildFolder->buildFolderName));
        return -1;
    }

    // Choose default run target
    const TargetInstantiator* runTargetInst = nullptr;
    const BuildTarget* runTarget = nullptr;
    for (StringView rootTargetName : env->currentBuildFolder->rootTargets) {
        const TargetInstantiator* rootTargetInst =
            RepoRegistry::get()->findTargetInstantiator(rootTargetName);
        auto rootTargetIdx =
            instResult.dependencyMap.find(rootTargetInst, &instResult.dependencies);
        if (!rootTargetIdx.wasFound())
            continue;

        const BuildTarget* rootTarget =
            static_cast<BuildTarget*>(instResult.dependencies[*rootTargetIdx].dep.get());
        PLY_ASSERT(rootTarget->type == DependencyType::Target);
        if (rootTarget->targetType == BuildTargetType::EXE) {
            runTargetInst = rootTargetInst;
            runTarget = rootTarget;
            break;
        }
    }

    if (!runTarget) {
        fatalError(String::format("Can't find an executable target in folder '{}'\n",
                                  env->currentBuildFolder->buildFolderName));
        return -1;
    }

    String exePath = getTargetOutputPath(runTarget, env->currentBuildFolder->getAbsPath(),
                                         env->currentBuildFolder->cmakeOptions,
                                         env->currentBuildFolder->cmakeOptions.buildType);
    if (FileSystem::native()->exists(exePath) != ExistsResult::File) {
        fatalError(String::format("Executable '{}' is not built in folder '{}'\n",
                                  RepoRegistry::get()->getShortDepSourceName(runTargetInst),
                                  env->currentBuildFolder->buildFolderName));
        return -1;
    }

    // FIXME: Implement --args option (should be last option on command line; rest of command line
    // consists of arguments to pass
    Owned<Subprocess> child = Subprocess::exec(exePath, {}, {}, Subprocess::Output::inherit());
    return child->join();
}

} // namespace ply
