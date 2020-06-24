/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/ProjectInstantiator.h>

namespace ply {

namespace build {
struct BuildFolder;
} // namespace build

struct AddParams {
    bool makeShared = false;

    void extractOptions(CommandLine* cl);
    bool exec(build::BuildFolder* folder, StringView fullTargetName);
};

struct BuildParams {
    String targetName;
    String configName;
    bool doAdd = false;
    bool doAuto = false;
    AddParams addParams;

    struct Result {
        build::ProjectInstantiationResult instResult;
        const build::BuildTarget* runTarget = nullptr;
        const build::TargetInstantiator* runTargetInst = nullptr;
        build::BuildFolder* folder = nullptr;
    };

    void extractOptions(PlyToolCommandEnv* env);
    bool exec(Result* result, PlyToolCommandEnv* env, bool doBuild);
};

} // namespace ply
