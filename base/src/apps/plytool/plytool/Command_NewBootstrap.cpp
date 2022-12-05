/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-repository/Instantiate.h>
#include <ply-build-repository/BuiltIns.h>

namespace ply {
namespace build2 {

void write_bootstrap(StringView bfPath, u32 configIndex) {
    // Write bulk.cpp
    {
        MemOutStream outs;
        for (const build2::Target* target : build2::Project.targets) {
            for (const build2::SourceGroup& sg : target->sourceGroups) {
                for (const build2::SourceFile& sf : sg.files) {
                    if (build2::hasBitAtIndex(sf.enabledBits, configIndex) &&
                        sf.relPath.endsWith(".cpp") && !sf.relPath.endsWith(".modules.cpp")) {
                        String includePath = PosixPath::from<NativePath>(NativePath::makeRelative(
                            bfPath, NativePath::join(sg.absPath, sf.relPath)));
                        outs.format("#include \"{}\"\n", includePath);
                    }
                }
            }
        }
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(NativePath::join(bfPath, "bulk.cpp"),
                                                             outs.moveToString(),
                                                             TextFormat::platformPreference());
    }

    // Write build.bat
    {
        Array<build2::Option> combinedOptions = build2::get_combined_options();
        MemOutStream outs;
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
                outs.format(" /I\"{}\"", opt.key);
            }
        }

        outs << " bulk.cpp";
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

        outs << "\n";

        FileSystem::native()->makeDirsAndSaveTextIfDifferent(NativePath::join(bfPath, "build.bat"),
                                                             outs.moveToString(),
                                                             TextFormat::platformPreference());
    }
}

} // namespace build2

void command_new_bootstrap(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

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
    build2::instantiate_all_configs(env->currentBuildFolder);
    build2::write_bootstrap(env->currentBuildFolder->getAbsPath(), 0);
}

} // namespace ply
