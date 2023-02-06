/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-steps/Project.h>

namespace ply {
namespace build {

struct CMakeEscape : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    CMakeEscape(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

void CMakeEscape::do_print(OutStream& out, const FormatArg& arg) {
    PLY_ASSERT(arg.type == View);
    StringView src_units = arg.view;
    while (src_units.num_bytes > 0) {
        char c = src_units.bytes[0];
        switch (c) {
            case '$':
            case '<':
            case '>':
            case '(':
            case ')':
            case '#':
            case '"':
            case '\\':
            case ';':
            case ',': {
                out << '\\';
                break;
            }
            default: {
                break;
            }
        }
        out << c;
        src_units.offset_head(1);
    }
}

String escape_cmake_list(ArrayView<const StringView> copts) {
    MemOutStream mout;
    bool first = true;
    for (StringView copt : copts) {
        if (!first) {
            mout << ';';
        }
        mout << to_string(CMakeEscape{copt});
        first = false;
    }
    return mout.move_to_string();
}

void write_CMakeLists_txt_if_different(StringView path) {
    PLY_ASSERT(Project.name);
    PLY_ASSERT(!Project.config_names.is_empty());
    PLY_ASSERT(Project.config_names.num_items() < 64);
    PLY_ASSERT(Project.did_inheritance);

    MemOutStream out;

    // Write header with list of configuration names
    out << "cmake_minimum_required(VERSION 3.12)\n";
    out.format(
        "set(CMAKE_CONFIGURATION_TYPES \"{}\" CACHE INTERNAL \"Build configs\")\n",
        StringView{";"}.join(Array<StringView>{Project.config_names}));
    // CMAKE_SUPPRESS_REGENERATION requires 3.12, but is harmless to set otherwise
    out << "set(CMAKE_SUPPRESS_REGENERATION true)\n";
    out << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)\n";
    out.format("project({})\n", Project.name);

    // Reset CMake's default compiler options
    out << "\n";
    out << "# Initialize compiler options\n";
    out.format("set(CMAKE_C_FLAGS \"\")\n");
    out.format("set(CMAKE_CXX_FLAGS \"\")\n");
    out.format("set(CMAKE_EXE_LINKER_FLAGS \"\")\n");
    out.format("set(CMAKE_STATIC_LINKER_FLAGS \"\")\n");
    out.format("set(CMAKE_SHARED_LINKER_FLAGS \"\")\n");
    for (StringView config_name : Project.config_names) {
        String upper_name = config_name.upper_asc();
        out.format("set(CMAKE_C_FLAGS_{} \"\")\n", upper_name);
        out.format("set(CMAKE_CXX_FLAGS_{} \"\")\n", upper_name);
        out.format("set(CMAKE_EXE_LINKER_FLAGS_{} \"\")\n", upper_name);
        out.format("set(CMAKE_STATIC_LINKER_FLAGS_{} \"\")\n", upper_name);
        out.format("set(CMAKE_SHARED_LINKER_FLAGS_{} \"\")\n", upper_name);
    }

    // Helper functions
    out << R"%(
# Helper functions
macro(AddSourceFiles var_name root)
    foreach(rel_file ${ARGN})
        set(abs_file "${root}/${rel_file}")
        list(APPEND "${var_name}" "${abs_file}")
        get_filename_component(folder "${rel_file}" PATH)
        string(REPLACE / \\ folder "${folder}")
        source_group("${folder}" FILES "${abs_file}")
    endforeach()
endmacro()
)%";

    // Compute all_config_bits
    u64 all_config_bits = Limits<u64>::Max;
    PLY_ASSERT(Project.config_names.num_items() > 0);
    if (Project.config_names.num_items() < 64) {
        all_config_bits = (u64{1} << Project.config_names.num_items()) - 1;
    }

    // For each library, output a CMake target
    for (s32 j = Project.targets.num_items() - 1; j >= 0; j--) {
        const Target* target = Project.targets[j];

        // Skip the CMake target when there are no source files
        if ((target->type != Target::Executable) && target->source_groups.is_empty()) {
            PLY_ASSERT(target->has_build_step_bits == 0);
            continue;
        }

        // FIXME: Ensure uppercase names are unique by adding suffixes if needed (?)
        StringView name = g_labelStorage.view(target->name);
        String upper_name = name.upper_asc();

        // Write source file list
        out << "\n";
        out.format("# {}\n", name);
        for (const SourceGroup& src_group : target->source_groups) {
            out.format("AddSourceFiles({}_SOURCES \"{}\"\n", upper_name,
                       CMakeEscape{PosixPath.from(Path, src_group.abs_path)});
            for (const SourceFile& src_file : src_group.files) {
                if (has_all_bits(src_file.enabled_bits, target->enabled_bits)) {
                    // This source file is enabled in all relevant configs.
                    out.format("    \"{}\"\n",
                               CMakeEscape{PosixPath.from(Path, src_file.rel_path)});
                } else {
                    // Use generator expressions to exclude source file from specific
                    // configs.
                    for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                        if (has_bit_at_index(src_file.enabled_bits, i)) {
                            out.format(
                                "    \"$<$<CONFIG:{}>:{}>\"\n", Project.config_names[i],
                                CMakeEscape{PosixPath.from(Path, src_file.rel_path)});
                        }
                    }
                }
            }
            out << ")\n";
        }
        if (target->type == Target::Executable) {
            out.format("add_executable({} ${{{}_SOURCES}})\n", name, upper_name);
        } else if (target->type == Target::Library ||
                   target->type == Target::ObjectLibrary) {
            if (target->has_build_step_bits != 0) {
                // Note: This library target might still be disabled in specific
                // configs.
                FormatArg a{true};
                out.format("add_library({}{} ${{{}_SOURCES}})\n", name,
                           target->type == Target::ObjectLibrary ? " OBJECT" : "",
                           upper_name);
            } else {
                // This library will never invoke the compiler in any config.
                out.format("add_custom_target({} ${{{}_SOURCES}})\n", name, upper_name);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (target->has_build_step_bits != 0) {
            // Write include directories
            out.format("target_include_directories({} PRIVATE\n", name);
            for (const Option& opt : target->options) {
                if (opt.type == Option::IncludeDir) {
                    if (has_all_bits(opt.enabled_bits, target->enabled_bits)) {
                        // This include directory is enabled in all relevant configs.
                        out.format("    \"{}\"\n",
                                   CMakeEscape{PosixPath.from(Path, opt.key)});
                    } else {
                        // Use generator expressions to exclude include directory from
                        // specific configs.
                        for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                            if (has_bit_at_index(opt.enabled_bits, i)) {
                                out.format("    \"$<$<CONFIG:{}>:{}>\"\n",
                                           Project.config_names[i],
                                           CMakeEscape{PosixPath.from(Path, opt.key)});
                            }
                        }
                    }
                }
            }
            out << ")\n";

            // Write preprocessor definitions
            out.format("target_compile_definitions({} PRIVATE\n", name);
            for (const Option& opt : target->options) {
                if (opt.type == Option::PreprocessorDef) {
                    HybridString def = opt.key;
                    if (opt.value) {
                        def = String::format("{}={}", opt.key, opt.value);
                    }
                    if (has_all_bits(opt.enabled_bits, target->enabled_bits)) {
                        // This include directory is enabled in all relevant configs.
                        out.format("    \"{}\"\n", CMakeEscape{def});
                    } else {
                        // Use generator expressions to exclude include directory from
                        // specific configs.
                        for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                            if (has_bit_at_index(opt.enabled_bits, i)) {
                                out.format("    \"$<$<CONFIG:{}>:{}>\"\n",
                                           Project.config_names[i], CMakeEscape{def});
                            }
                        }
                    }
                }
            }
            out << ")\n";

            // Write compile options
            out.format("set_property(TARGET {} PROPERTY COMPILE_OPTIONS\n", name);
            for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (has_bit_at_index(opt.enabled_bits, i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }

                out.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.config_names[i],
                           escape_cmake_list(Array<StringView>{copts.compile}));
            }
            out << ")\n";
        }

        if (target->type == Target::Executable) {
            // Write libraries to link with
            out.format("target_link_libraries({} PRIVATE\n", name);
            // ...from dependencies
            for (const Dependency& dep : target->dependencies) {
                u64 link_mask = dep.enabled_bits;
                link_mask &= dep.target->has_build_step_bits;
                String target_name = g_labelStorage.view(dep.target->name);
                if (dep.target->type == Target::ObjectLibrary) {
                    target_name = String::format("$<TARGET_OBJECTS:{}>", target_name);
                }
                if (has_all_bits(link_mask, target->enabled_bits)) {
                    out.format("    {}\n", target_name);
                } else if (link_mask != 0) {
                    // Use generator expressions to exclude library from linking in
                    // specific configs.
                    out << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                        if (has_bit_at_index(link_mask, i)) {
                            if (!first) {
                                out << ",";
                            }
                            out.format(CMakeEscape{Project.config_names[i]});
                        }
                        out.format(">:{}>\"\n", target_name);
                        first = false;
                    }
                }
            }
            // ...from external libs
            for (const Option& opt : target->options) {
                if (opt.type != Option::LinkerInput)
                    continue;
                StringView lib_path = opt.key;
                if (has_all_bits(opt.enabled_bits, target->enabled_bits)) {
                    // This lib is enabled for linking in all relevant configs.
                    out.format("    {}\n", CMakeEscape{lib_path});
                } else {
                    // Use generator expressions to exclude lib from linking in specific
                    // configs.
                    out << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                        if (has_bit_at_index(opt.enabled_bits, i)) {
                            if (!first) {
                                out << ",";
                            }
                            out.format(CMakeEscape{Project.config_names[i]});
                        }
                        out.format(">:{}>\"\n", CMakeEscape{lib_path});
                        first = false;
                    }
                }
            }
            out << ")\n";

            // Write link options
            out.format("set_target_properties({} PROPERTIES\n", name);
            for (u32 i = 0; i < Project.config_names.num_items(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (has_bit_at_index(opt.enabled_bits, i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }
                out.format("    LINK_FLAGS_{} \"{}\"\n",
                           Project.config_names[i].upper_asc(),
                           escape_cmake_list(Array<StringView>{copts.link}));
            }
            out << ")\n";
        }
    }

    FileSystem.make_dirs_and_save_text_if_different(path, out.move_to_string());
}

} // namespace build
} // namespace ply
