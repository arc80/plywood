/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-runtime/algorithm/Find.h>
#include <Workspace.h>

void command_target(CrowbarCommandEnv* env);
void command_codegen(CrowbarCommandEnv* env);
bool command_open(CrowbarCommandEnv* env);
void command_new_generate(CrowbarCommandEnv* env);
void command_new_bootstrap(CrowbarCommandEnv* env);

int main(int argc, char* argv[]) {
    Workspace.load();
    CommandLine cl{argc, argv};
    CrowbarCommandEnv env;
    env.cl = &cl;

    StringView category = cl.readToken();
    bool success = true;
    if (prefixMatch(category, "target")) {
        command_target(&env);
    } else if (prefixMatch(category, "generate")) {
        command_new_generate(&env);
    } else if (prefixMatch(category, "bootstrap")) {
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
