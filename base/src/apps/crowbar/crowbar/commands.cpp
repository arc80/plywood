/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "core.h"
#include <ply-build-repo/Workspace.h>
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

bool is_multi_config_cmake_generator(StringView generator) {
    if (generator.starts_with("Visual Studio")) {
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

Tuple<s32, String> generate_cmake_project(StringView cmake_lists_folder,
                                          const CMakeGeneratorOptions& generator_opts,
                                          StringView config) {
    PLY_ASSERT(generator_opts.generator);
    bool is_multi_config = is_multi_config_cmake_generator(generator_opts.generator);
    PLY_ASSERT(is_multi_config || config);
    String build_folder = Path.join(cmake_lists_folder, "build");
    String rel_path_to_cmake_lists = "..";
    if (!is_multi_config) {
        build_folder = Path.join(build_folder, config);
        rel_path_to_cmake_lists = "../..";
    }
    FSResult result = FileSystem.make_dirs(build_folder);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        Error.log("Can't create folder '{}'\n", build_folder);
        return {-1, ""};
    }
    PLY_ASSERT(!generator_opts.generator.is_empty());
    Array<String> args = {rel_path_to_cmake_lists, "-G", generator_opts.generator};
    if (generator_opts.platform) {
        args.extend({"-A", generator_opts.platform});
    }
    if (generator_opts.toolset) {
        args.extend({"-T", generator_opts.toolset});
    }
    if (generator_opts.toolchain_file == "ios") {
        // FIXME: Verify that we're using CMake version 3.14 or higher
        args.append("-DCMAKE_SYSTEM_NAME=iOS");
    }
    if (!is_multi_config) {
        args.append(String::format("-DCMAKE_BUILD_TYPE={}", config));
    }
    args.extend({"-DCMAKE_C_COMPILER_FORCED=1", "-DCMAKE_CXX_COMPILER_FORCED=1"});
    Owned<Process> sub = Process::exec(PLY_CMAKE_PATH, Array<StringView>{args},
                                       build_folder, Process::Output::open_merged());
    String output = InStream{TextFormat::default_utf8().create_importer(
                                 {sub->read_from_std_out, false})}
                        .read_remaining_contents();
    s32 rc = sub->join();
    if (rc != 0) {
        Error.log("Error generating build system using CMake for folder '{}'\n",
                  build_folder);
    }
    return {rc, std::move(output)};
}

void generate(BuildFolder_t* build_folder) {
    init_built_ins(build_folder);
    instantiate_all_configs(build_folder);
    String cmake_lists_path = Path.join(build_folder->abs_path, "CMakeLists.txt");
    write_CMakeLists_txt_if_different(cmake_lists_path);
    Tuple<s32, String> result =
        generate_cmake_project(build_folder->abs_path, build_folder->cmake_options, {});
    if (result.first != 0) {
        Error.log("Failed to generate build system for '{}':\n{}",
                  build_folder->solution_name, result.second);
    }
}

//                       ▄▄            ▄▄ ▄▄▄      ▄▄
//  ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ██▄▄▄  ▄▄  ▄▄ ▄▄  ██   ▄▄▄██
//  ██  ██ ██  ▀▀ ██▄▄██ ██  ██ ██  ██ ██  ██  ██  ██
//  ██▄▄█▀ ██     ▀█▄▄▄  ██▄▄█▀ ▀█▄▄██ ██ ▄██▄ ▀█▄▄██
//  ██

void do_prebuild_steps(BuildFolder_t* build_folder, StringView config_name) {
    init_built_ins(build_folder);
    instantiate_all_configs(build_folder, config_name);
}

//  ▄▄                    ▄▄           ▄▄
//  ██▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██  ██  ██   ▀█▄▄▄   ██   ██  ▀▀  ▄▄▄██ ██  ██
//  ██▄▄█▀ ▀█▄▄█▀ ▀█▄▄█▀  ▀█▄▄  ▄▄▄█▀  ▀█▄▄ ██     ▀█▄▄██ ██▄▄█▀
//                                                        ██

void write_bootstrap(BuildFolder_t* build_folder, u32 config_index) {
    init_built_ins(build_folder);
    instantiate_all_configs(build_folder);

    // Write crowbar_bulk.cpp
    {
        MemOutStream out;
        String source_dir = Path.join(Workspace.path, "base/scripts");
        for (const Target* target : Project.targets) {
            for (const SourceGroup& sg : target->source_groups) {
                for (const SourceFile& sf : sg.files) {
                    if (has_bit_at_index(sf.enabled_bits, config_index) &&
                        sf.rel_path.ends_with(".cpp")) {
                        String include_path = PosixPath.from(
                            Path, Path.make_relative(
                                      source_dir, Path.join(sg.abs_path, sf.rel_path)));
                        out.format("#include \"{}\"\n", include_path);
                    }
                }
            }
        }
        String source_path = Path.join(source_dir, "crowbar_bulk.cpp");
        FSResult result = FileSystem.make_dirs_and_save_text_if_different(
            source_path, out.move_to_string());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log("Error writing {}", source_path);
        }
    }

    // Write build.bat
    {
        Array<Option> combined_options = get_combined_options();
        MemOutStream out;
        out << "@echo off\n";
        out << "cl";

        // Compilation options
        CompilerSpecificOptions copts;
        for (const Option& opt : combined_options) {
            if (!has_bit_at_index(opt.enabled_bits, config_index))
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
        for (const Option& opt : combined_options) {
            if (!has_bit_at_index(opt.enabled_bits, config_index))
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
        for (const Option& opt : combined_options) {
            if (!has_bit_at_index(opt.enabled_bits, config_index))
                continue;
            if (opt.type == Option::IncludeDir) {
                out.format(" /I\"{}\"", Path.make_relative(Workspace.path, opt.key));
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
        for (const Option& opt : combined_options) {
            if (opt.type == Option::LinkerInput) {
                out << ' ' << opt.key;
            }
        }

        out << " /incremental:no /out:crowbar.exe\n";
        out << "del crowbar_bulk.obj\n";
        out << "del crowbar_bulk.pdb\n";

        String bat_path = Path.join(Workspace.path, "setup.bat");
        FSResult result = FileSystem.make_dirs_and_save_text_if_different(
            bat_path, out.move_to_string());
        if ((result != FSResult::OK) && (result != FSResult::Unchanged)) {
            Error.log("Error writing {}", bat_path);
        }
    }
}

//
//   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄█▀ ██▄▄█▀ ▀█▄▄▄  ██  ██
//         ██

void command_open(BuildFolder_t* bf) {
    if (bf->cmake_options.generator == "Unix Makefiles") {
        Error.log("No IDE to open for Unix Makefiles");
#if PLY_TARGET_WIN32
    } else if (bf->cmake_options.generator.starts_with("Visual Studio")) {
        String sln_path = Path.join(bf->abs_path, "build", bf->solution_name + ".sln");
        if (FileSystem.exists(sln_path) != ExistsResult::File) {
            Error.log("Can't find '{}'", sln_path);
        }

        // Open IDE
        if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) !=
            S_OK) {
            Error.log("Unable to initialize COM");
            exit(1);
        }
        ShellExecuteW(NULL, L"open", to_wstring(sln_path), NULL, NULL, SW_SHOWNORMAL);
        return;
#endif // PLY_TARGET_WIN32
#if PLY_TARGET_APPLE
    } else if (folder->cmake_options.generator == "Xcode") {
        String proj_path = Path.join(folder->get_abs_path(), "build",
                                     folder->solution_name + ".xcodeproj");
        if (FileSystem.exists(proj_path) != ExistsResult::Directory) {
            fatal_error(String::format("Can't find '{}'", proj_path));
        }

        // Open IDE
        Owned<Process> sub = Process::exec("open", Array<StringView>{proj_path}.view(),
                                           {}, Process::Output::inherit());
        if (!sub) {
            fatal_error("Unable to open IDE using 'open'");
        }
        sub->join();
#endif // PLY_TARGET_APPLE
    }

    Error.log("Don't know how to open IDE for generator '{}'",
              bf->cmake_options.generator);
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
    bf.solution_name = name;
    bf.abs_path = abs_path;
    bf.save();
    return true;
}

Array<String> get_build_folders() {
    Array<String> result;
    for (const FileInfo& file_info :
         FileSystem.list_dir(Path.join(Workspace.path, "data/build"), 0)) {
        if (file_info.is_dir) {
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

    Workspace.current_build_folder = name;
    return Workspace.save();
}
