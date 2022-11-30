/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/Project.h>
#include <ply-runtime/algorithm/Map.h>

namespace ply {
namespace build2 {

struct CMakeEscape {
    StringView view;
    PLY_INLINE CMakeEscape(StringView view) : view{view} {
    }
};

} // namespace build2
} // namespace ply

namespace ply {
namespace fmt {
template <>
struct TypePrinter<build2::CMakeEscape> {
    static void print(OutStream* outs, const build2::CMakeEscape& value) {
        StringView srcUnits = value.view;
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
                    *outs << '\\';
                    break;
                }
                default: {
                    break;
                }
            }
            *outs << c;
            srcUnits.offsetHead(1);
        }
    }
};
} // namespace fmt
} // namespace ply

namespace ply {
namespace build2 {

String escapeCMakeList(ArrayView<const StringView> copts) {
    MemOutStream mout;
    bool first = true;
    for (StringView copt : copts) {
        if (!first) {
            mout << ';';
        }
        mout << CMakeEscape{copt};
        first = false;
    }
    return mout.moveToString();
}

void write_CMakeLists_txt_if_different(StringView path) {
    PLY_ASSERT(Project.name);
    PLY_ASSERT(!Project.configNames.isEmpty());
    PLY_ASSERT(Project.configNames.numItems() < 64);
    PLY_ASSERT(Project.didInheritance);

    MemOutStream outs;

    // Write header with list of configuration names
    outs << "cmake_minimum_required(VERSION 3.0)\n";
    outs.format("set(CMAKE_CONFIGURATION_TYPES \"{}\" CACHE INTERNAL \"Build configs\")\n",
                StringView{";"}.join(Array<StringView>{Project.configNames}));
    // CMAKE_SUPPRESS_REGENERATION requires 3.12, but is harmless to set otherwise
    outs << "set(CMAKE_SUPPRESS_REGENERATION true)\n";
    outs << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)\n";
    outs.format("project({})\n", Project.name);

    // Reset CMake's default compiler options
    outs << "\n";
    outs << "# Initialize compiler options\n";
    outs.format("set(CMAKE_C_FLAGS \"\")\n");
    outs.format("set(CMAKE_CXX_FLAGS \"\")\n");
    outs.format("set(CMAKE_EXE_LINKER_FLAGS \"\")\n");
    outs.format("set(CMAKE_STATIC_LINKER_FLAGS \"\")\n");
    outs.format("set(CMAKE_SHARED_LINKER_FLAGS \"\")\n");
    for (StringView configName : Project.configNames) {
        String upperName = configName.upperAsc();
        outs.format("set(CMAKE_C_FLAGS_{} \"\")\n", upperName);
        outs.format("set(CMAKE_CXX_FLAGS_{} \"\")\n", upperName);
        outs.format("set(CMAKE_EXE_LINKER_FLAGS_{} \"\")\n", upperName);
        outs.format("set(CMAKE_STATIC_LINKER_FLAGS_{} \"\")\n", upperName);
        outs.format("set(CMAKE_SHARED_LINKER_FLAGS_{} \"\")\n", upperName);
    }

    // Helper functions
    outs << R"%(
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

    // For each module, output a CMake target
    for (s32 j = Project.targets.numItems() - 1; j >= 0; j--) {
        const Target* target = Project.targets[j];

        // Skip the CMake target when there are no source files
        if ((target->type == Target::Library) && target->sourceGroups.isEmpty()) {
            PLY_ASSERT(!target->hasBuildStep.hasAnyBit());
            continue;
        }

        // FIXME: Ensure uppercase names are unique by adding suffixes if needed (?)
        StringView name = g_labelStorage.view(target->name);
        String upperName = name.upperAsc();

        // Write source file list
        outs << "\n";
        outs.format("# {}\n", name);
        for (const SourceGroup& srcGroup : target->sourceGroups) {
            outs.format("AddSourceFiles({}_SOURCES \"{}\"\n", upperName,
                        CMakeEscape{PosixPath::from<NativePath>(srcGroup.absPath)});
            for (const SourceFile& srcFile : srcGroup.files) {
                if (srcFile.enabled.hasAllBitsIn(target->enabled.bits)) {
                    // This source file is enabled in all relevant configs.
                    outs.format("    \"{}\"\n",
                                CMakeEscape{PosixPath::from<NativePath>(srcFile.relPath)});
                } else {
                    // Use generator expressions to exclude source file from specific configs.
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (srcFile.enabled.hasBitAtIndex(i)) {
                            outs.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                                        CMakeEscape{PosixPath::from<NativePath>(srcFile.relPath)});
                        }
                    }
                }
            }
            outs << ")\n";
        }
        if (target->type == Target::Executable) {
            outs.format("add_executable({} ${{{}_SOURCES}})\n", name, upperName);
        } else if (target->type == Target::Library) {
            if (target->hasBuildStep.hasAnyBit()) {
                // Note: This library target might still be disabled in specific configs.
                outs.format("add_library({} ${{{}_SOURCES}})\n", name, upperName);
            } else {
                // This module will never invoke the compiler in any config.
                outs.format("add_custom_target({} ${{{}_SOURCES}})\n", name, upperName);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (target->hasBuildStep.hasAnyBit()) {
            // Write include directories
            outs.format("target_include_directories({} PRIVATE\n", name);
            for (const Option& opt : target->options) {
                if (opt.type == Option::IncludeDir) {
                    if (opt.enabled.hasAllBitsIn(target->enabled.bits)) {
                        // This include directory is enabled in all relevant configs.
                        outs.format("    \"{}\"\n",
                                    CMakeEscape{PosixPath::from<NativePath>(opt.key)});
                    } else {
                        // Use generator expressions to exclude include directory from specific
                        // configs.
                        for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                            if (opt.enabled.hasBitAtIndex(i)) {
                                outs.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                                            CMakeEscape{PosixPath::from<NativePath>(opt.key)});
                            }
                        }
                    }
                }
            }
            outs << ")\n";

            // Write preprocessor definitions
            outs.format("target_compile_definitions({} PRIVATE\n", name);
            for (const Option& opt : target->options) {
                if (opt.type == Option::PreprocessorDef) {
                    HybridString def = opt.key;
                    if (opt.value) {
                        def = String::format("{}={}", opt.key, opt.value);
                    }
                    if (opt.enabled.hasAllBitsIn(target->enabled.bits)) {
                        // This include directory is enabled in all relevant configs.
                        outs.format("    \"{}\"\n", CMakeEscape{def});
                    } else {
                        // Use generator expressions to exclude include directory from specific
                        // configs.
                        for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                            if (opt.enabled.hasBitAtIndex(i)) {
                                outs.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                                            CMakeEscape{def});
                            }
                        }
                    }
                }
            }
            outs << ")\n";

            // Write compile options
            outs.format("set_property(TARGET {} PROPERTY COMPILE_OPTIONS\n", name);
            for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (opt.enabled.hasBitAtIndex(i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }

                outs.format("    \"$<$<CONFIG:{}>:{}>\"\n", Project.configNames[i],
                            escapeCMakeList(Array<StringView>{copts.compile}));
            }
            outs << ")\n";
        }

        if (target->type == Target::Executable) {
            // Write libraries to link with
            outs.format("target_link_libraries({} PRIVATE\n", name);
            // ...from dependencies
            for (const Dependency& dep : target->dependencies) {
                ConfigMask linkMask = dep.enabled;
                linkMask.bits &= dep.target->hasBuildStep.bits;
                if (linkMask.hasAllBitsIn(target->enabled.bits)) {
                    // This module is enabled for linking in all relevant configs.
                    outs.format("    {}\n", g_labelStorage.view(dep.target->name));
                } else if (linkMask.hasAnyBit()) {
                    // Use generator expressions to exclude module from linking in specific configs.
                    outs << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (linkMask.hasBitAtIndex(i)) {
                            if (!first) {
                                outs << ",";
                            }
                            outs << CMakeEscape{Project.configNames[i]};
                        }
                        outs.format(">:{}>\"\n", g_labelStorage.view(dep.target->name));
                        first = false;
                    }
                }
            }
            // ...from external libs
            for (const Option& opt : target->options) {
                if (opt.type != Option::LinkerInput)
                    continue;
                StringView libPath = opt.key;
                if (opt.enabled.hasAllBitsIn(target->enabled.bits)) {
                    // This lib is enabled for linking in all relevant configs.
                    outs.format("    {}\n", CMakeEscape{libPath});
                } else {
                    // Use generator expressions to exclude lib from linking in specific configs.
                    outs << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                        if (opt.enabled.hasBitAtIndex(i)) {
                            if (!first) {
                                outs << ",";
                            }
                            outs << CMakeEscape{Project.configNames[i]};
                        }
                        outs.format(">:{}>\"\n", CMakeEscape{libPath});
                        first = false;
                    }
                }
            }
            outs << ")\n";

            // Write link options
            outs.format("set_target_properties({} PROPERTIES\n", name);
            for (u32 i = 0; i < Project.configNames.numItems(); i++) {
                CompilerSpecificOptions copts;
                for (const Option& opt : target->options) {
                    if (opt.type == Option::Generic) {
                        if (opt.enabled.hasBitAtIndex(i)) {
                            translate_toolchain_option(&copts, opt);
                        }
                    }
                }
                outs.format("    LINK_FLAGS_{} \"{}\"\n", Project.configNames[i].upperAsc(),
                            escapeCMakeList(Array<StringView>{copts.link}));
            }
            outs << ")\n";
        }
    }

    FileSystem::native()->makeDirsAndSaveTextIfDifferent(path, outs.moveToString(),
                                                         TextFormat::platformPreference());
}

} // namespace build2
} // namespace ply
