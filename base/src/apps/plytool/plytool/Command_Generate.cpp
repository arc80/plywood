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
//#include <ply-build-repository/Repository.h>

namespace ply {

bool command_generate(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    StringView configName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    env->cl->finalize();

    //latest::Repository::create();
    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    return env->currentBuildFolder->generateLoop(configName);
}

} // namespace ply
