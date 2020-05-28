/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-build-provider/ToolchainInfo.h>

namespace ply {
namespace build {

struct ProjectInstantiationEnv;
struct ProjectInstantiationResult;
struct DependencyTree;
struct RepoRegistry;
struct ExternFolderRegistry;
struct ExternSelector;

struct BuildFolder {
    String buildFolderName; // Not written to the .pylon file

    PLY_REFLECT()
    String solutionName;
    CMakeGeneratorOptions cmakeOptions;
    Array<String> rootTargets;
    Array<String> makeShared;
    Array<String> externSelectors;
    // ply reflect off

    String getAbsPath() const;

    // BuildFolder management
    static Owned<BuildFolder> create(StringView buildFolderName, StringView solutionName);
    static Owned<BuildFolder> load(StringView buildFolderName);
    bool save() const;

    // Operations
    Owned<ProjectInstantiationEnv> createEnvironment() const;
    ProjectInstantiationResult instantiateAllTargets() const;
    DependencyTree buildDepTree() const;
    bool generate(const ProjectInstantiationResult* instResult) const;
    bool build(StringView buildType, bool captureOutput = true) const;

    static Array<Owned<BuildFolder>> getList();
};

ToolchainInfo toolchainInfoFromCMakeOptions(const CMakeGeneratorOptions& cmakeOpts);

} // namespace build
} // namespace ply
