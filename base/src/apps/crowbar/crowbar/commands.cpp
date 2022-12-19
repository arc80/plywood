/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"
#include "workspace.h"
#include <ply-build-steps/Project.h>
#include <ply-build-repo/BuildFolder.h>
#include <ply-runtime/string/WString.h>
#include <ply-runtime/io/text/TextConverter.h>

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

//  ▄▄                    ▄▄           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██   ▀█▄▄▄   ██   ██  ▀▀  ▄▄▄██ ██  ██
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀  ▀█▄▄  ▄▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ██▄▄█▀
//                                                        ██

void write_bootstrap(u32 configIndex) {
    // Write crowbar_bulk.cpp
    {
        MemOutStream outs;
        String sourceDir = NativePath::join(Workspace.path, "base/scripts");
        for (const Target* target : Project.targets) {
            for (const SourceGroup& sg : target->sourceGroups) {
                for (const SourceFile& sf : sg.files) {
                    if (hasBitAtIndex(sf.enabledBits, configIndex) && sf.relPath.endsWith(".cpp") &&
                        !sf.relPath.endsWith(".modules.cpp")) {
                        String includePath = PosixPath::from<NativePath>(NativePath::makeRelative(
                            sourceDir, NativePath::join(sg.absPath, sf.relPath)));
                        outs.format("#include \"{}\"\n", includePath);
                    }
                }
            }
        }
        String sourcePath = NativePath::join(sourceDir, "crowbar_bulk.cpp");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            sourcePath, outs.moveToString(), TextFormat::platformPreference());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log("Error writing {}", sourcePath);
        }
    }

    // Write build.bat
    {
        Array<Option> combinedOptions = get_combined_options();
        MemOutStream outs;
        outs << "@echo off\n";
        outs << "cl";

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
            outs << ' ' << opt;
        }
        outs << " /Fd\"crowbar_bulk.pdb\"";

        // Preprocessor definitions
        for (const Option& opt : combinedOptions) {
            if (!hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == Option::PreprocessorDef) {
                if (opt.value) {
                    outs.format(" /D{}={}", opt.key, opt.value);
                } else {
                    outs.format(" /D{}", opt.key);
                }
            }
        }

        // Include directories
        for (const Option& opt : combinedOptions) {
            if (!hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == Option::IncludeDir) {
                outs.format(" /I\"{}\"", NativePath::makeRelative(Workspace.path, opt.key));
            }
        }

        outs << " base\\scripts\\crowbar_bulk.cpp";
        outs << " /link";

        // Link options
        if (copts.link) {
            for (StringView opt : copts.link) {
                outs << ' ' << opt;
            }
        }
        for (const Option& opt : combinedOptions) {
            if (opt.type == Option::LinkerInput) {
                outs << ' ' << opt.key;
            }
        }

        outs << " /incremental:no /out:crowbar.exe\n";
        outs << "del crowbar_bulk.obj\n";
        outs << "del crowbar_bulk.pdb\n";

        String batPath = NativePath::join(Workspace.path, "setup.bat");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            batPath, outs.moveToString(), TextFormat::platformPreference());
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

bool command_open() {
    BuildFolder_t bf;
    bf.load(NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar"));

    if (bf.cmakeOptions.generator == "Unix Makefiles") {
        Error.log("No IDE to open for Unix Makefiles");
#if PLY_TARGET_WIN32
    } else if (bf.cmakeOptions.generator.startsWith("Visual Studio")) {
        String slnPath =
            NativePath::join(bf.absPath, "build", bf.solutionName + ".sln");
        if (FileSystem::native()->exists(slnPath) != ExistsResult::File) {
            Error.log("Can't find '{}'", slnPath);
        }

        // Convert to UTF-16 path
        WString wstr;
        {
            MemOutStream mout;
            StringView srcView = slnPath.view();
            TextConverter::create<UTF16_Native, UTF8>().writeTo(&mout, &srcView, true);
            mout << StringView{"\0\0", 2}; // null terminated
            wstr = WString::moveFromString(mout.moveToString());
        }

        // Open IDE
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) != S_OK) {
            Error.log("Unable to initialize COM");
            exit(1);
        }
        return ((sptr) ShellExecuteW(NULL, L"open", wstr, NULL, NULL, SW_SHOWNORMAL) > 32);
#endif // PLY_TARGET_WIN32
#if PLY_TARGET_APPLE
    } else if (folder->cmakeOptions.generator == "Xcode") {
        String projPath =
            NativePath::join(folder->getAbsPath(), "build", folder->solutionName + ".xcodeproj");
        if (FileSystem::native()->exists(projPath) != ExistsResult::Directory) {
            fatalError(String::format("Can't find '{}'", projPath));
        }

        // Open IDE
        Owned<Subprocess> sub = Subprocess::exec("open", Array<StringView>{projPath}.view(), {},
                                                 Subprocess::Output::inherit());
        if (!sub) {
            fatalError("Unable to open IDE using 'open'");
        }
        return sub->join() == 0;
#endif // PLY_TARGET_APPLE
    }

    Error.log("Don't know how to open IDE for generator '{}'", bf.cmakeOptions.generator);
    exit(1);
    return false;
}
