/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>

namespace ply {

bool command_build(PlyToolCommandEnv* env) {
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    StringView targetName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--target="); });
    if (targetName) {
        targetName = targetName.subStr(9);
    }
    StringView configName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    env->cl->finalize();

    if (!env->currentBuildFolder->isGenerated(configName)) {
        if (!env->currentBuildFolder->generateLoop(configName))
            return false;
    }
    return env->currentBuildFolder->build(configName, targetName, false);
}

} // namespace ply
