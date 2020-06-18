/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>

namespace ply {

bool command_generate(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    env->cl->finalize();

    // FIXME: Support --config option
    return env->currentBuildFolder->generateLoop({});
}

} // namespace ply
