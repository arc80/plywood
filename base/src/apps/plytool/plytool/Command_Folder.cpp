/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {

void command_folder(PlyToolCommandEnv* env) {
    using namespace build;
    StringView cmd = env->cl->readToken();
    if (cmd.isEmpty()) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        auto outs = StdErr::text();
        printUsage(&outs, "folder",
                   {
                       {"list", "list description"},
                       {"create", "create description"},
                       {"delete", "delete description"},
                       {"set", "set description"},
                   });

        return;
    }

    if (prefixMatch(cmd, "list")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        OutStream outs = StdOut::text();
        outs << "Build folders found:\n";
        for (const BuildFolder* bf : env->buildFolders) {
            PLY_ASSERT(!bf->buildFolderName.isEmpty());
            bool isActive = (bf->buildFolderName == env->workspace->currentBuildFolder);
            outs.format("    {}{}\n", bf->buildFolderName, (isActive ? " (active)" : ""));
        }
    } else if (prefixMatch(cmd, "create")) {
        StringView name = env->cl->readToken();
        if (name.isEmpty()) {
            fatalError("Expected folder name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        if (find(env->buildFolders,
                 [&](const BuildFolder* bf) { return bf->buildFolderName == name; }) >= 0) {
            fatalError(String::format("Folder \"{}\" already exists", name));
        }

        Owned<BuildFolder> info = BuildFolder::create(name, name);
        info->cmakeOptions = NativeToolchain;
        if (env->workspace->defaultCMakeOptions.generator) {
            info->cmakeOptions = env->workspace->defaultCMakeOptions;
        }
        info->activeConfig = DefaultNativeConfig;
        if (env->workspace->defaultConfig) {
            info->activeConfig = env->workspace->defaultConfig;
        }
        info->save();
        StdOut::text().format("Created build folder '{}' at: {}\n", info->buildFolderName,
                              info->getAbsPath());
        env->workspace->currentBuildFolder = name;
        env->workspace->save();
        StdOut::text().format("'{}' is now the current build folder.\n", info->buildFolderName);
    } else if (prefixMatch(cmd, "delete")) {
        StringView name = env->cl->readToken();
        if (name.isEmpty()) {
            fatalError("Expected folder name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        s32 index = find(env->buildFolders,
                         [&](const BuildFolder* bf) { return bf->buildFolderName == name; });
        if (index < 0) {
            fatalError(String::format("Folder \"{}\" does not exist", name));
        }

        // FIXME: Add confirmation prompt or -f/--force option
        BuildFolder* bf = env->buildFolders[index];
        if (FileSystem::native()->removeDirTree(bf->getAbsPath()) == FSResult::OK) {
            StdOut::text().format("Deleted build folder '{}'.\n", bf->buildFolderName);
        } else {
            fatalError(String::format("Can't delete build folder '{}'.\n", bf->buildFolderName));
        }
    } else if (prefixMatch(cmd, "set")) {
        StringView name = env->cl->readToken();
        if (name.isEmpty()) {
            fatalError("Expected folder name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        if (find(env->buildFolders,
                 [&](const BuildFolder* bf) { return bf->buildFolderName == name; }) < 0) {
            fatalError(String::format("Folder \"{}\" does not exist", name));
        }

        env->workspace->currentBuildFolder = name;
        env->workspace->save();
        StdOut::text().format("'{}' is now the current build folder.\n", name);
    } else {
        fatalError(String::format("Unrecognized folder command \"{}\"", cmd));
    }
}

} // namespace ply
