/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {

void command_rpc(PlyToolCommandEnv* env);
void command_folder(PlyToolCommandEnv* env);
void command_target(PlyToolCommandEnv* env);
void command_module(PlyToolCommandEnv* env);
void command_extern(PlyToolCommandEnv* env);
bool command_generate(PlyToolCommandEnv* env);
bool command_build(PlyToolCommandEnv* env);
void command_codegen(PlyToolCommandEnv* env);
void command_bootstrap(PlyToolCommandEnv* env);
void command_cleanup(PlyToolCommandEnv* env);
s32 command_run(PlyToolCommandEnv* env);
bool command_open(PlyToolCommandEnv* env);
void command_new_generate(PlyToolCommandEnv* env);
void command_new_bootstrap(PlyToolCommandEnv* env);

} // namespace ply

int main(int argc, char* argv[]) {
    using namespace ply;

    auto errorHandler = [](build::ErrorHandler::Level errorLevel, HybridString&& error) {
        Owned<OutStream> outs;
        if (errorLevel == build::ErrorHandler::Error || errorLevel == build::ErrorHandler::Fatal) {
            outs = new OutStream{StdErr::text()};
            *outs << "Error: ";
        } else {
            outs = new OutStream{StdOut::text()};
        }
        *outs << error.view();
        if (!error.view().endsWith("\n")) {
            *outs << '\n';
        }
    };
    PLY_SET_IN_SCOPE(build::ErrorHandler::current, errorHandler);

    WorkspaceSettings workspace;
    if (!workspace.load()) {
        fatalError(String::format("Invalid workspace settings file: \"{}\"",
                                  WorkspaceSettings::getPath()));
    }

    CommandLine cl{argc, argv};

    PlyToolCommandEnv env;
    env.cl = &cl;
    env.workspace = &workspace;

    // FIXME: Only call getBuildFolders when really needed
    env.buildFolders = build::BuildFolder::getList();
    s32 defaultIndex = find(env.buildFolders, [&](const build::BuildFolder* bf) {
        return bf->buildFolderName == workspace.currentBuildFolder;
    });
    if (defaultIndex >= 0) {
        env.currentBuildFolder = env.buildFolders[defaultIndex];
    }

    StringView category = cl.readToken();
    bool success = true;
    if (category.isEmpty()) {
        cl.finalize();
        auto outs = StdErr::text();
        const CommandList commands = {
            {"bootstrap", "bootstrap description"}, {"build", "build description"},
            {"cleanup", "cleanup description"},     {"codegen", "codegen description"},
            {"extern", "extern description"},       {"folder", "folder description"},
            {"generate", "generate description"},   {"module", "module description"},
            {"target", "target description"},
        };
        printUsage(&outs, commands);
    } else if (category == "rpc") {
        command_rpc(&env);
    } else if (prefixMatch(category, "folder")) {
        command_folder(&env);
    } else if (prefixMatch(category, "target")) {
        command_target(&env);
    } else if (prefixMatch(category, "module")) {
        command_module(&env);
    } else if (prefixMatch(category, "extern")) {
        command_extern(&env);
    } else if (prefixMatch(category, "generate")) {
        success = command_generate(&env);
    } else if (prefixMatch(category, "new_generate")) {
        command_new_generate(&env);
    } else if (prefixMatch(category, "new_bootstrap")) {
        command_new_bootstrap(&env);
    } else if (prefixMatch(category, "build")) {
        success = command_build(&env);
    } else if (prefixMatch(category, "run")) {
        return command_run(&env);
    } else if (prefixMatch(category, "open")) {
        success = command_open(&env);
    } else if (prefixMatch(category, "codegen")) {
        command_codegen(&env);
    } else if (prefixMatch(category, "bootstrap")) {
        command_bootstrap(&env);
    } else if (prefixMatch(category, "cleanup")) {
        command_cleanup(&env);
    } else {
        fatalError(String::format("Unrecognized command \"{}\"", category));
    }
    return success ? 0 : 1;
}
