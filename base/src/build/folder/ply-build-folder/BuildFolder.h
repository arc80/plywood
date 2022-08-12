/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/CMakeLists.h>
#include <pylon/Node.h>

namespace ply {
namespace build {

struct ProjectInstantiationEnv;
struct ProjectInstantiationResult;
struct DependencyTree;
struct RepoRegistry;
struct ExternFolderRegistry;
struct ExternSelector;

struct BuildFolder {
    String buildFolderName;                  // Not saved to the .pylon file
    Owned<pylon::Node> buildSystemSignature; // determines whether build system needs regenerating

    PLY_REFLECT()
    String solutionName;
    CMakeGeneratorOptions cmakeOptions;
    Array<String> rootTargets;
    Array<String> makeShared;
    Array<String> externSelectors;
    String activeConfig;
    String activeTarget;
    // ply reflect off

    String getAbsPath() const;

    // BuildFolder management
    static Owned<BuildFolder> create(StringView buildFolderName, StringView solutionName);
    static Owned<BuildFolder> autocreate(StringView baseName);
    static Owned<BuildFolder> load(StringView buildFolderName);
    bool save() const;

    // Operations
    Owned<ProjectInstantiationEnv> createEnvironment() const;
    ProjectInstantiationResult instantiateAllTargets(bool isGenerating) const;
    void instantiateLatest(bool isGenerating) const;
    DependencyTree buildDepTree() const;
    u128 currentBuildSystemSignature() const;
    bool isGenerated(StringView config) const;
    bool generate(StringView config, const ProjectInstantiationResult* instResult);
    bool generateLoop(StringView config);
    bool build(StringView config, StringView targetName, bool captureOutput) const;

    static Array<Owned<BuildFolder>> getList();
};

Owned<pylon::Node> toolchainInfoFromCMakeOptions(const CMakeGeneratorOptions& cmakeOpts);

} // namespace build
} // namespace ply
