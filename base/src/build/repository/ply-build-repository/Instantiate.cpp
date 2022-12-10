/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Instantiate.h>
#include <ply-build-repository/BuiltIns.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build2 {

//--------------------------------------------------------------

template <typename T, typename U, typename Callable>
T& appendOrFind(Array<T>& arr, U&& item, const Callable& callable) {
    for (u32 i = 0; i < arr.numItems(); i++) {
        if (callable(arr[i]))
            return arr[i];
    }
    return arr.append(U{std::forward<U>(item)});
}

struct InstantiatingInterpreter {
    crowbar::Interpreter interp;
    ModuleInstantiator* mi = nullptr;
    Repository::ModuleOrFunction* currentModule = nullptr;
    Target* target = nullptr;

    String makeAbsPath(StringView relPath) {
        StringView plyfilePath =
            NativePath::split(this->currentModule->plyfile->tkr.fileLocationMap.path).first;
        return NativePath::join(plyfilePath, relPath);
    }
};

enum class Visibility {
    Error,
    Public,
    Private,
};

Visibility getVisibility(crowbar::Interpreter* interp, const AnyObject& attributes, bool isModule,
                         StringView propertyType) {
    Visibility vis = Visibility::Private;
    s32 tokenIdx = -1;
    if (const StatementAttributes* sa = attributes.cast<StatementAttributes>()) {
        tokenIdx = sa->visibilityTokenIdx;
        if (sa->isPublic) {
            vis = Visibility::Public;
        }
    }
    if (isModule) {
        if (tokenIdx < 0) {
            interp->base.error(
                String::format("{} must have 'public' or 'private' attribute", propertyType));
            return Visibility::Error;
        }
    } else {
        if (tokenIdx >= 0) {
            crowbar::ExpandedToken token = interp->currentFrame->tkr->expandToken(tokenIdx);
            interp->base.error(
                String::format("'{}' cannot be used inside config block", token.text));
            return Visibility::Error;
        }
    }
    return vis;
}

bool assignToCompileOptions(PropertyCollector* pc, const AnyObject& attributes, Label label) {
    Visibility vis = getVisibility(pc->interp, attributes, pc->isModule, "compile options");
    if (vis == Visibility::Error)
        return false;

    Option tcOpt{Option::Generic, g_labelStorage.view(label),
                 *pc->interp->base.returnValue.cast<String>()};
    int i = find(*pc->options,
                 [&](const Option& o) { return (o.type == tcOpt.type) && (o.key == tcOpt.key); });
    if (i >= 0) {
        pc->options->erase(i);
    }
    Option& opt = pc->options->append(std::move(tcOpt));
    opt.enabledBits |= pc->configBit;
    if (vis == Visibility::Public) {
        opt.isPublicBits |= pc->configBit;
    }
    return true;
}

bool onEvaluateSourceFile(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String absPath = ii->makeAbsPath(*ii->interp.base.returnValue.cast<String>());
    SourceGroup& srcGroup = appendOrFind(ii->target->sourceGroups, absPath,
                                         [&](const auto& a) { return a.absPath == absPath; });
    bool needsCompilation = false;
    for (WalkTriple& triple : FileSystem::native()->walk(absPath, 0)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if ((file.name.endsWith(".cpp") && !file.name.endsWith(".modules.cpp")) ||
                file.name.endsWith(".h")) {
                if (!needsCompilation && file.name.endsWith(".cpp")) {
                    needsCompilation = true;
                }
                String relPath =
                    NativePath::makeRelative(absPath, NativePath::join(triple.dirPath, file.name));
                SourceFile& srcFile =
                    appendOrFind(srcGroup.files, std::move(relPath),
                                 [&](const SourceFile& a) { return a.relPath == relPath; });
                srcFile.enabledBits |= ii->mi->configBit;
            }
        }
    }
    if (needsCompilation) {
        ii->target->hasBuildStepBits |= ii->mi->configBit;
    }
    return true;
}

bool onEvaluateIncludeDirectory(PropertyCollector* pc, const AnyObject& attributes) {
    Visibility vis = getVisibility(pc->interp, attributes, pc->isModule, "include directory");
    if (vis == Visibility::Error)
        return false;

    Option opt{Option::IncludeDir,
               NativePath::join(pc->basePath, *pc->interp->base.returnValue.cast<String>())};
    Option& foundOpt = appendOrFind(*pc->options, std::move(opt),
                                    [&](const Option& o) { return o == opt; });
    foundOpt.enabledBits |= pc->configBit;
    if (vis == Visibility::Public) {
        foundOpt.isPublicBits |= pc->configBit;
    }
    return true;
}

bool onEvaluatePreprocessorDefinition(PropertyCollector* pc, const AnyObject& attributes) {
    Visibility vis = getVisibility(pc->interp, attributes, pc->isModule, "preprocessor definition");
    if (vis == Visibility::Error)
        return false;

    String key = *pc->interp->base.returnValue.cast<String>();
    String value;
    s32 i = key.findByte('=');
    if (i >= 0) {
        value = key.subStr(i + 1);
        key = key.left(i);
    }

    Option opt{Option::PreprocessorDef, key, value};
    Option& foundOpt = appendOrFind(*pc->options, std::move(opt), [&](const Option& o) {
        return (o.type == Option::PreprocessorDef) && (o.key == opt.key);
    });
    foundOpt.enabledBits |= pc->configBit;
    if (vis == Visibility::Public) {
        foundOpt.isPublicBits |= pc->configBit;
    }
    return true;
}

bool onEvaluateDependency(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    Visibility vis = getVisibility(&ii->interp, attributes, true, "dependency");
    if (vis == Visibility::Error)
        return false;

    // Instantiate the dependency
    Repository::ModuleOrFunction* mod =
        ii->interp.base.returnValue.cast<Repository::ModuleOrFunction>();
    Target* depTarget = nullptr;
    if (instantiateModuleForCurrentConfig(&depTarget, ii->mi, mod->stmt->customBlock()->name) !=
        MethodResult::OK)
        return false;
    Dependency& foundDep = appendOrFind(ii->target->dependencies, depTarget,
                                        [&](const Dependency& d) { return d.target == depTarget; });
    foundDep.enabledBits |= ii->mi->configBit;
    if (vis == Visibility::Public) {
        foundDep.isPublicBits |= ii->mi->configBit;
    }
    return true;
}

bool onEvaluateLinkLibrary(PropertyCollector* pc, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String* path = pc->interp->base.returnValue.cast<String>();
    Option desiredOpt{Option::LinkerInput, *path, {}};
    Option& opt = appendOrFind(*pc->options, desiredOpt,
                               [&](const Option& o) { return o == desiredOpt; });
    opt.enabledBits |= pc->configBit;
    return true;
}

MethodResult doCustomBlockInsideConfig(PropertyCollector* pc,
                                       const crowbar::Statement::CustomBlock* cb) {
    crowbar::Interpreter::Hooks hooks;
    if (cb->type == g_common->includeDirectoriesKey) {
        hooks.onEvaluate = {onEvaluateIncludeDirectory, pc};
    } else if (cb->type == g_common->preprocessorDefinitionsKey) {
        hooks.onEvaluate = {onEvaluatePreprocessorDefinition, pc};
    } else if (cb->type == g_common->compileOptionsKey) {
        hooks.assignToLocal = {assignToCompileOptions, pc};
    } else if (cb->type == g_common->linkLibrariesKey) {
        hooks.onEvaluate = {onEvaluateLinkLibrary, pc};
    } else {
        // FIXME: Make this a runtime error instead of an assert because the config block can call a
        // function that contains, for example, a dependencies {} block
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(pc->interp->currentFrame->hooks, hooks);
    return execBlock(pc->interp->currentFrame, cb->body);
}

MethodResult doCustomBlockAtModuleScope(InstantiatingInterpreter* ii,
                                        const crowbar::Statement::CustomBlock* cb) {
    PropertyCollector pc;
    pc.interp = &ii->interp;
    pc.basePath = NativePath::split(ii->currentModule->plyfile->tkr.fileLocationMap.path).first;
    pc.options = &ii->target->options;
    pc.configBit = ii->mi->configBit;
    pc.isModule = true;

    crowbar::Interpreter::Hooks hooks;
    if (cb->type == g_common->sourceFilesKey) {
        hooks.onEvaluate = {onEvaluateSourceFile, ii};
    } else if (cb->type == g_common->includeDirectoriesKey) {
        hooks.onEvaluate = {onEvaluateIncludeDirectory, &pc};
    } else if (cb->type == g_common->preprocessorDefinitionsKey) {
        hooks.onEvaluate = {onEvaluatePreprocessorDefinition, &pc};
    } else if (cb->type == g_common->compileOptionsKey) {
        hooks.assignToLocal = {assignToCompileOptions, &pc};
    } else if (cb->type == g_common->linkLibrariesKey) {
        hooks.onEvaluate = {onEvaluateLinkLibrary, &pc};
    } else if (cb->type == g_common->dependenciesKey) {
        hooks.onEvaluate = {onEvaluateDependency, ii};
    } else {
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(ii->interp.currentFrame->customBlock, cb);
    PLY_SET_IN_SCOPE(ii->interp.currentFrame->hooks, hooks);
    return execBlock(ii->interp.currentFrame, cb->body);
}

MethodResult runGenerateBlock(Repository::ModuleOrFunction* mod, StringView buildFolderPath) {
    // Create new interpreter.
    crowbar::Interpreter interp;
    interp.base.error = [&interp](StringView message) {
        OutStream outs = StdErr::text();
        logErrorWithStack(&outs, &interp, message);
    };

    // Populate dictionaries.
    BuiltInStorage.sys_build_folder = buildFolderPath;
    BuiltInStorage.sys_cmake_path = PLY_CMAKE_PATH;
    BuiltInStorage.script_path = mod->plyfile->tkr.fileLocationMap.path;
    interp.resolveName = [](Label identifier) -> AnyObject {
        if (AnyObject* builtIn = BuiltInMap.find(identifier))
            return *builtIn;
        return {};
    };

    // Invoke generate block.
    crowbar::Interpreter::StackFrame frame;
    frame.interp = &interp;
    frame.desc = [mod]() -> HybridString {
        return String::format("module '{}'", g_labelStorage.view(mod->stmt->customBlock()->name));
    };
    frame.tkr = &mod->plyfile->tkr;
    interp.currentFrame = &frame;
    return execBlock(&frame, mod->generateBlock->customBlock()->body);
}

MethodResult instantiateModuleForCurrentConfig(Target** outTarget, ModuleInstantiator* mi,
                                               Label name) {
    // Check for an existing target; otherwise create one.
    Target* target = nullptr;
    {
        TargetWithStatus* tws = nullptr;
        if (mi->moduleMap.insertOrFind(name, &tws)) {
            // No existing target found. Create a new one.
            tws->target = new Target;
            tws->target->name = name;
            Project.targets.append(tws->target);
        } else {
            // Yes. If the module was already fully instantiated in this config, return it.
            if (tws->statusInCurrentConfig == Instantiated) {
                *outTarget = tws->target;
                return MethodResult::OK;
            }
            // Circular dependency check. FIXME: Handle gracefully
            if (tws->statusInCurrentConfig == Instantiating) {
                PLY_ASSERT(0);
            }
            PLY_ASSERT(tws->statusInCurrentConfig == NotInstantiated);
        }
        // Set this module's status as Instantiating so that circular dependencies can be detected.
        tws->statusInCurrentConfig = Instantiating;
        target = tws->target;
        *outTarget = tws->target;
    }

    // Set node as active in this config.
    PLY_ASSERT(mi->configBit);
    target->enabledBits |= mi->configBit;

    // Find module function by name.
    Repository::ModuleOrFunction** mod_ = g_repository->globalScope.find(name);
    if (!mod_ || !(*mod_)->stmt->customBlock()) {
        PLY_FORCE_CRASH(); // FIXME: Handle gracefully
    }
    Repository::ModuleOrFunction* mod = *mod_;

    // Run the generate block if it didn't run already.
    if (!mod->generatedOnce) {
        if (mod->generateBlock) {
            MethodResult result = runGenerateBlock(mod, build::BuildFolder.absPath);
            if (result != MethodResult::OK)
                return result;
        }
        mod->generatedOnce = true;
    }

    const crowbar::Statement::CustomBlock* moduleDef = mod->stmt->customBlock().get();
    if (moduleDef->type == g_common->executableKey) {
        target->type = Target::Executable;
    } else {
        PLY_ASSERT(moduleDef->type == g_common->moduleKey);
        PLY_ASSERT(target->type == Target::Library);
    }

    // Create new interpreter.
    InstantiatingInterpreter ii;
    ii.interp.base.error = [&ii](StringView message) {
        OutStream outs = StdErr::text();
        logErrorWithStack(&outs, &ii.interp, message);
    };
    ii.mi = mi;
    ii.currentModule = mod;
    ii.target = target;

    // Populate global & module namespaces.
    ii.interp.resolveName = [&ii](Label identifier) -> AnyObject {
        if (AnyObject* builtIn = BuiltInMap.find(identifier))
            return *builtIn;
        if (AnyObject* obj = ii.currentModule->currentOptions->map.find(identifier))
            return *obj;
        if (Repository::ModuleOrFunction** mod = g_repository->globalScope.find(identifier)) {
            if (auto fnDef = (*mod)->stmt->functionDefinition())
                return AnyObject::bind(fnDef.get());
            else
                return AnyObject::bind(*mod);
        }
        return {};
    };

    // Invoke module function.
    crowbar::Interpreter::StackFrame frame;
    frame.hooks.doCustomBlock = {doCustomBlockAtModuleScope, &ii};
    frame.interp = &ii.interp;
    frame.desc = [moduleDef]() -> HybridString {
        return String::format("module '{}'", g_labelStorage.view(moduleDef->name));
    };
    frame.tkr = &mod->plyfile->tkr;
    MethodResult result = execFunction(&frame, moduleDef->body);
    mi->moduleMap.find(name)->statusInCurrentConfig = Instantiated;
    return result;
}

// 

struct ConfigListInterpreter {
    crowbar::Interpreter interp;
    build2::ModuleInstantiator* mi = nullptr;
};

MethodResult doCustomBlock(ConfigListInterpreter* cli,
                           const crowbar::Statement::CustomBlock* customBlock) {
    PLY_ASSERT(customBlock->type == build2::g_common->configKey);

    // Evaluate config name
    MethodResult result = eval(cli->interp.currentFrame, customBlock->expr);
    PLY_ASSERT(result == MethodResult::OK); // FIXME: Make robust
    String currentConfigName = *cli->interp.base.returnValue.cast<String>();
    PLY_ASSERT(currentConfigName); // FIXME: Make robust

    // Initialize all Module::currentOptions
    for (build2::Repository::ModuleOrFunction* mod : build2::g_repository->modules) {
        auto newOptions = Owned<build2::Repository::ConfigOptions>::create();
        for (const auto item : mod->defaultOptions->map) {
            AnyOwnedObject* dst = newOptions->map.insert(item.key);
            *dst = AnyOwnedObject::create(item.value.type);
            dst->copy(item.value);
        }
        mod->currentOptions = std::move(newOptions);
    }

    // Enable debug info by default
    u32 configIndex = build2::Project.configNames.numItems();
    PLY_ASSERT(configIndex < 64); // FIXME: Handle elegantly
    build2::Option debugInfo{build2::Option::Generic, "debug_info", "true"};
    debugInfo.enabledBits |= u64{1} << configIndex;
    build2::append_option(build2::Project.perConfigOptions, debugInfo);

    // Execute config block
    build2::PropertyCollector pc;
    pc.interp = &cli->interp;
    pc.basePath = NativePath::split(cli->interp.currentFrame->tkr->fileLocationMap.path).first;
    pc.options = &build2::Project.perConfigOptions;
    pc.configBit = u64{1} << configIndex;
    crowbar::Interpreter::Hooks hooks;
    hooks.doCustomBlock = {build2::doCustomBlockInsideConfig, &pc};
    PLY_SET_IN_SCOPE(cli->interp.currentFrame->hooks, hooks);
    result = execBlock(cli->interp.currentFrame, customBlock->body);
    if (result != MethodResult::OK)
        return result;

    // Add config to project
    build2::Project.configNames.append(currentConfigName);

    // Instantiate all root modules in this config
    PLY_SET_IN_SCOPE(cli->mi->configBit, pc.configBit);
    for (StringView targetName : build::BuildFolder.rootTargets) {
        build2::Target* rootTarget = nullptr;
        MethodResult result = build2::instantiateModuleForCurrentConfig(
            &rootTarget, cli->mi, g_labelStorage.insert(targetName));
        if (result != MethodResult::OK)
            return result;
    }

    // Reset state between configs.
    // Clear currentConfigName, Module::currentOptions, and set ModuleInstantiator status to
    // NotInstantiated.
    for (build2::Repository::ModuleOrFunction* mod : build2::g_repository->modules) {
        mod->currentOptions.clear();
    }
    for (auto& item : cli->mi->moduleMap) {
        item.value.statusInCurrentConfig = build2::NotInstantiated;
    }

    return MethodResult::OK;
}

PLY_NO_INLINE void instantiate_all_configs() {
    build2::ModuleInstantiator mi{};
    build2::Project.name = build::BuildFolder.solutionName;
    build2::init_toolchain_msvc();

    // Execute the config_list block
    build2::Repository::ConfigList* configList = build2::g_repository->configList;
    if (!configList) {
        Error.log("No config_list block defined.\n");
    }

    {
        // Create new interpreter.
        ConfigListInterpreter cli;
        cli.interp.base.error = [&cli](StringView message) {
            OutStream outs = StdErr::text();
            logErrorWithStack(&outs, &cli.interp, message);
        };
        cli.mi = &mi;

        // Add builtin namespace.
        LabelMap<AnyObject> builtIns;
        bool true_ = true;
        bool false_ = false;
        *builtIns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
        *builtIns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);
        cli.interp.resolveName = [&builtIns, &cli](Label identifier) -> AnyObject {
            if (AnyObject* builtIn = builtIns.find(identifier))
                return *builtIn;
            if (build2::Repository::ModuleOrFunction** mod =
                    build2::g_repository->globalScope.find(identifier)) {
                if (auto fnDef = (*mod)->stmt->functionDefinition())
                    return AnyObject::bind(fnDef.get());
                else
                    // FIXME: Don't resolve module names outside config {} block
                    return AnyObject::bind((*mod)->currentOptions.get());
            }
            return {};
        };

        // Invoke block.
        crowbar::Interpreter::StackFrame frame;
        frame.interp = &cli.interp;
        frame.desc = []() -> HybridString { return "config_list"; };
        frame.tkr = &configList->plyfile->tkr;
        frame.hooks.doCustomBlock = {doCustomBlock, &cli};
        MethodResult result = execFunction(&frame, configList->blockStmt->customBlock()->body);
        if (result == MethodResult::Error) {
            exit(1);
        }
    }

    build2::do_inheritance();
}

} // namespace build2
} // namespace ply
