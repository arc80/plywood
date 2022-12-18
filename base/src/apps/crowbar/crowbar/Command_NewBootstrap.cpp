/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repository/Instantiate.h>
#include <ply-build-repository/BuiltIns.h>
#include <ply-build-folder/BuildFolder.h>

void write_bootstrap(u32 configIndex) {
    // Write bulk_crowbar.cpp
    {
        MemOutStream outs;
        String sourceDir = NativePath::join(Workspace.path, "base/scripts");
        for (const build2::Target* target : build2::Project.targets) {
            for (const build2::SourceGroup& sg : target->sourceGroups) {
                for (const build2::SourceFile& sf : sg.files) {
                    if (build2::hasBitAtIndex(sf.enabledBits, configIndex) &&
                        sf.relPath.endsWith(".cpp") && !sf.relPath.endsWith(".modules.cpp")) {
                        String includePath = PosixPath::from<NativePath>(NativePath::makeRelative(
                            sourceDir, NativePath::join(sg.absPath, sf.relPath)));
                        outs.format("#include \"{}\"\n", includePath);
                    }
                }
            }
        }
        String sourcePath = NativePath::join(sourceDir, "bulk_crowbar.cpp");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            sourcePath, outs.moveToString(), TextFormat::platformPreference());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log(String::format("Error writing {}", sourcePath));
        }
    }

    // Write build.bat
    {
        Array<build2::Option> combinedOptions = build2::get_combined_options();
        MemOutStream outs;
        outs << "@echo off\n";
        outs << "cl";

        // Compilation options
        build2::CompilerSpecificOptions copts;
        for (const build2::Option& opt : combinedOptions) {
            if (!build2::hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == build2::Option::Generic) {
                build2::translate_toolchain_option(&copts, opt);
            }
        }
        for (StringView opt : copts.compile) {
            outs << ' ' << opt;
        }
        outs << " /Fd\"bulk_crowbar.pdb\"";

        // Preprocessor definitions
        for (const build2::Option& opt : combinedOptions) {
            if (!build2::hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == build2::Option::PreprocessorDef) {
                if (opt.value) {
                    outs.format(" /D{}={}", opt.key, opt.value);
                } else {
                    outs.format(" /D{}", opt.key);
                }
            }
        }

        // Include directories
        for (const build2::Option& opt : combinedOptions) {
            if (!build2::hasBitAtIndex(opt.enabledBits, configIndex))
                continue;
            if (opt.type == build2::Option::IncludeDir) {
                outs.format(" /I\"{}\"", NativePath::makeRelative(Workspace.path, opt.key));
            }
        }

        outs << " base\\scripts\\bulk_crowbar.cpp";
        outs << " /link";

        // Link options
        if (copts.link) {
            for (StringView opt : copts.link) {
                outs << ' ' << opt;
            }
        }
        for (const build2::Option& opt : combinedOptions) {
            if (opt.type == build2::Option::LinkerInput) {
                outs << ' ' << opt.key;
            }
        }

        outs << " /incremental:no /out:crowbar-new.exe\n";
        outs << "del bulk_crowbar.obj\n";
        outs << "del bulk_crowbar.pdb\n";

        String batPath = NativePath::join(Workspace.path, "setup.bat");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            batPath, outs.moveToString(), TextFormat::platformPreference());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log(String::format("Error writing {}", batPath));
        }
    }
}

void command_new_bootstrap(CrowbarCommandEnv* env) {
    ensureTerminated(env->cl);
    StringView configName =
        env->cl->checkForSkippedOpt([](StringView arg) { return arg.startsWith("--config="); });
    if (configName) {
        configName = configName.subStr(9);
    }
    env->cl->finalize();

    build2::Common::initialize();
    build2::init_built_ins();
    build2::Repository::create();
    build::BuildFolder.absPath = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build/crowbar");
    build::BuildFolder.solutionName = "crowbar";
    build::BuildFolder.rootTargets.append("crowbar");
    build2::instantiate_all_configs();
    write_bootstrap(1);
}
