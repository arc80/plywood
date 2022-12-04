/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repository/Instantiate.h>
#include <ply-build-repository/BuiltIns.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {

void command_new_generate(PlyToolCommandEnv* env) {
    using namespace build;
    BuildFolder* bf = env->currentBuildFolder;
    if (!bf) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    StringView configName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    env->cl->finalize();

    build2::Common::initialize();
    build2::init_built_ins();
    build2::Repository::create();
    build2::instantiate_all_configs(bf);
    String cmakeListsPath =
        NativePath::join(bf->getAbsPath(), "CMakeLists.txt");
    build2::write_CMakeLists_txt_if_different(cmakeListsPath);
    Tuple<s32, String> result =
        generateCMakeProject(bf->getAbsPath(), bf->cmakeOptions, {}, [&](StringView errMsg) {
            ErrorHandler::log(ErrorHandler::Error, errMsg);
        });
    if (result.first != 0) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format("Failed to generate build system for '{}':\n", bf->solutionName) +
                result.second);
    }
}

} // namespace ply
