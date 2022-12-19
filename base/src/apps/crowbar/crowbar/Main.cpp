/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"
#include "command_line.h"
#include "workspace.h"
#include <ply-build-repo/BuildFolder.h>
#include <ply-build-repo/Common.h>
#include <ply-build-repo/Repository.h>
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>

using namespace ply::build;

void tidy_repo(StringView repoPath, StringView clangFormatPath, const TextFormat& tff);
void print_bigfont(StringView text);
void do_codegen();
bool command_open();
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const CMakeGeneratorOptions& generatorOpts,
                                        StringView config);
void write_bootstrap(u32 configIndex);

int main(int argc, char* argv[]) {
#if PLY_TARGET_WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    Workspace.load();
    CommandLine cl{argc, argv};

    StringView category = cl.readToken();
    bool success = true;
    if (prefixMatch(category, "tidy")) {
        ensureTerminated(&cl);
        cl.finalize();
        tidy_repo(NativePath::join(Workspace.path, "base/src"), {},
                  Workspace.getSourceTextFormat());
    } else if (prefixMatch(category, "target")) {
        StringView cmd = cl.readToken();
        if (cmd.isEmpty()) {
            exit(1);
        }

        if (prefixMatch(cmd, "add")) {
            StringView targetName = cl.readToken();
            if (targetName.isEmpty()) {
                Error.log("Expected target name");
                exit(1);
            }

            ensureTerminated(&cl);
            cl.finalize();
        } else {
            Error.log("Unrecognized target command '{}'", cmd);
        }
    } else if (prefixMatch(category, "generate")) {
        ensureTerminated(&cl);
        cl.finalize();

        Common::initialize();
        Repository::create();

        BuildFolder_t bf;
        bf.load(NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar"));
        PLY_SET_IN_SCOPE(BuildFolder, &bf);

        init_built_ins();
        instantiate_all_configs();
        String cmakeListsPath = NativePath::join(BuildFolder->absPath, "CMakeLists.txt");
        write_CMakeLists_txt_if_different(cmakeListsPath);
        Tuple<s32, String> result =
            generateCMakeProject(BuildFolder->absPath, BuildFolder->cmakeOptions, {});
        if (result.first != 0) {
            Error.log("Failed to generate build system for '{}':\n{}", BuildFolder->solutionName,
                      result.second);
        }
    } else if (prefixMatch(category, "bootstrap")) {
        ensureTerminated(&cl);
        StringView configName =
            cl.checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
        if (configName) {
            configName = configName.subStr(9);
        }
        cl.finalize();

        Common::initialize();
        Repository::create();

        BuildFolder_t bf;
        bf.absPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar");
        bf.solutionName = "crowbar";
        bf.rootTargets.append("crowbar");
        PLY_SET_IN_SCOPE(BuildFolder, &bf);

        init_built_ins();
        instantiate_all_configs();
        write_bootstrap(1);
    } else if (prefixMatch(category, "open")) {
        ensureTerminated(&cl);
        cl.finalize();
        success = command_open();
    } else if (prefixMatch(category, "codegen")) {
        ensureTerminated(&cl);
        do_codegen();
    } else if (prefixMatch(category, "bigfont")) {
        MemOutStream outs;
        while (StringView word = cl.readToken()) {
            if (outs.getSeekPos() > 0) {
                outs << " ";
            }
            outs << word;
        }
        cl.finalize();

        print_bigfont(outs.moveToString());
    } else {
        Error.log("Unrecognized command \"{}\"", category);
        success = false;
    }

    return success ? 0 : 1;
}
