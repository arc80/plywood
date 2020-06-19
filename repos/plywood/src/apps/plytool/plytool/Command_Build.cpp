/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>

namespace ply {

bool command_build(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    StringView targetName = env->cl->readToken();
    ensureTerminated(env->cl);
    StringView configName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    env->cl->finalize();

    if (!env->currentBuildFolder->isGenerated(configName)) {
        PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
        PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
        PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

        if (!env->currentBuildFolder->generateLoop(configName))
            return false;
    }
    return env->currentBuildFolder->build(configName, targetName, false);
}

} // namespace ply
