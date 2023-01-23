/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

// This file implements workspace commands in a way that can be called from either the
// command line or a GUI.

#include "core.h"
#include "workspace.h"
#include <ply-build-steps/Project.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>
#include <ply-runtime/string/WString.h>

using namespace ply::build;

#if PLY_TARGET_WIN32
#include <combaseapi.h>
#include <shellapi.h>
#endif

//                                             ▄▄
//   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄
//  ██  ██ ██▄▄██ ██  ██ ██▄▄██ ██  ▀▀  ▄▄▄██  ██   ██▄▄██
//  ▀█▄▄██ ▀█▄▄▄  ██  ██ ▀█▄▄▄  ██     ▀█▄▄██  ▀█▄▄ ▀█▄▄▄
//   ▄▄▄█▀

bool isMultiConfigCMakeGenerator(StringView generator) {
    if (generator.startsWith("Visual Studio")) {
        return true;
    } else if (generator == "Xcode") {
        return true;
    } else if (generator == "Unix Makefiles") {
        return false;
    } else {
        // FIXME: Make this a not-fatal warning instead, perhaps logging to some
        // kind of thread-local variable that can be set in the caller's scope.
        PLY_ASSERT(0); // Unrecognized generator
        return false;
    }
}

PLY_NO_INLINE Tuple<s32, String>
generateCMakeProject(StringView cmakeListsFolder,
                     const CMakeGeneratorOptions& generatorOpts, StringView config) {
    PLY_ASSERT(generatorOpts.generator);
    bool isMultiConfig = isMultiConfigCMakeGenerator(generatorOpts.generator);
    PLY_ASSERT(isMultiConfig || config);
    String buildFolder = Path.join(cmakeListsFolder, "build");
    String relPathToCMakeLists = "..";
    if (!isMultiConfig) {
        buildFolder = Path.join(buildFolder, config);
        relPathToCMakeLists = "../..";
    }
    FSResult result = FileSystem.makeDirs(buildFolder);
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
    Owned<Subprocess> sub =
        Subprocess::exec(PLY_CMAKE_PATH, Array<StringView>{args}, buildFolder,
                         Subprocess::Output::openMerged());
    String output = TextFormat::platformPreference()
                        .createImporter({sub->readFromStdOut, false})
                        .read_remaining_contents();
    s32 rc = sub->join();
    if (rc != 0) {
        Error.log("Error generating build system using CMake for folder '{}'\n",
                  buildFolder);
    }
    return {rc, std::move(output)};
}

void generate(BuildFolder_t* build_folder) {
    init_built_ins(build_folder);
    instantiate_all_configs(build_folder);
    String cmakeListsPath = Path.join(build_folder->absPath, "CMakeLists.txt");
    write_CMakeLists_txt_if_different(cmakeListsPath);
    Tuple<s32, String> result =
        generateCMakeProject(build_folder->absPath, build_folder->cmakeOptions, {});
    if (result.first != 0) {
        Error.log("Failed to generate build system for '{}':\n{}",
                  build_folder->solutionName, result.second);
    }
}

//  ▄▄                    ▄▄           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██   ▀█▄▄▄   ██   ██  ▀▀  ▄▄▄██ ██  ██
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀  ▀█▄▄  ▄▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ██▄▄█▀
//                                                        ██

void write_bootstrap(BuildFolder_t* build_folder, u32 configIndex) {
    init_built_ins(build_folder);
    instantiate_all_configs(build_folder);

    // Write crowbar_bulk.cpp
    {
        MemOutStream out;
        String sourceDir = Path.join(Workspace.path, "base/scripts");
        for (const Target* target : Project.targets) {
            for (const SourceGroup& sg : target->sourceGroups) {
                for (const SourceFile& sf : sg.files) {
                    if (hasBitAtIndex(sf.enabledBits, configIndex) &&
                        sf.relPath.endsWith(".cpp")) {
                        String includePath = PosixPath.from(
                            Path, Path.makeRelative(sourceDir,
                                                    Path.join(sg.absPath, sf.relPath)));
                        out.format("#include \"{}\"\n", includePath);
                    }
                }
            }
        }
        String sourcePath = Path.join(sourceDir, "crowbar_bulk.cpp");
        FSResult result = FileSystem.makeDirsAndSaveTextIfDifferent(
            sourcePath, out.moveToString(), TextFormat::platformPreference());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log("Error writing {}", sourcePath);
        }
    }

    // Write build.bat
    {
        Array<Option> combinedOptions = get_combined_options();
        MemOutStream out;
        out << "@echo off\n";
        out << "cl";

        // Compilation options
        CompilerSpecificOptions copts;
        for (const Option& opt : combinedOptions) {
            if (!hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == Option::Generic) {
                translate_toolchain_option(&copts, opt);
            }
        }
        for (StringView opt : copts.compile) {
            out << ' ' << opt;
        }
        out << " /Fd\"crowbar_bulk.pdb\"";

        // Preprocessor definitions
        for (const Option& opt : combinedOptions) {
            if (!hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == Option::PreprocessorDef) {
                if (opt.value) {
                    out.format(" /D{}={}", opt.key, opt.value);
                } else {
                    out.format(" /D{}", opt.key);
                }
            }
        }

        // Include directories
        for (const Option& opt : combinedOptions) {
            if (!hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == Option::IncludeDir) {
                out.format(" /I\"{}\"", Path.makeRelative(Workspace.path, opt.key));
            }
        }

        out << " base\\scripts\\crowbar_bulk.cpp";
        out << " /link";

        // Link options
        if (copts.link) {
            for (StringView opt : copts.link) {
                out << ' ' << opt;
            }
        }
        for (const Option& opt : combinedOptions) {
            if (opt.type == Option::LinkerInput) {
                out << ' ' << opt.key;
            }
        }

        out << " /incremental:no /out:crowbar.exe\n";
        out << "del crowbar_bulk.obj\n";
        out << "del crowbar_bulk.pdb\n";

        String batPath = Path.join(Workspace.path, "setup.bat");
        FSResult result = FileSystem.makeDirsAndSaveTextIfDifferent(
            batPath, out.moveToString(), TextFormat::platformPreference());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log("Error writing {}", batPath);
        }
    }
}

//
//   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄█▀ ██▄▄█▀ ▀█▄▄▄  ██  ██
//         ██

void command_open(BuildFolder_t* bf) {
    if (bf->cmakeOptions.generator == "Unix Makefiles") {
        Error.log("No IDE to open for Unix Makefiles");
#if PLY_TARGET_WIN32
    } else if (bf->cmakeOptions.generator.startsWith("Visual Studio")) {
        String slnPath = Path.join(bf->absPath, "build", bf->solutionName + ".sln");
        if (FileSystem.exists(slnPath) != ExistsResult::File) {
            Error.log("Can't find '{}'", slnPath);
        }

        // Open IDE
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) !=
            S_OK) {
            Error.log("Unable to initialize COM");
            exit(1);
        }
        ShellExecuteW(NULL, L"open", toWString(slnPath), NULL, NULL, SW_SHOWNORMAL);
        return;
#endif // PLY_TARGET_WIN32
#if PLY_TARGET_APPLE
    } else if (folder->cmakeOptions.generator == "Xcode") {
        String projPath = Path.join(folder->getAbsPath(), "build",
                                    folder->solutionName + ".xcodeproj");
        if (FileSystem.exists(projPath) != ExistsResult::Directory) {
            fatalError(String::format("Can't find '{}'", projPath));
        }

        // Open IDE
        Owned<Subprocess> sub =
            Subprocess::exec("open", Array<StringView>{projPath}.view(), {},
                             Subprocess::Output::inherit());
        if (!sub) {
            fatalError("Unable to open IDE using 'open'");
        }
        sub->join();
#endif // PLY_TARGET_APPLE
    }

    Error.log("Don't know how to open IDE for generator '{}'",
              bf->cmakeOptions.generator);
    exit(1);
}

//    ▄▄▄        ▄▄▄      ▄▄
//   ██    ▄▄▄▄   ██   ▄▄▄██  ▄▄▄▄  ▄▄▄▄▄
//  ▀██▀  ██  ██  ██  ██  ██ ██▄▄██ ██  ▀▀
//   ██   ▀█▄▄█▀ ▄██▄ ▀█▄▄██ ▀█▄▄▄  ██
//

bool create_build_folder(StringView name) {
    String abs_path = Path.join(Workspace.path, "data/build", name, "info.pylon");
    if (FileSystem.exists(abs_path) == ExistsResult::File) {
        Error.log("Build folder '{}' already exists", name);
        return false;
    }

    BuildFolder_t bf;
    bf.solutionName = name;
    bf.absPath = abs_path;
    bf.save();
    return true;
}

Array<String> get_build_folders() {
    Array<String> result;
    for (const FileInfo& file_info :
         FileSystem.listDir(Path.join(Workspace.path, "data/build"), 0)) {
        if (file_info.isDir) {
            if (FileSystem.exists(Path.join(Workspace.path, "data/build",
                                            file_info.name, "info.pylon")) ==
                ExistsResult::File) {
                result.append(file_info.name);
            }
        }
    }
    return result;
}

bool set_build_folder(StringView name) {
    if (FileSystem.exists(Path.join(Workspace.path, "data/build", name,
                                    "info.pylon")) != ExistsResult::File) {
        Error.log("Build folder '{}' does not exist", name);
        return false;
    }

    Workspace.currentBuildFolder = name;
    return Workspace.save();
}
