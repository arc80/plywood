/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-folder/BuildFolder.h>

namespace ply {

void command_bootstrap(PlyToolCommandEnv* env) {
    using namespace build;
    ensureTerminated(env->cl);
    env->cl->finalize();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create());
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());

    const TargetInstantiator* target =
        RepoRegistry::get()->findTargetInstantiator("plywood.plytool");
    if (!target) {
        StdErr::text() << "Error: Can't find 'plytool' module in 'plywood' repo.\n";
        return;
    }

    Owned<BuildFolder> buildFolder = BuildFolder::create({}, "plytool");
    buildFolder->rootTargets.append("plywood.plytool");
    buildFolder->makeShared.append("plywood.plytool");
    ProjectInstantiationResult instResult = buildFolder->instantiateAllTargets(true);
    if (!instResult.isValid) {
        return;
    }
    if (instResult.unselectedExterns.numItems() > 0 ||
        instResult.uninstalledProviders.numItems() > 0) {
        StdErr::text()
            << "Error: Bootstrap file must not have external dependencies.\n";
        return;
    }

    StringWriter sw;
    writeCMakeLists(&sw, buildFolder->solutionName, buildFolder->getAbsPath(), &instResult, true);
    String savePath =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/scripts/bootstrap_CMakeLists.txt");
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        savePath, sw.moveToString(), TextFormat::platformPreference());
    if (result == FSResult::OK) {
        StdOut::text().format("Successfully wrote bootstrap file '{}'.\n", savePath);
    } else if (result == FSResult::Unchanged) {
        StdOut::text().format("Bootstrap file '{}' is already up-to-date.\n",
                                            savePath);
    } else {
        StdErr::text().format("Error writing bootstrap file '{}'.\n", savePath);
    }
}

} // namespace ply
