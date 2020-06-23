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
#include <ply-build-target/BuildTarget.h>

namespace ply {

s32 command_run(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    BuildParams buildParams;
    buildParams.targetName = env->cl->readToken();
    ensureTerminated(env->cl);
    bool doBuild = true;
    if (env->cl->checkForSkippedOpt("--nobuild")) {
        doBuild = false;
    }
    buildParams.extractOptions(env->cl, env->currentBuildFolder);
    env->cl->finalize();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    BuildParams::Result buildResult;
    if (!buildParams.exec(&buildResult, env->currentBuildFolder, doBuild))
        return -1;

    String exePath =
        getTargetOutputPath(buildResult.runTarget->targetType, buildResult.runTarget->name,
                            env->currentBuildFolder->getAbsPath(), buildParams.configName);
    if (FileSystem::native()->exists(exePath) != ExistsResult::File) {
        fatalError(
            String::format("Executable '{}' is not built in folder '{}'\n",
                           RepoRegistry::get()->getShortDepSourceName(buildResult.runTargetInst),
                           env->currentBuildFolder->buildFolderName));
    }

    // FIXME: Implement --args option (should be last option on command line; rest of command line
    // consists of arguments to pass
    Owned<Subprocess> child = Subprocess::exec(exePath, {}, {}, Subprocess::Output::inherit());
    return child->join();
}

} // namespace ply
