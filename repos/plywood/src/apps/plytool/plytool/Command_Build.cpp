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
    env->cl->finalize();

    // FIXME: Support --config option
    if (!env->currentBuildFolder->isGenerated({})) {
        if (!env->currentBuildFolder->generateLoop({}))
            return false;
    }
    return env->currentBuildFolder->build({}, targetName, false);
}

} // namespace ply
