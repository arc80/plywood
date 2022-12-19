/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-runtime/algorithm/Find.h>
#include <Workspace.h>

void command_target(CommandLine* cl);
void command_codegen(CommandLine* cl);
bool command_open(CommandLine* cl);
void command_generate(CommandLine* cl);
void command_bootstrap(CommandLine* cl);
void command_bigfont(CommandLine* cl);

int main(int argc, char* argv[]) {
#if PLY_TARGET_WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    Workspace.load();
    CommandLine cl{argc, argv};

    StringView category = cl.readToken();
    bool success = true;
    if (prefixMatch(category, "target")) {
        command_target(&cl);
    } else if (prefixMatch(category, "generate")) {
        command_generate(&cl);
    } else if (prefixMatch(category, "bootstrap")) {
        command_bootstrap(&cl);
    } else if (prefixMatch(category, "open")) {
        success = command_open(&cl);
    } else if (prefixMatch(category, "codegen")) {
        command_codegen(&cl);
    } else if (prefixMatch(category, "bigfont")) {
        command_bigfont(&cl);
    } else {
        Error.log("Unrecognized command \"{}\"", category);
        success = false;
    }

    return success ? 0 : 1;
}
