/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Core.h>
#include <ply-build-repo/BuildFolder.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>
#include <ply-runtime/container/Hash128.h>

namespace ply {
namespace build {

BuildFolder_t* BuildFolder = nullptr;

PLY_NO_INLINE bool BuildFolder_t::load(StringView absPath) {
    ArrayView<float> xx{(float*) nullptr, (u32) 0};
    String infoPath = Path.join(absPath, "info.pylon");
    String strContents = FileSystem.loadTextAutodetect(infoPath);
    if (FileSystem.lastResult() != FSResult::OK) {
        Error.log("Unable to read file '{}'", infoPath);
        return false;
    }

    Owned<pylon::Node> aRoot = pylon::Parser{}.parse(infoPath, strContents).root;
    if (!aRoot->isValid()) {
        Error.log("Unable to parse the contents of '{}'", infoPath);
        return false;
    }

    this->absPath = absPath;
    pylon::importInto(AnyObject::bind(this), aRoot);
    return true;
}

PLY_NO_INLINE bool BuildFolder_t::save() const {
    Owned<pylon::Node> aRoot = pylon::exportObj(AnyObject::bind(this));
    String strContents = pylon::toString(aRoot);
    String infoPath = Path.join(this->absPath, "info.pylon");
    FSResult rc = FileSystem.makeDirsAndSaveTextIfDifferent(
        infoPath, strContents, TextFormat::platformPreference());
    if ((rc != FSResult::OK) && (rc != FSResult::Unchanged)) {
        Error.log("Unable to save file '{}'", infoPath);
        return false;
    }
    return true;
}

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

PLY_NO_INLINE s32 buildCMakeProject(StringView cmakeListsFolder,
                                    const CMakeGeneratorOptions& generatorOpts, StringView config,
                                    StringView targetName) {
    PLY_ASSERT(generatorOpts.generator);
    PLY_ASSERT(config);
    String buildFolder = Path.join(cmakeListsFolder, "build");
    bool isMultiConfig = isMultiConfigCMakeGenerator(generatorOpts.generator);
    if (!isMultiConfig) {
        buildFolder = Path.join(buildFolder, config);
    }
    Subprocess::Output outputType = Subprocess::Output::inherit();
    Owned<Subprocess> sub;
    if (generatorOpts.generator == "Unix Makefiles") {
        Array<HybridString> args = {};
        u32 hwThreads = Affinity{}.getNumHWThreads();
        if (hwThreads > 1) {
            args.extend({"-j", to_string(hwThreads)});
        }
        if (targetName) {
            args.append(targetName);
        }
        sub = Subprocess::exec("make", Array<StringView>{args}, buildFolder, outputType);
    } else {
        Array<StringView> args = {"--build", "."};
        if (isMultiConfig) {
            args.extend({"--config", config});
        }
        if (targetName) {
            args.extend({"--target", targetName});
        }
        sub = Subprocess::exec(PLY_CMAKE_PATH, args, buildFolder, outputType);
    }
    return sub->join();
}

/*
String getTargetOutputPath(BuildTargetType targetType, StringView targetName,
                           StringView buildFolderPath, StringView config) {
    PLY_ASSERT(config);

    // FIXME: The following logic assumes we're always using a native toolchain. In order to make it
    // work with cross-compilers, we'll need to pass in more information about the target platform,
    // perhaps using ToolchainInfo. (In that case, the real question will be, in general, how to
    // initialize that ToolchainInfo.)
    StringView filePrefix;
    StringView fileExtension;
    if (targetType == BuildTargetType::EXE) {
#if PLY_TARGET_WIN32
        fileExtension = ".exe";
#endif
    } else if (targetType == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        fileExtension = ".dll";
#elif PLY_TARGET_APPLE
        filePrefix = "lib";
        fileExtension = ".dylib";
#else
        filePrefix = "lib";
        fileExtension = ".so";
#endif
    } else if (targetType == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        fileExtension = ".lib";
#else
        filePrefix = "lib";
        fileExtension = ".a";
#endif
    } else {
        PLY_ASSERT(0); // Not supported
    }

    // Compose full path to the target output:
    Array<StringView> pathComponents = {buildFolderPath, "build", config};
    String fullName = filePrefix + targetName + fileExtension;
    pathComponents.append(fullName);
    return Path.joinArray(Array<StringView>{pathComponents});
}
*/

PLY_NO_INLINE bool BuildFolder_t::build(StringView config, StringView targetName) const {
    // Note: Should we check that targetName actually exists in the build folder before invoking
    // CMake? If targetName isn't a root target, this would require us to instaniate all
    // dependencies first.
    if (!config) {
        config = this->activeConfig;
        if (!config) {
            Error.log("Active config not set");
        }
    }
    Error.log("Building {} configuration of '{}'...\n", config,
              targetName ? targetName : this->solutionName.view());

    s32 rc = buildCMakeProject(this->absPath, this->cmakeOptions, config, targetName);
    if (rc != 0) {
        Error.log("Build failed");
        return false;
    }
    return true;
}

} // namespace build
} // namespace ply

#include "codegen/BuildFolder.inl" //%%
