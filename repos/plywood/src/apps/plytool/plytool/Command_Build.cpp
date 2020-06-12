/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>

namespace ply {

void command_build(PlyToolCommandEnv* env) {
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }
    ensureTerminated(env->cl);
    env->cl->finalize();
    // FIXME: Support --config option
    env->currentBuildFolder->build({}, false);
}

} // namespace ply
