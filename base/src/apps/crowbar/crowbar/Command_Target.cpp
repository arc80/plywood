/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-runtime/algorithm/Find.h>

using namespace ply::build;

struct DepTreeIndent {
    String node;
    String children;
};

void command_target(CrowbarCommandEnv* env) {
    using namespace build;

    StringView cmd = env->cl->readToken();
    if (cmd.isEmpty()) {
        exit(1);
    }

    if (prefixMatch(cmd, "add")) {
        StringView targetName = env->cl->readToken();
        if (targetName.isEmpty()) {
            Error.log("Expected target name");
            exit(1);
        }

        ensureTerminated(env->cl);
        env->cl->finalize();

        BuildFolder.save();
    } else {
        Error.log(String::format("Unrecognized target command '{}'", cmd));
    }
}
