/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>
#include <ply-build-repo/BuildFolder.h>

namespace ply {
namespace build {

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
    biscuit::Interpreter interp;
    TargetInstantiator* mi = nullptr;
    Repository::Function* target_func = nullptr;
    Target* target = nullptr;

    String makeAbsPath(StringView relPath) {
        StringView plyfilePath =
            Path.split(this->target_func->plyfile->tkr.fileLocationMap.path).first;
        return Path.join(plyfilePath, relPath);
    }
};

enum class Visibility {
    Error,
    Public,
    Private,
};

Visibility getVisibility(biscuit::Interpreter* interp, const AnyObject& attributes,
                         bool isTarget, StringView propertyType) {
    Visibility vis = Visibility::Private;
    s32 tokenIdx = -1;
    if (const StatementAttributes* sa = attributes.cast<StatementAttributes>()) {
        tokenIdx = sa->visibilityTokenIdx;
        if (sa->isPublic) {
            vis = Visibility::Public;
        }
    }
    if (isTarget) {
        if (tokenIdx < 0) {
            interp->base.error(String::format(
                "{} must have 'public' or 'private' attribute", propertyType));
            return Visibility::Error;
        }
    } else {
        if (tokenIdx >= 0) {
            biscuit::ExpandedToken token =
                interp->currentFrame->tkr->expandToken(tokenIdx);
            interp->base.error(
                String::format("'{}' cannot be used inside config block", token.text));
            return Visibility::Error;
        }
    }
    return vis;
}

bool assignToCompileOptions(PropertyCollector* pc, const AnyObject& attributes,
                            Label label) {
    Visibility vis =
        getVisibility(pc->interp, attributes, pc->isTarget, "compile options");
    if (vis == Visibility::Error)
        return false;

    Option opt{Option::Generic, g_labelStorage.view(label),
               *pc->interp->base.returnValue.cast<String>()};
    opt.enabledBits |= pc->configBit;
    if (vis == Visibility::Public) {
        opt.isPublicBits |= pc->configBit;
    }
    append_option(*pc->options, opt);
    return true;
}

bool onEvaluateSourceFile(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String absPath = ii->makeAbsPath(*ii->interp.base.returnValue.cast<String>());
    SourceGroup& srcGroup =
        appendOrFind(ii->target->sourceGroups, absPath,
                     [&](const auto& a) { return a.absPath == absPath; });
    bool needsCompilation = false;
    for (WalkTriple& triple : FileSystem.walk(absPath, 0)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                if (!needsCompilation && file.name.endsWith(".cpp")) {
                    needsCompilation = true;
                }
                String relPath =
                    Path.makeRelative(absPath, Path.join(triple.dirPath, file.name));
                SourceFile& srcFile = appendOrFind(
                    srcGroup.files, std::move(relPath),
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
    Visibility vis =
        getVisibility(pc->interp, attributes, pc->isTarget, "include directory");
    if (vis == Visibility::Error)
        return false;

    Option opt{Option::IncludeDir,
               Path.join(pc->basePath, *pc->interp->base.returnValue.cast<String>())};
    Option& foundOpt = appendOrFind(*pc->options, std::move(opt),
                                    [&](const Option& o) { return o == opt; });
    foundOpt.enabledBits |= pc->configBit;
    if (vis == Visibility::Public) {
        foundOpt.isPublicBits |= pc->configBit;
    }
    return true;
}

bool onEvaluatePreprocessorDefinition(PropertyCollector* pc,
                                      const AnyObject& attributes) {
    Visibility vis =
        getVisibility(pc->interp, attributes, pc->isTarget, "preprocessor definition");
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
    Repository::Function* target =
        ii->interp.base.returnValue.cast<Repository::Function>();
    Target* depTarget = nullptr;
    if (instantiateTargetForCurrentConfig(&depTarget, ii->mi,
                                          target->stmt->customBlock()->name) != Fn_OK)
        return false;
    Dependency& foundDep =
        appendOrFind(ii->target->dependencies, depTarget,
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

FnResult custom_block_inside_config(PropertyCollector* pc,
                                    const biscuit::Statement::CustomBlock* cb) {
    biscuit::Interpreter::Hooks hooks;
    if (cb->type == g_common->includeDirectoriesKey) {
        hooks.onEvaluate = {onEvaluateIncludeDirectory, pc};
    } else if (cb->type == g_common->preprocessorDefinitionsKey) {
        hooks.onEvaluate = {onEvaluatePreprocessorDefinition, pc};
    } else if (cb->type == g_common->compileOptionsKey) {
        hooks.assignToLocal = {assignToCompileOptions, pc};
    } else if (cb->type == g_common->linkLibrariesKey) {
        hooks.onEvaluate = {onEvaluateLinkLibrary, pc};
    } else {
        // FIXME: Make this a runtime error instead of an assert because the config
        // block can call a function that contains, for example, a dependencies {} block
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(pc->interp->currentFrame->hooks, hooks);
    return execBlock(pc->interp->currentFrame, cb->body);
}

FnResult
custom_block_inside_target_function(InstantiatingInterpreter* ii,
                                    const biscuit::Statement::CustomBlock* cb) {
    PropertyCollector pc;
    pc.interp = &ii->interp;
    pc.basePath = Path.split(ii->target_func->plyfile->tkr.fileLocationMap.path).first;
    pc.options = &ii->target->options;
    pc.configBit = ii->mi->configBit;
    pc.isTarget = true;

    biscuit::Interpreter::Hooks hooks;
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

FnResult runGenerateBlock(Repository::Function* target) {
    // Create new interpreter.
    biscuit::Interpreter interp;
    interp.base.error = [&interp](StringView message) {
        OutStream out = Console.error();
        logErrorWithStack(out, &interp, message);
    };

    // Populate dictionaries.
    BuiltInStorage.sys_cmake_path = PLY_CMAKE_PATH;
    BuiltInStorage.script_path = target->plyfile->tkr.fileLocationMap.path;
    interp.resolveName = [](Label identifier) -> AnyObject {
        if (AnyObject* builtIn = BuiltInMap.find(identifier))
            return *builtIn;
        return {};
    };

    // Invoke generate block.
    biscuit::Interpreter::StackFrame frame;
    frame.interp = &interp;
    frame.desc = [target]() -> HybridString {
        return String::format("library '{}'",
                              g_labelStorage.view(target->stmt->customBlock()->name));
    };
    frame.tkr = &target->plyfile->tkr;
    interp.currentFrame = &frame;
    return execBlock(&frame, target->generateBlock->customBlock()->body);
}

FnResult fn_link_objects_directly(InstantiatingInterpreter* ii,
                                  const FnParams& params) {
    if (params.args.numItems != 0) {
        params.base->error(
            String::format("'link_objects_directly' takes no arguments"));
        return Fn_Error;
    }

    if (ii->target->type == Target::Executable) {
        params.base->error("Target must be a library");
        return Fn_Error;
    }

    ii->target->type = Target::ObjectLibrary;

    return Fn_OK;
}

FnResult instantiateTargetForCurrentConfig(Target** outTarget, TargetInstantiator* mi,
                                           Label name) {
    // Check for an existing target; otherwise create one.
    Target* target = nullptr;
    {
        bool was_found = false;
        TargetWithStatus* tws = mi->targetMap.insert_or_find(name, &was_found);
        if (!was_found) {
            // No existing target found. Create a new one.
            tws->target = new Target;
            tws->target->name = name;
            Project.targets.append(tws->target);
        } else {
            // Yes. If the library was already fully instantiated in this config, return
            // it.
            if (tws->statusInCurrentConfig == Instantiated) {
                *outTarget = tws->target;
                return Fn_OK;
            }
            // Circular dependency check. FIXME: Handle gracefully
            if (tws->statusInCurrentConfig == Instantiating) {
                PLY_ASSERT(0);
            }
            PLY_ASSERT(tws->statusInCurrentConfig == NotInstantiated);
        }
        // Set this library's status as Instantiating so that circular dependencies can
        // be detected.
        tws->statusInCurrentConfig = Instantiating;
        target = tws->target;
        *outTarget = tws->target;
    }

    // Set node as active in this config.
    PLY_ASSERT(mi->configBit);
    target->enabledBits |= mi->configBit;

    // Find library function by name.
    Repository::Function** func_ = g_repository->globalScope.find(name);
    if (!func_ || !(*func_)->stmt->customBlock()) {
        PLY_FORCE_CRASH(); // FIXME: Handle gracefully
    }
    Repository::Function* target_func = *func_;

    // Run the generate block if it didn't run already.
    if (!target_func->generatedOnce) {
        if (target_func->generateBlock) {
            FnResult result = runGenerateBlock(target_func);
            if (result != Fn_OK)
                return result;
        }
        target_func->generatedOnce = true;
    }

    const biscuit::Statement::CustomBlock* targetDef =
        target_func->stmt->customBlock().get();
    if (targetDef->type == g_common->executableKey) {
        target->type = Target::Executable;
    } else {
        PLY_ASSERT(targetDef->type == g_common->libraryKey);
        PLY_ASSERT(target->type == Target::Library ||
                   target->type == Target::ObjectLibrary);
    }

    // Create new interpreter.
    InstantiatingInterpreter ii;
    ii.interp.base.error = [&ii](StringView message) {
        OutStream out = Console.error();
        logErrorWithStack(out, &ii.interp, message);
    };
    ii.mi = mi;
    ii.target_func = target_func;
    ii.target = target;

    // Populate global & library namespaces.
    ii.interp.resolveName = [&ii](Label identifier) -> AnyObject {
        if (AnyObject* builtIn = BuiltInMap.find(identifier))
            return *builtIn;
        if (AnyObject* obj = ii.target_func->currentOptions->map.find(identifier))
            return *obj;
        if (identifier == g_common->linkObjectsDirectlyKey) {
            AnyObject* obj = ii.interp.base.localVariableStorage.appendObject(
                getTypeDescriptor<BoundNativeMethod>());
            *obj->cast<BoundNativeMethod>() = {&ii, fn_link_objects_directly};
            return *obj;
        }
        if (Repository::Function** target =
                g_repository->globalScope.find(identifier)) {
            if (auto fnDef = (*target)->stmt->functionDefinition())
                return AnyObject::bind(fnDef.get());
            else
                return AnyObject::bind(*target);
        }
        return {};
    };

    // Invoke library function.
    biscuit::Interpreter::StackFrame frame;
    frame.hooks.doCustomBlock = {custom_block_inside_target_function, &ii};
    frame.interp = &ii.interp;
    frame.desc = [targetDef]() -> HybridString {
        return String::format("library '{}'", g_labelStorage.view(targetDef->name));
    };
    frame.tkr = &target_func->plyfile->tkr;
    FnResult result = execFunction(&frame, targetDef->body);
    mi->targetMap.find(name)->statusInCurrentConfig = Instantiated;
    return result;
}

struct ConfigListInterpreter {
    biscuit::Interpreter interp;
    BuildFolder_t* build_folder = nullptr;
    TargetInstantiator* mi = nullptr;
};

FnResult custom_block_inside_config_list(ConfigListInterpreter* cli,
                                         const biscuit::Statement::CustomBlock* cb) {
    PLY_ASSERT(cb->type == g_common->configKey);

    // Evaluate config name
    FnResult result = eval(cli->interp.currentFrame, cb->expr);
    PLY_ASSERT(result == Fn_OK); // FIXME: Make robust
    String currentConfigName = *cli->interp.base.returnValue.cast<String>();
    PLY_ASSERT(currentConfigName); // FIXME: Make robust

    // Initialize all Target::currentOptions
    for (Repository::Function* target : g_repository->targets) {
        auto newOptions = Owned<Repository::ConfigOptions>::create();
        for (const auto& item : target->defaultOptions->map) {
            AnyOwnedObject* dst = newOptions->map.insert_or_find(item.key);
            *dst = AnyOwnedObject::create(item.value.type);
            dst->copy(item.value);
        }
        target->currentOptions = std::move(newOptions);
    }

    // By default, enable debug info and disable optimization
    u32 configIndex = Project.configNames.numItems();
    PLY_ASSERT(configIndex < 64); // FIXME: Handle elegantly
    {
        Option debugInfo{Option::Generic, "debug_info", "true"};
        debugInfo.enabledBits |= u64{1} << configIndex;
        append_option(Project.perConfigOptions, debugInfo);
        Option optim{Option::Generic, "optimization", "none"};
        optim.enabledBits |= u64{1} << configIndex;
        append_option(Project.perConfigOptions, optim);
    }

    // Execute config block
    PropertyCollector pc;
    pc.interp = &cli->interp;
    pc.basePath = Path.split(cli->interp.currentFrame->tkr->fileLocationMap.path).first;
    pc.options = &Project.perConfigOptions;
    pc.configBit = u64{1} << configIndex;
    biscuit::Interpreter::Hooks hooks;
    hooks.doCustomBlock = {custom_block_inside_config, &pc};
    PLY_SET_IN_SCOPE(cli->interp.currentFrame->hooks, hooks);
    result = execBlock(cli->interp.currentFrame, cb->body);
    if (result != Fn_OK)
        return result;

    // Add config to project
    Project.configNames.append(currentConfigName);

    // Instantiate all root targets in this config
    PLY_SET_IN_SCOPE(cli->mi->configBit, pc.configBit);
    for (StringView targetName : cli->build_folder->rootTargets) {
        Target* rootTarget = nullptr;
        FnResult result = instantiateTargetForCurrentConfig(
            &rootTarget, cli->mi, g_labelStorage.insert(targetName));
        if (result != Fn_OK)
            return result;
    }

    // Reset state between configs.
    // Clear currentConfigName, Target::currentOptions, and set TargetInstantiator
    // status to NotInstantiated.
    for (Repository::Function* target : g_repository->targets) {
        target->currentOptions.clear();
    }
    for (auto& item : cli->mi->targetMap) {
        item.value.statusInCurrentConfig = NotInstantiated;
    }

    return Fn_OK;
}

PLY_NO_INLINE void instantiate_all_configs(BuildFolder_t* build_folder) {
    TargetInstantiator mi{};
    Project.name = build_folder->solutionName;
    init_toolchain_msvc();

    // Execute the config_list block
    Repository::ConfigList* configList = g_repository->configList;
    if (!configList) {
        Error.log("No config_list block defined.\n");
    }

    {
        // Create new interpreter.
        ConfigListInterpreter cli;
        cli.interp.base.error = [&cli](StringView message) {
            OutStream out = Console.error();
            logErrorWithStack(out, &cli.interp, message);
        };
        cli.mi = &mi;

        // Add builtin namespace.
        Map<Label, AnyObject> builtIns;
        bool true_ = true;
        bool false_ = false;
        builtIns.assign(g_labelStorage.insert("true"), AnyObject::bind(&true_));
        builtIns.assign(g_labelStorage.insert("false"), AnyObject::bind(&false_));
        cli.build_folder = build_folder;
        cli.interp.resolveName = [&builtIns, &cli](Label identifier) -> AnyObject {
            if (AnyObject* builtIn = builtIns.find(identifier))
                return *builtIn;
            if (Repository::Function** target =
                    g_repository->globalScope.find(identifier)) {
                if (auto fnDef = (*target)->stmt->functionDefinition())
                    return AnyObject::bind(fnDef.get());
                else
                    // FIXME: Don't resolve library names outside config {} block
                    return AnyObject::bind((*target)->currentOptions.get());
            }
            return {};
        };

        // Invoke block.
        biscuit::Interpreter::StackFrame frame;
        frame.interp = &cli.interp;
        frame.desc = []() -> HybridString { return "config_list"; };
        frame.tkr = &configList->plyfile->tkr;
        frame.hooks.doCustomBlock = {custom_block_inside_config_list, &cli};
        FnResult result =
            execFunction(&frame, configList->blockStmt->customBlock()->body);
        if (result == Fn_Error) {
            exit(1);
        }
    }

    do_inheritance();
}

} // namespace build
} // namespace ply
