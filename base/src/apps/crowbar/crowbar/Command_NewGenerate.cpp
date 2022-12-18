/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>
#include <ply-build-repo/BuildFolder.h>

using namespace ply::build;

bool isMultiConfigCMakeGenerator(StringView generator) {
    if (generator.startsWith("Visual Studio")) {
        return true;
    } else if (generator == "Xcode") {
        return true;
    } else if (generator == "Unix Makefiles") {
        return false;
    } else {
        // FIXME: Make this a not-fatal warning instead, perhaps logging to some kind of
        // thread-local variable that can be set in the caller's scope.
        PLY_ASSERT(0); // Unrecognized generator
        return false;
    }
}

PLY_NO_INLINE Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                                      const CMakeGeneratorOptions& generatorOpts,
                                                      StringView config) {
    PLY_ASSERT(generatorOpts.generator);
    bool isMultiConfig = isMultiConfigCMakeGenerator(generatorOpts.generator);
    PLY_ASSERT(isMultiConfig || config);
    String buildFolder = NativePath::join(cmakeListsFolder, "build");
    String relPathToCMakeLists = "..";
    if (!isMultiConfig) {
        buildFolder = NativePath::join(buildFolder, config);
        relPathToCMakeLists = "../..";
    }
    FSResult result = FileSystem::native()->makeDirs(buildFolder);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        Error.log("Can't create folder '{}'\n", buildFolder);
        return {-1, ""};
    }
    PLY_ASSERT(!generatorOpts.generator.isEmpty());
    Array<String> args = {relPathToCMakeLists, "-G", generatorOpts.generator};
    if (generatorOpts.platform) {
        args.extend({"-A", generatorOpts.platform});
    }
    if (generatorOpts.toolset) {
        args.extend({"-T", generatorOpts.toolset});
    }
    if (generatorOpts.toolchainFile == "ios") {
        // FIXME: Verify that we're using CMake version 3.14 or higher
        args.append("-DCMAKE_SYSTEM_NAME=iOS");
    }
    if (!isMultiConfig) {
        args.append(String::format("-DCMAKE_BUILD_TYPE={}", config));
    }
    args.extend({"-DCMAKE_C_COMPILER_FORCED=1", "-DCMAKE_CXX_COMPILER_FORCED=1"});
    Owned<Subprocess> sub = Subprocess::exec(PLY_CMAKE_PATH, Array<StringView>{args}, buildFolder,
                                             Subprocess::Output::openMerged());
    String output = TextFormat::platformPreference()
                        .createImporter(Owned<InStream>::create(sub->readFromStdOut.borrow()))
                        ->readRemainingContents();
    s32 rc = sub->join();
    if (rc != 0) {
        Error.log("Error generating build system using CMake for folder '{}'\n", buildFolder);
    }
    return {rc, std::move(output)};
}

void command_new_generate(CommandLine* cl) {
    Common::initialize();
    init_built_ins();
    Repository::create();
    instantiate_all_configs();
    String cmakeListsPath = NativePath::join(BuildFolder.absPath, "CMakeLists.txt");
    write_CMakeLists_txt_if_different(cmakeListsPath);
    Tuple<s32, String> result =
        generateCMakeProject(BuildFolder.absPath, BuildFolder.cmakeOptions, {});
    if (result.first != 0) {
        Error.log("Failed to generate build system for '{}':\n{}", BuildFolder.solutionName,
                  result.second);
    }
}
