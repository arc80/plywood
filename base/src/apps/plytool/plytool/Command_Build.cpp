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

namespace ply {

bool command_build(PlyToolCommandEnv* env) {
    using namespace build;

    BuildParams buildParams;
    buildParams.targetName = env->cl->readToken();
    ensureTerminated(env->cl);
    buildParams.extractOptions(env);
    env->cl->finalize();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    BuildParams::Result buildResult;
    return buildParams.exec(&buildResult, env, true);
}

} // namespace ply
