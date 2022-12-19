/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>
#include <ply-build-repo/BuildFolder.h>
#include <Workspace.h>

using namespace ply::build;

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

void command_bootstrap(CommandLine* cl) {
    ensureTerminated(cl);
    StringView configName =
        cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    cl->finalize();

    Common::initialize();
    Repository::create();

    BuildFolder_t bf;
    bf.absPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar");
    bf.solutionName = "crowbar";
    bf.rootTargets.append("crowbar");
    PLY_SET_IN_SCOPE(BuildFolder, &bf);

    init_built_ins();
    instantiate_all_configs();
    write_bootstrap(1);
}
