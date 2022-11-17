/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/FlatProject.h>
#include <ply-runtime/algorithm/Map.h>

namespace buildSteps {

struct CMakeEscape {
    StringView view;
    PLY_INLINE CMakeEscape(StringView view) : view{view} {
    }
};

} // namespace buildSteps

namespace ply {
namespace fmt {
template <>
struct TypePrinter<buildSteps::CMakeEscape> {
    static void print(OutStream* outs, const buildSteps::CMakeEscape& value) {
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

namespace buildSteps {

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

void writeCMakeLists(OutStream* outs, const FlatProject* flatProj) {
    const Project* proj = flatProj->proj;

    // Write header with list of configuration names
    *outs << "cmake_minimum_required(VERSION 3.0)\n";
    outs->format("set(CMAKE_CONFIGURATION_TYPES \"{}\" CACHE INTERNAL \"Build configs\")\n",
                 StringView{";"}.join(Array<StringView>{proj->configNames}));
    // CMAKE_SUPPRESS_REGENERATION requires 3.12, but is harmless to set otherwise
    *outs << "set(CMAKE_SUPPRESS_REGENERATION true)\n";
    *outs << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)\n";
    outs->format("project({})\n", proj->name);

    // Reset CMake's default compiler options
    *outs << "\n";
    *outs << "# Initialize compiler options\n";
    outs->format("set(CMAKE_C_FLAGS \"\")\n");
    outs->format("set(CMAKE_CXX_FLAGS \"\")\n");
    outs->format("set(CMAKE_EXE_LINKER_FLAGS \"\")\n");
    outs->format("set(CMAKE_STATIC_LINKER_FLAGS \"\")\n");
    outs->format("set(CMAKE_SHARED_LINKER_FLAGS \"\")\n");
    for (StringView configName : proj->configNames) {
        String upperName = configName.upperAsc();
        outs->format("set(CMAKE_C_FLAGS_{} \"\")\n", upperName);
        outs->format("set(CMAKE_CXX_FLAGS_{} \"\")\n", upperName);
        outs->format("set(CMAKE_EXE_LINKER_FLAGS_{} \"\")\n", upperName);
        outs->format("set(CMAKE_STATIC_LINKER_FLAGS_{} \"\")\n", upperName);
        outs->format("set(CMAKE_SHARED_LINKER_FLAGS_{} \"\")\n", upperName);
    }

    // Helper functions
    *outs << R"%(
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
    PLY_ASSERT(proj->configNames.numItems() > 0);
    if (proj->configNames.numItems() < 64) {
        allConfigBits = (u64{1} << proj->configNames.numItems()) - 1;
    }

    // For each module, output a CMake target
    for (const FlatNode* flatNode : flatProj->allFlatNodes) {
        const Node* node = flatNode->node;

        // Skip the CMake target when there are no source files
        if ((node->type == Node::Type::Lib) && node->sourceGroups.isEmpty()) {
            PLY_ASSERT(!node->hasBuildStep.hasAnyBit());
            continue;
        }

        // FIXME: Ensure uppercase names are unique by adding suffixes if needed (?)
        String upperName = node->name.upperAsc();

        // Write source file list
        *outs << "\n";
        outs->format("# {}\n", node->name);
        for (const Node::SourceGroup& srcGroup : node->sourceGroups) {
            outs->format("AddSourceFiles({}_SOURCES \"{}\"\n", upperName,
                         CMakeEscape{PosixPath::from<NativePath>(srcGroup.absPath)});
            for (const Node::SourceFile& srcFile : srcGroup.files) {
                if (srcFile.enabled.hasAllBitsIn(node->enabled.bits)) {
                    // This source file is enabled in all relevant configs.
                    outs->format("    \"{}\"\n",
                                 CMakeEscape{PosixPath::from<NativePath>(srcFile.relPath)});
                } else {
                    // Use generator expressions to exclude source file from specific configs.
                    for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                        if (srcFile.enabled.hasBitAtIndex(i)) {
                            outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                                         CMakeEscape{PosixPath::from<NativePath>(srcFile.relPath)});
                        }
                    }
                }
            }
            *outs << ")\n";
        }
        if (node->type == Node::Type::Executable) {
            outs->format("add_executable({} ${{{}_SOURCES}})\n", node->name, upperName);
        } else if (node->type == Node::Type::Lib) {
            if (node->hasBuildStep.hasAnyBit()) {
                // Note: This library target might still be disabled in specific configs.
                outs->format("add_library({} ${{{}_SOURCES}})\n", node->name, upperName);
            } else {
                // This module will never invoke the compiler in any config.
                outs->format("add_custom_target({} ${{{}_SOURCES}})\n", node->name, upperName);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (node->hasBuildStep.hasAnyBit()) {
            // Write include directories
            outs->format("target_include_directories({} PRIVATE\n", node->name);
            for (const Node::Option& opt : flatNode->opts) {
                if (opt.opt.type == ToolChainOpt::Type::IncludeDir) {
                    if (opt.enabled.hasAllBitsIn(node->enabled.bits)) {
                        // This include directory is enabled in all relevant configs.
                        outs->format("    \"{}\"\n",
                                     CMakeEscape{PosixPath::from<NativePath>(opt.opt.key)});
                    } else {
                        // Use generator expressions to exclude include directory from specific
                        // configs.
                        for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                            if (opt.enabled.hasBitAtIndex(i)) {
                                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                                             CMakeEscape{PosixPath::from<NativePath>(opt.opt.key)});
                            }
                        }
                    }
                }
            }
            *outs << ")\n";

            // Write preprocessor definitions
            outs->format("target_compile_definitions({} PRIVATE\n", node->name);
            for (const Node::Option& opt : flatNode->opts) {
                if (opt.opt.type == ToolChainOpt::Type::PreprocessorDef) {
                    HybridString def = opt.opt.key;
                    if (opt.opt.value) {
                        def = String::format("{}={}", opt.opt.key, opt.opt.value);
                    }
                    if (opt.enabled.hasAllBitsIn(node->enabled.bits)) {
                        // This include directory is enabled in all relevant configs.
                        outs->format("    \"{}\"\n", CMakeEscape{def});
                    } else {
                        // Use generator expressions to exclude include directory from specific
                        // configs.
                        for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                            if (opt.enabled.hasBitAtIndex(i)) {
                                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                                             CMakeEscape{def});
                            }
                        }
                    }
                }
            }
            *outs << ")\n";

            // Write compile options
            outs->format("set_property(TARGET {} PROPERTY COMPILE_OPTIONS\n", node->name);
            for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                CompileOpts compileOpts;
                for (const Node::Option& opt : flatNode->opts) {
                    if (opt.opt.type == ToolChainOpt::Type::Generic) {
                        if (opt.enabled.hasBitAtIndex(i)) {
                            flatProj->proj->tc->translateOption(&compileOpts, opt.opt);
                        }
                    }
                }

                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                             escapeCMakeList(Array<StringView>{compileOpts.compileFlags}));
            }
            *outs << ")\n";
        }

        if (node->type == Node::Type::Executable) {
            // Write libraries to link with
            outs->format("target_link_libraries({} PRIVATE\n", node->name);
            // ...from dependencies
            for (const FlatNode::Dependency& dep : flatNode->dependencies) {
                ConfigMask linkMask = dep.enabled;
                linkMask.bits &= dep.dep->node->hasBuildStep.bits;
                if (linkMask.hasAllBitsIn(node->enabled.bits)) {
                    // This module is enabled for linking in all relevant configs.
                    outs->format("    {}\n", dep.dep->node->name);
                } else if (linkMask.hasAnyBit()) {
                    // Use generator expressions to exclude module from linking in specific configs.
                    *outs << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                        if (linkMask.hasBitAtIndex(i)) {
                            if (!first) {
                                *outs << ",";
                            }
                            *outs << CMakeEscape{proj->configNames[i]};
                        }
                        outs->format(">:{}>\"\n", dep.dep->node->name);
                        first = false;
                    }
                }
            }
            // ...from external libs
            for (const Node::LinkerInput& input : flatNode->prebuiltLibs) {
                if (input.enabled.hasAllBitsIn(node->enabled.bits)) {
                    // This lib is enabled for linking in all relevant configs.
                    outs->format("    {}\n", CMakeEscape{input.path});
                } else {
                    // Use generator expressions to exclude lib from linking in specific configs.
                    *outs << "    \"$<$<CONFIG:";
                    bool first = true;
                    for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                        if (input.enabled.hasBitAtIndex(i)) {
                            if (!first) {
                                *outs << ",";
                            }
                            *outs << CMakeEscape{proj->configNames[i]};
                        }
                        outs->format(">:{}>\"\n", CMakeEscape{input.path});
                        first = false;
                    }
                }
            }
            *outs << ")\n";

            // Write link options
            outs->format("set_target_properties({} PROPERTIES\n", node->name);
            for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                CompileOpts compileOpts;
                for (const Node::Option& opt : flatNode->opts) {
                    if (opt.opt.type == ToolChainOpt::Type::Generic) {
                        if (opt.enabled.hasBitAtIndex(i)) {
                            flatProj->proj->tc->translateOption(&compileOpts, opt.opt);
                        }
                    }
                }
                outs->format("    LINK_FLAGS_{} \"{}\"\n", proj->configNames[i].upperAsc(),
                             escapeCMakeList(Array<StringView>{compileOpts.linkFlags}));
            }
            *outs << ")\n";
        }
    }
}

} // namespace buildSteps
