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

    // Compute everyConfigMask
    u64 everyConfigMask = Limits<u64>::Max;
    PLY_ASSERT(proj->configNames.numItems() > 0);
    if (proj->configNames.numItems() < 64) {
        everyConfigMask = (u64{1} << proj->configNames.numItems()) - 1;
    }

    // Dump each project
    for (const FlatNode* flatNode : flatProj->allFlatNodes) {
        const Node* node = flatNode->node;

        // FIXME: Ensure names are unique
        String upperName = node->name.upperAsc();

        *outs << "\n";
        outs->format("# {}\n", node->name);
        bool needsBuild = false;
        for (const Node::SourceFilePath& sourceFilePath : node->sourceFilePaths) {
            outs->format("AddSourceFiles({}_SOURCES \"{}\"\n", upperName,
                         CMakeEscape{PosixPath::from<NativePath>(sourceFilePath.path)});
            for (WalkTriple& triple : FileSystem::native()->walk(sourceFilePath.path, 0)) {
                for (const WalkTriple::FileInfo& file : triple.files) {
                    if ((file.name.endsWith(".cpp") && !file.name.endsWith(".modules.cpp")) ||
                        file.name.endsWith(".h")) {
                        if (!needsBuild && file.name.endsWith(".cpp")) {
                            needsBuild = true;
                        }
                        String relPath = NativePath::makeRelative(
                            sourceFilePath.path, NativePath::join(triple.dirPath, file.name));
                        outs->format("    \"{}\"\n",
                                     CMakeEscape{PosixPath::from<NativePath>(relPath)});
                    }
                }
            }
            *outs << ")\n";
        }
        if (node->type == Node::Type::Executable) {
            outs->format("add_executable({} ${{{}_SOURCES}})\n", node->name, upperName);
        } else if (node->type == Node::Type::Lib) {
            if (needsBuild) {
                outs->format("add_library({} ${{{}_SOURCES}})\n", node->name, upperName);
            } else {
                outs->format("add_custom_target({} ${{{}_SOURCES}})\n", node->name, upperName);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (needsBuild) {
            auto isBitSet = [](u64 mask, u32 bitIndex) -> bool {
                PLY_ASSERT(bitIndex < 64);
                return (mask & (u64{1} << bitIndex)) != 0;
            };
            auto isEveryConfig = [&](u64 mask) -> bool {
                return (mask & everyConfigMask) == everyConfigMask;
            };
            auto anyConfig = [&](u64 mask) -> bool { return (mask & everyConfigMask) != 0; };

            // Write include directories
            outs->format("target_include_directories({} PRIVATE\n", node->name);
            for (const Node::Option& opt : flatNode->opts) {
                if (opt.opt.type == ToolchainOpt::Type::IncludeDir) {
                    if (opt.activeMask == everyConfigMask) {
                        outs->format("    \"{}\"\n",
                                     CMakeEscape{PosixPath::from<NativePath>(opt.opt.key)});
                    } else {
                        for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                            if (isBitSet(opt.activeMask, i)) {
                                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                                             CMakeEscape{PosixPath::from<NativePath>(opt.opt.key)});
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
                    if (opt.opt.type == ToolchainOpt::Type::Generic) {
                        if (isBitSet(opt.activeMask, i)) {
                            translateGenericOption(flatProj->tc, &compileOpts, opt.opt.key,
                                                   opt.opt.value);
                        }
                    }
                }

                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", proj->configNames[i],
                             escapeCMakeList(Array<StringView>{compileOpts.compileFlags}));
            }
            *outs << ")\n";

            if (node->type == Node::Type::Executable) {
                // Write libraries to link with
                outs->format("target_link_libraries({} PRIVATE\n", node->name);
                for (const Node::LinkerInput& dep : flatNode->dependencies) {
                    if (isEveryConfig(dep.activeMask)) {
                        outs->format("    {}\n", dep.nameOrPath);
                    } else if (anyConfig(dep.activeMask)) {
                        *outs << "    \"$<$<CONFIG:";
                        bool first = true;
                        for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                            if (isBitSet(dep.activeMask, i)) {
                                if (!first) {
                                    *outs << ",";
                                }
                                *outs << CMakeEscape{proj->configNames[i]};
                                outs->format(">:{}>\"\n", CMakeEscape{dep.nameOrPath});
                                first = false;
                            }
                        }
                    }
                }
                *outs << ")\n";

                // Write link options
                outs->format("set_target_properties({} PROPERTIES\n", node->name);
                for (u32 i = 0; i < proj->configNames.numItems(); i++) {
                    CompileOpts compileOpts;
                    for (const Node::Option& opt : flatNode->opts) {
                        if (opt.opt.type == ToolchainOpt::Type::Generic) {
                            if (isBitSet(opt.activeMask, i)) {
                                translateGenericOption(flatProj->tc, &compileOpts, opt.opt.key,
                                                       opt.opt.value);
                            }
                        }
                    }
                    outs->format("    LINK_FLAGS_{} \"{}\"\n", proj->configNames[i].upperAsc(),
                                 escapeCMakeList(Array<StringView>{compileOpts.compileFlags}));
                }
                *outs << ")\n";
            }
        }
    }
}

} // namespace buildSteps
