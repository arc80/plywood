/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/BuildSteps.h>
#include <ply-runtime/algorithm/Find.h>

namespace buildSteps {

//------------------------------------------------------------------------------

struct FinalOption {
    u32 type;
    String key;
    Array<String> values; // if numItems() == 1, it's an every-config option
};

void merge(Array<FinalOption>& dstOpts, ArrayView<const ToolchainOpt> srcOpts, s32 configIdx,
           u32 numConfigs) {
    for (const ToolchainOpt& srcOpt : srcOpts) {
        for (u32 i = 0; i < dstOpts.numItems(); i++) {
            FinalOption& dstOpt = dstOpts[i];
            if (srcOpt.type == dstOpt.type && srcOpt.key == dstOpt.key) {
                if (configIdx < 0) {
                    dstOpt.values.resize(1);
                    dstOpt.values[0] = srcOpt.value;
                } else {
                    // Expand every-config option if needed
                    if (dstOpt.values.numItems() < numConfigs) {
                        PLY_ASSERT(dstOpt.values.numItems() == 1);
                        StringView value = dstOpt.values[0];
                        while (dstOpt.values.numItems() < numConfigs) {
                            dstOpt.values.append(value);
                        }
                    }
                    dstOpt.values[configIdx] = srcOpt.value;
                }
                goto found;
            }
        }
        {
            // Not found
            FinalOption& dstOpt = dstOpts.append();
            dstOpt.type = srcOpt.type;
            dstOpt.key = srcOpt.key;
            if (configIdx < 0) {
                dstOpt.values.resize(1);
                dstOpt.values[0] = srcOpt.value;
            } else {
                while (dstOpt.values.numItems() < numConfigs) {
                    dstOpt.values.append(ToolchainOpt::Erased);
                }
                dstOpt.values[configIdx] = srcOpt.value;
            }
        }
    found:;
    }
}

//------------------------------------------------------------------------------

template <typename Callback>
PLY_INLINE void iterOverOptions(ArrayView<const FinalOption> opts, u32 configIdx,
                                const Callback& cb) {
    for (const FinalOption& opt : opts) {
        StringView value = opt.values[opt.values.numItems() == 1 ? 0 : configIdx];
        if (value == ToolchainOpt::Erased)
            continue;
        cb(opt.type, opt.key, value);
    }
}

struct ToolChain {
    struct ValueToAdd {
        StringView value;
        ArrayView<const StringView> compile;
        ArrayView<const StringView> link;
    };

    struct Value {
        String value;
        Array<String> compile;
        Array<String> link;

        Value(const ValueToAdd& from) : value{from.value}, compile{from.compile}, link{from.link} {
        }
    };

    struct Traits {
        using Key = StringView;
        struct Item {
            String key;
            Array<Value> values;
            Item(StringView key) : key{key} {
            }
        };
        static bool match(const Item& item, StringView key) {
            return item.key == key;
        }
    };

    HashMap<Traits> allGenericOptions;

    void addOption(StringView key, ArrayView<const ValueToAdd> values) {
        auto cursor = this->allGenericOptions.insertOrFind(key);
        PLY_ASSERT(!cursor.wasFound());
        cursor->values = values;
    }
};

ToolChain getMSVC() {
    ToolChain tc;
    tc.addOption("DebugInfo", {{{}, {"/Zi"}, {"/DEBUG"}}});
    tc.addOption("LanguageLevel", {{"c++14", {"/std:c++14"}}, {"c++17", {"/std:c++17"}}});
    return tc;
}

ToolChain getGCC() {
    ToolChain tc;
    tc.addOption("DebugInfo", {{{}, {"-g"}}});
    tc.addOption("LanguageLevel", {{"c++14", {"-std=c++14"}}, {"c++17", {"-std=c++17"}}});
    return tc;
}

struct CompileOpts {
    Array<HybridString> compile;
    Array<HybridString> link;
};

CompileOpts extractCompilerOpts(const ToolChain& tc, ArrayView<const FinalOption> opts,
                                u32 configIdx) {
    CompileOpts copts;
    iterOverOptions(opts, configIdx, [&](u32 type, StringView key, StringView value) {
        if (type == ToolchainOpt::Generic) {
            auto cursor = tc.allGenericOptions.find(key);
            PLY_ASSERT(cursor.wasFound()); // FIXME: Handle gracefully
            if (cursor->values.numItems() == 1 && cursor->values[0].value.isEmpty()) {
                copts.compile.extend(cursor->values[0].compile);
                copts.link.extend(cursor->values[0].link);
            } else {
                s32 idx =
                    find(cursor->values, [&](const auto& cand) { return cand.value == value; });
                PLY_ASSERT(idx >= 0); // FIXME: Handle gracefully
                copts.compile.extend(cursor->values[idx].compile);
                copts.link.extend(cursor->values[idx].link);
            }
        }
    });
    return copts;
}

//------------------------------------------------------------------------------

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

const String ToolchainOpt::Erased = StringView{"\0", 1};

void inherit(Array<ToolchainOpt>& dstOpts, ArrayView<const ToolchainOpt> srcOpts,
             Visibility dstVis) {
    for (const ToolchainOpt& srcOpt : srcOpts) {
        if (srcOpt.vis == Visibility::Public) {
            for (u32 i = 0; i < dstOpts.numItems(); i++) {
                ToolchainOpt& dstOpt = dstOpts[i];
                if (srcOpt.type == dstOpt.type && srcOpt.key == dstOpt.key) {
                    if (dstOpt.value != srcOpt.value) {
                        dstOpt.value = srcOpt.value;
                        // An option override shouldn't turn a public option private
                        // FIXME: Handle this more gracefully
                        PLY_ASSERT(
                            !(dstOpt.vis == Visibility::Public && dstVis == Visibility::Private));
                    }
                    if (dstVis == Visibility::Public) {
                        dstOpt.vis = Visibility::Public;
                    }
                    goto found;
                }
            }
            // Not found
            {
                ToolchainOpt& dstOpt = dstOpts.append(srcOpt);
                dstOpt.vis = dstVis;
            }
        found:;
        }
    }
}

struct MetaNode {
    struct Options {
        Array<ToolchainOpt> tcOpts;
        Array<MetaNode*> dependencies;
    };

    const Node* node;
    bool initialized = false;
    bool needsBuild = false;
    Options opts;
    Array<Options> configs;
};

struct MetaProject {
    struct Traits {
        using Key = const Node*;
        using Item = MetaNode*;
        static PLY_INLINE bool match(const MetaNode* item, const Node* key) {
            return item->node == key;
        }
    };

    ToolChain tc;
    const Project* proj = nullptr;
    HashMap<Traits> map;
    Array<Owned<MetaNode>> allMetas; // dependencies listed before dependents
};

MetaNode* expand(MetaProject* metaProj, const Node* origNode) {
    Owned<MetaNode> meta;
    {
        auto cursor = metaProj->map.insertOrFind(origNode);
        if (cursor.wasFound()) {
            // FIXME: Handle more elegantly
            PLY_ASSERT((*cursor)->initialized); // Circular dependency
            return *cursor;
        }
        meta = new MetaNode;
        PLY_ASSERT(*cursor == nullptr);
        *cursor = meta;
    }

    meta->node = origNode;
    meta->opts.tcOpts = origNode->opts.tcOpts;
    meta->configs.resize(metaProj->proj->configs.numItems());
    for (const Node::Config& nodeConfig : origNode->configs) {
        s32 configIdx = find(metaProj->proj->configs, [&](const auto& projConfig) {
            return projConfig.name == nodeConfig.name;
        });
        if (configIdx >= 0) {
            meta->configs[configIdx].tcOpts = nodeConfig.opts.tcOpts;
        } else {
            // Node config not found in project
            PLY_ASSERT(0); // FIXME: Handle gracefully
        }
    }
    // FIXME: Support config-specific dependencies
    for (const auto& origDepTuple : origNode->opts.dependencies) {
        Visibility v0 = origDepTuple.first;
        const Node* origDep = origDepTuple.second;
        MetaNode* metaDep = expand(metaProj, origDep);
        inherit(meta->opts.tcOpts, metaDep->opts.tcOpts, v0);
        for (u32 configIdx = 0; configIdx < metaProj->proj->configs.numItems(); configIdx++) {
            inherit(meta->configs[configIdx].tcOpts, metaDep->configs[configIdx].tcOpts, v0);
        }
        auto addDep = [](MetaNode* toMeta, MetaNode* dep) {
            if (find(toMeta->opts.dependencies, dep) < 0) {
                toMeta->opts.dependencies.append(dep);
            }
        };
        for (MetaNode* mdd : metaDep->opts.dependencies) {
            addDep(meta, mdd);
        }
        addDep(meta, metaDep);
    }

    for (const auto& sourceFiles : meta->node->sourceFiles) {
        for (StringView relPath : sourceFiles.relFiles) {
            String ext = NativePath::splitExt(relPath).second.lowerAsc();
            if (find<StringView>({".cpp", ".c"}, ext) >= 0) {
                meta->needsBuild = true;
                break;
            }
        }
    }

    meta->initialized = true;
    MetaNode* result = meta;
    metaProj->allMetas.append(std::move(meta));
    return result;
}

Owned<MetaProject> expand(const Project* proj) {
    Owned<MetaProject> metaProj = new MetaProject;
#if PLY_TARGET_WIN32
    metaProj->tc = getMSVC();
#else
    metaProj->tc = getGCC();
#endif
    metaProj->proj = proj;

    for (const Node* root : proj->rootNodes) {
        expand(metaProj, root);
    }

    return metaProj;
}

String escapeCMakeList(ArrayView<const HybridString> copts) {
    MemOutStream mout;
    for (u32 i : range(copts.numItems)) {
        if (i > 0) {
            mout << ';';
        }
        mout << CMakeEscape{copts[i]};
    }
    return mout.moveToString();
}

void writeCMakeLists(OutStream* outs, const MetaProject* metaProj) {
    const Project* proj = metaProj->proj;
    *outs << "cmake_minimum_required(VERSION 3.0)\n";
    {
        Array<StringView> configs;
        for (const Node::Config& config : metaProj->proj->configs) {
            configs.append(config.name);
        }
        outs->format("set(CMAKE_CONFIGURATION_TYPES \"{}\" CACHE INTERNAL \"Build configs\")\n",
                     StringView{";"}.join(configs));
    }
    // CMAKE_SUPPRESS_REGENERATION requires 3.12, but is harmless to set otherwise
    *outs << "set(CMAKE_SUPPRESS_REGENERATION true)\n";
    *outs << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)\n";
    outs->format("project({})\n", proj->name);

    // Initialize compiler options
    *outs << "\n";
    *outs << "# Initialize compiler options\n";
    outs->format("set(CMAKE_C_FLAGS \"\")\n");
    for (u32 i = 0; i < proj->configs.numItems(); i++) {
        outs->format("set(CMAKE_C_FLAGS_{} \"\")\n", proj->configs[i].name.upperAsc());
    }
    outs->format("set(CMAKE_CXX_FLAGS \"\")\n");
    for (u32 i = 0; i < proj->configs.numItems(); i++) {
        outs->format("set(CMAKE_CXX_FLAGS_{} \"\")\n", proj->configs[i].name.upperAsc());
    }
    outs->format("set(CMAKE_EXE_LINKER_FLAGS \"\")\n");
    for (u32 i = 0; i < proj->configs.numItems(); i++) {
        outs->format("set(CMAKE_EXE_LINKER_FLAGS_{} \"\")\n", proj->configs[i].name.upperAsc());
    }
    outs->format("set(CMAKE_STATIC_LINKER_FLAGS \"\")\n");
    for (u32 i = 0; i < proj->configs.numItems(); i++) {
        outs->format("set(CMAKE_STATIC_LINKER_FLAGS_{} \"\")\n", proj->configs[i].name.upperAsc());
    }
    outs->format("set(CMAKE_SHARED_LINKER_FLAGS \"\")\n");
    for (u32 i = 0; i < proj->configs.numItems(); i++) {
        outs->format("set(CMAKE_SHARED_LINKER_FLAGS_{} \"\")\n", proj->configs[i].name.upperAsc());
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

    // Dump each project
    for (const MetaNode* meta : metaProj->allMetas) {
        // FIXME: Ensure names are unique
        String upperName = meta->node->name.upperAsc();

        *outs << "\n";
        outs->format("# {}\n", meta->node->name);
        for (const Node::SourceFiles& sourceFiles : meta->node->sourceFiles) {
            outs->format("AddSourceFiles({}_SOURCES \"{}\"\n", upperName,
                         CMakeEscape{PosixPath::from<NativePath>(sourceFiles.root)});
            for (StringView relPath : sourceFiles.relFiles) {
                outs->format("    \"{}\"\n", CMakeEscape{PosixPath::from<NativePath>(relPath)});
            }
            *outs << ")\n";
        }
        if (meta->node->type == Node::EXE) {
            outs->format("add_executable({} ${{{}_SOURCES}})\n", meta->node->name, upperName);
        } else if (meta->node->type == Node::Lib) {
            if (meta->needsBuild) {
                outs->format("add_library({} ${{{}_SOURCES}})\n", meta->node->name, upperName);
            } else {
                outs->format("add_custom_target({} ${{{}_SOURCES}})\n", meta->node->name,
                             upperName);
            }
        } else {
            PLY_ASSERT(0);
        }

        if (meta->needsBuild) {
            // Expand toolchain options
            Array<FinalOption> finalOpts;
            {
                u32 numConfigs = metaProj->proj->configs.numItems();
                merge(finalOpts, metaProj->proj->opts.tcOpts, -1, numConfigs);
                for (u32 configIdx = 0; configIdx < numConfigs; configIdx++) {
                    merge(finalOpts, metaProj->proj->configs[configIdx].opts.tcOpts, configIdx,
                          numConfigs);
                }
                merge(finalOpts, meta->opts.tcOpts, -1, numConfigs);
                for (u32 configIdx = 0; configIdx < meta->configs.numItems(); configIdx++) {
                    merge(finalOpts, meta->configs[configIdx].tcOpts, configIdx, numConfigs);
                }
            }

            Array<CompileOpts> configToCompileOpts;
            for (u32 i : range(metaProj->proj->configs.numItems())) {
                configToCompileOpts.append(extractCompilerOpts(metaProj->tc, finalOpts, i));
            }

            // Write include directories
            outs->format("target_include_directories({} PRIVATE\n", meta->node->name);
            for (const FinalOption& opt : finalOpts) {
                if (opt.type == ToolchainOpt::IncludeDir) {
                    for (u32 configIdx = 0; configIdx < opt.values.numItems(); configIdx++) {
                        if (opt.values[configIdx] != ToolchainOpt::Erased) {
                            if (opt.values.numItems() == 1) {
                                outs->format("    \"{}\"\n",
                                             CMakeEscape{PosixPath::from<NativePath>(opt.key)});
                            } else {
                                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n",
                                             metaProj->proj->configs[configIdx].name,
                                             CMakeEscape{PosixPath::from<NativePath>(opt.key)});
                            }
                        }
                    }
                }
            }
            *outs << ")\n";

            // Write compile options
            outs->format("set_property(TARGET {} PROPERTY COMPILE_OPTIONS\n", meta->node->name);
            for (u32 configIdx = 0; configIdx < metaProj->proj->configs.numItems(); configIdx++) {
                StringView configName = metaProj->proj->configs[configIdx].name;
                String escapedList = escapeCMakeList(configToCompileOpts[configIdx].compile);
                outs->format("    \"$<$<CONFIG:{}>:{}>\"\n", configName, escapedList);
            }
            *outs << ")\n";

            if (meta->node->type == Node::EXE) {
                // Write libraries to link with
                outs->format("target_link_libraries({} PRIVATE\n", meta->node->name);
                for (u32 i = 0; i < meta->opts.dependencies.numItems(); i++) {
                    MetaNode* metaDep = meta->opts.dependencies[i];
                    for (StringView lib : metaDep->node->opts.prebuiltLibs) {
                        outs->format("    {}\n", lib);
                    }
                    if (metaDep->needsBuild) {
                        outs->format("    {}\n", metaDep->node->name);
                    }
                }
                *outs << ")\n";

                // Write link options
                outs->format("set_target_properties({} PROPERTIES\n", meta->node->name);
                for (u32 configIdx = 0; configIdx < metaProj->proj->configs.numItems();
                     configIdx++) {
                    StringView configName = metaProj->proj->configs[configIdx].name;
                    String escapedList = escapeCMakeList(configToCompileOpts[configIdx].link);
                    outs->format("    LINK_FLAGS_{} \"{}\"\n", configName.upperAsc(), escapedList);
                }
                *outs << ")\n";
            }
        }
    }
}

void destroy(MetaProject* mp) {
    delete mp;
}

} // namespace buildSteps
