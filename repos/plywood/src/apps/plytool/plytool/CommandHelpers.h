/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ply-build-repo/ProjectInstantiator.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

struct CommandLine;

struct AddParams {
    bool makeShared = false;

    void extractOptions(CommandLine* cl);
    bool exec(build::BuildFolder* folder, StringView fullTargetName);
};

struct BuildParams {
    String targetName;
    String configName;
    bool doAdd = false;
    AddParams addParams;

    struct Result {
        build::ProjectInstantiationResult instResult;
        const build::BuildTarget* runTarget = nullptr;
        const build::TargetInstantiator* runTargetInst = nullptr;
    };

    void extractOptions(CommandLine* cl, const build::BuildFolder* folder);
    bool exec(Result* result, build::BuildFolder* folder, bool doBuild);
};

} // namespace ply
