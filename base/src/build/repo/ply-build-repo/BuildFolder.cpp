/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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

bool BuildFolder_t::load(StringView abs_path) {
    ArrayView<float> xx{(float*) nullptr, (u32) 0};
    String info_path = Path.join(abs_path, "info.pylon");
    String str_contents = FileSystem.load_text_autodetect(info_path);
    if (FileSystem.last_result() != FSResult::OK) {
        Error.log("Unable to read file '{}'", info_path);
        return false;
    }

    Owned<pylon::Node> a_root = pylon::Parser{}.parse(info_path, str_contents).root;
    if (!a_root->is_valid()) {
        Error.log("Unable to parse the contents of '{}'", info_path);
        return false;
    }

    this->abs_path = abs_path;
    pylon::import_into(AnyObject::bind(this), a_root);
    return true;
}

bool BuildFolder_t::save() const {
    Owned<pylon::Node> a_root = pylon::export_obj(AnyObject::bind(this));
    String str_contents = pylon::to_string(a_root);
    String info_path = Path.join(this->abs_path, "info.pylon");
    FSResult rc =
        FileSystem.make_dirs_and_save_text_if_different(info_path, str_contents);
    if ((rc != FSResult::OK) && (rc != FSResult::Unchanged)) {
        Error.log("Unable to save file '{}'", info_path);
        return false;
    }
    return true;
}

bool is_multi_config_cmake_generator(StringView generator) {
    if (generator.starts_with("Visual Studio")) {
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

s32 build_cmake_project(StringView cmake_lists_folder,
                        const CMakeGeneratorOptions& generator_opts, StringView config,
                        StringView target_name) {
    PLY_ASSERT(generator_opts.generator);
    PLY_ASSERT(config);
    String build_folder = Path.join(cmake_lists_folder, "build");
    bool is_multi_config = is_multi_config_cmake_generator(generator_opts.generator);
    if (!is_multi_config) {
        build_folder = Path.join(build_folder, config);
    }
    Process::Output output_type = Process::Output::inherit();
    Owned<Process> sub;
    if (generator_opts.generator == "Unix Makefiles") {
        Array<HybridString> args = {};
        u32 hw_threads = Affinity{}.get_num_hwthreads();
        if (hw_threads > 1) {
            args.extend({"-j", to_string(hw_threads)});
        }
        if (target_name) {
            args.append(target_name);
        }
        sub = Process::exec("make", Array<StringView>{args}, build_folder, output_type);
    } else {
        Array<StringView> args = {"--build", "."};
        if (is_multi_config) {
            args.extend({"--config", config});
        }
        if (target_name) {
            args.extend({"--target", target_name});
        }
        sub = Process::exec(PLY_CMAKE_PATH, args, build_folder, output_type);
    }
    return sub->join();
}

/*
String get_target_output_path(BuildTargetType target_type, StringView target_name,
                           StringView build_folder_path, StringView config) {
    PLY_ASSERT(config);

    // FIXME: The following logic assumes we're always using a native toolchain. In
order to make it
    // work with cross-compilers, we'll need to pass in more information about the
target platform,
    // perhaps using ToolchainInfo. (In that case, the real question will be, in
general, how to
    // initialize that ToolchainInfo.)
    StringView file_prefix;
    StringView file_extension;
    if (target_type == BuildTargetType::EXE) {
#if PLY_TARGET_WIN32
        file_extension = ".exe";
#endif
    } else if (target_type == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        file_extension = ".dll";
#elif PLY_TARGET_APPLE
        file_prefix = "lib";
        file_extension = ".dylib";
#else
        file_prefix = "lib";
        file_extension = ".so";
#endif
    } else if (target_type == BuildTargetType::DLL) {
#if PLY_TARGET_WIN32
        file_extension = ".lib";
#else
        file_prefix = "lib";
        file_extension = ".a";
#endif
    } else {
        PLY_ASSERT(0); // Not supported
    }

    // Compose full path to the target output:
    Array<StringView> path_components = {build_folder_path, "build", config};
    String full_name = file_prefix + target_name + file_extension;
    path_components.append(full_name);
    return Path.join_array(Array<StringView>{path_components});
}
*/

bool BuildFolder_t::build(StringView config, StringView target_name) const {
    // Note: Should we check that target_name actually exists in the build folder before
    // invoking CMake? If target_name isn't a root target, this would require us to
    // instaniate all dependencies first.
    if (!config) {
        config = this->active_config;
        if (!config) {
            Error.log("Active config not set");
        }
    }
    Error.log("Building {} configuration of '{}'...\n", config,
              target_name ? target_name : this->solution_name.view());

    s32 rc =
        build_cmake_project(this->abs_path, this->cmake_options, config, target_name);
    if (rc != 0) {
        Error.log("Build failed");
        return false;
    }
    return true;
}

} // namespace build
} // namespace ply

#include "codegen/BuildFolder.inl" //%%
