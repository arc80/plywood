/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-runtime/algorithm/Find.h>
#include <Workspace.h>

void command_target(PlyToolCommandEnv* env);
void command_codegen(PlyToolCommandEnv* env);
bool command_open(PlyToolCommandEnv* env);
void command_new_generate(PlyToolCommandEnv* env);
void command_new_bootstrap(PlyToolCommandEnv* env);

int main(int argc, char* argv[]) {
    using namespace ply;

    Workspace.load();
    CommandLine cl{argc, argv};
    PlyToolCommandEnv env;
    env.cl = &cl;

    StringView category = cl.readToken();
    bool success = true;
    if (prefixMatch(category, "target")) {
        command_target(&env);
    } else if (prefixMatch(category, "new_generate")) {
        command_new_generate(&env);
    } else if (prefixMatch(category, "new_bootstrap")) {
        command_new_bootstrap(&env);
    } else if (prefixMatch(category, "open")) {
        success = command_open(&env);
    } else if (prefixMatch(category, "codegen")) {
        command_codegen(&env);
    } else {
        Error.log(String::format("Unrecognized command \"{}\"", category));
        success = false;
    }

    return success ? 0 : 1;
}
