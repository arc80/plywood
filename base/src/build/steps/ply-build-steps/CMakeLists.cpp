/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
    StringView srcUnits = arg.view;
    while (srcUnits.numBytes > 0) {
        char c = srcUnits.bytes[0];
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
        srcUnits.offsetHead(1);
    }
}

String escapeCMakeList(ArrayView<const StringView> copts) {
    MemOutStream mout;
    bool first = true;
    for (StringView copt : copts) {
        if (!first) {
            mout << ';';
        }
        mout << to_string(CMakeEscape{copt});
        first = false;
    }
    return mout.moveToString();
}

void write_CMakeLists_txt_if_different(StringView path) {
    PLY_ASSERT(Project.name);
    PLY_ASSERT(!Project.configNames.isEmpty());
    PLY_ASSERT(Project.configNames.numItems() < 64);
    PLY_ASSERT(Project.didInheritance);

    MemOutStream out;

    // Write header with list of configuration names
    out << "cmake_minimum_required(VERSION 3.12)\n";
    out.format(
        "set(CMAKE_CONFIGURATION_TYPES \"{}\" CACHE INTERNAL \"Build configs\")\n",
        StringView{";"}.join(Array<StringView>{Project.configNames}));
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
    for (StringView configName : Project.configNames) {
        String upperName = configName.upperAsc();
        out.format("set(CMAKE_C_FLAGS_{} \"\")\n", upperName);
        out.format("set(CMAKE_CXX_FLAGS_{} \"\")\n", upperName);
        out.format("set(CMAKE_EXE_LINKER_FLAGS_{} \"\")\n", upperName);
        out.format("set(CMAKE_STATIC_LINKER_FLAGS_{} \"\")\n", upperName);
        out.format("set(CMAKE_SHARED_LINKER_FLAGS_{} \"\")\n", upperName);
    }

    // Helper functions
    out << R"%(
# Helper functions
macro(AddSourceFiles varName root)
    foreach(relFile ${ARGN})
        set(absFile "${root}/${relFile}")
        list(APPEND "${varName}" "${absFile}")
        get_filename_component(folder "${relFile}" PATH)
        string(REPLACE / \\ folder "${folder}")
        source_group("${folder}" FILES "${absFile}")
    endforeach()
endmacro()
)%";

    // Compute allConfigBits
    u64 allConfigBits = Limits<u64>::Max;
    PLY_ASSERT(Project.configNames.numItems() > 0);
    if (Project.configNames.numItems() < 64) {
        allConfigBits = (u64{1} << Project.configNames.numItems()) - 1;
    }

    // For each library, output a CMake target
    for (s32 j = Project.targets.numItems() - 1; j >= 0; j--) {
        const Target* target = Project.targets[j];

        // Skip the CMake target when there are no source files
        if ((target->type != Target::Executable) && target->sourceGroups.isEmpty()) {
            PLY_ASSERT(target->hasBuildStepBits == 0);
            continue;
        }

        // FIXME: Ensure uppercase names are unique by adding suffixes if needed (?)
        StringView name = g_labelStorage.view(target->name);
        String upperName = name.upperAsc();

        // Write source file list
        out << "\n";
        out.format("# {}\n", name);
        for (const SourceGroup& srcGroup : target->sourceGroups) {
            out.format("AddSourceFiles({}_SOURCES \"{}\"\n", upperName,
                       CMakeEscape{PosixPath.from(Path, srcGroup.absPath)});
            for (const SourceFile& srcFile : srcGroup.files) {
                if (hasAllBits(srcFile.enabledBits, target->enabledBits)) {
                    // This source file is enabled in all relevant configs.
                    out.format("    \"{}\"\n",
                               CMakeEscape{PosixPath.from(Path, srcFile.relPath)});
                } else {
                    // Use generator expressions to exclude source file from specific
                    // configs.
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (hasBitAtIndex(srcFile.enabledBits, i)) {
                            out.format(
                                "    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                                CMakeEscape{PosixPath.from(Path, srcFile.relPath)});
                        }
                    }
                }
            }
            out << ")\n";
        }
        if (target->type == Target::Executable) {
            out.format("add_executable({} ${{{}_SOURCES}})\n", name, upperName);
        } else if (target->type == Target::Library ||
                   target->type == Target::ObjectLibrary) {
            if (target->hasBuildStepBits != 0) {
                // Note: This library target might still be disabled in specific
                // configs.
                FormatArg a{true};
                out.format("add_library({}{} ${{{}_SOURCES}})\n", name,
                           target->type == Target::ObjectLibrary ? " OBJECT" : "",
                           upperName);
            } else {
                // This library will never invoke the compiler in any config.
                out.format("add_custom_target({} ${{{}_SOURCES}})\n", name, upperName);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (target->hasBuildStepBits != 0) {
            // Write include directories
            out.format("target_include_directories({} PRIVATE\n", name);
            for (const Option& opt : target->options) {
                if (opt.type == Option::IncludeDir) {
                    if (hasAllBits(opt.enabledBits, target->enabledBits)) {
                        // This include directory is enabled in all relevant configs.
                        out.format("    \"{}\"\n",
                                   CMakeEscape{PosixPath.from(Path, opt.key)});
                    } else {
                        // Use generator expressions to exclude include directory from
                        // specific configs.
                        for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                            if (hasBitAtIndex(opt.enabledBits, i)) {
                                out.format("    \"$<$<CONFIG:{}>:{}>\"\n",
                                           Project.configNames[i],
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
                    if (hasAllBits(opt.enabledBits, target->enabledBits)) {
                        // This include directory is enabled in all relevant configs.
                        out.format("    \"{}\"\n", CMakeEscape{def});
                    } else {
                        // Use generator expressions to exclude include directory from
                        // specific configs.
                        for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                            if (hasBitAtIndex(opt.enabledBits, i)) {
                                out.format("    \"$<$<CONFIG:{}>:{}>\"\n",
                                           Project.configNames[i], CMakeEscape{def});
                            }
                        }
                    }
                }
            }
            out << ")\n";

            // Write compile options
            out.format("set_property(TARGET {} PROPERTY COMPILE_OPTIONS\n", name);
            for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (hasBitAtIndex(opt.enabledBits, i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }

                out.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                           escapeCMakeList(Array<StringView>{copts.compile}));
            }
            out << ")\n";
        }

        if (target->type == Target::Executable) {
            // Write libraries to link with
            out.format("target_link_libraries({} PRIVATE\n", name);
            // ...from dependencies
            for (const Dependency& dep : target->dependencies) {
                u64 linkMask = dep.enabledBits;
                linkMask &= dep.target->hasBuildStepBits;
                String target_name = g_labelStorage.view(dep.target->name);
                if (dep.target->type == Target::ObjectLibrary) {
                    target_name = String::format("$<TARGET_OBJECTS:{}>", target_name);
                }
                if (hasAllBits(linkMask, target->enabledBits)) {
                    out.format("    {}\n", target_name);
                } else if (linkMask != 0) {
                    // Use generator expressions to exclude library from linking in
                    // specific configs.
                    out << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (hasBitAtIndex(linkMask, i)) {
                            if (!first) {
                                out << ",";
                            }
                            out.format(CMakeEscape{Project.configNames[i]});
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
                StringView libPath = opt.key;
                if (hasAllBits(opt.enabledBits, target->enabledBits)) {
                    // This lib is enabled for linking in all relevant configs.
                    out.format("    {}\n", CMakeEscape{libPath});
                } else {
                    // Use generator expressions to exclude lib from linking in specific
                    // configs.
                    out << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (hasBitAtIndex(opt.enabledBits, i)) {
                            if (!first) {
                                out << ",";
                            }
                            out.format(CMakeEscape{Project.configNames[i]});
                        }
                        out.format(">:{}>\"\n", CMakeEscape{libPath});
                        first = false;
                    }
                }
            }
            out << ")\n";

            // Write link options
            out.format("set_target_properties({} PROPERTIES\n", name);
            for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (hasBitAtIndex(opt.enabledBits, i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }
                out.format("    LINK_FLAGS_{} \"{}\"\n",
                           Project.configNames[i].upperAsc(),
                           escapeCMakeList(Array<StringView>{copts.link}));
            }
            out << ")\n";
        }
    }

    FileSystem.makeDirsAndSaveTextIfDifferent(path, out.moveToString());
}

} // namespace build
} // namespace ply
