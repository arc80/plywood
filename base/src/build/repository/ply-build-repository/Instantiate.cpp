/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Instantiate.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace build {
namespace latest {

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
    buildSteps::Node* node = nullptr;

    String makeAbsPath(StringView relPath) {
        StringView plyfilePath =
            NativePath::split(this->currentModule->plyfile->tkr.fileLocationMap.path).first;
        return NativePath::join(plyfilePath, relPath);
    }
};

bool customBlock(InstantiatingInterpreter* ii, const crowbar::Statement::CustomBlock* enteringBlock,
                 bool isEntering) {
    if (isEntering) {
        if (ii->interp.currentFrame->customBlock) {
            PLY_FORCE_CRASH();
        }
    } else {
    }
    return false;
}

bool onEvaluate(InstantiatingInterpreter* ii, const AnyObject& evaluationTraits) {
    if (ii->interp.currentFrame->customBlock) {
        Label blockType = ii->interp.currentFrame->customBlock->type;

        if (blockType == g_common->sourceFilesKey) {
            // Expression inside source_files block
            PLY_ASSERT(!evaluationTraits.data);
            String absPath = ii->makeAbsPath(*ii->interp.base.returnValue.cast<String>());
            buildSteps::Node::SourceFilePath& srcFilePath =
                appendOrFind(ii->node->sourceFilePaths, std::move(absPath),
                             [&](const auto& a) { return a.path == absPath; });
            srcFilePath.activeMask |= ii->mi->configBit;

        } else if (blockType == g_common->includeDirectoriesKey) {
            // Expression inside include_directories block
            const ExpressionTraits* traits = evaluationTraits.cast<ExpressionTraits>();
            PLY_ASSERT(traits->visibilityTokenIdx >= 0);
            buildSteps::ToolchainOpt tcOpt{
                buildSteps::ToolchainOpt::Type::IncludeDir,
                ii->makeAbsPath(*ii->interp.base.returnValue.cast<String>())};
            buildSteps::Node::Option& foundOpt = appendOrFind(
                ii->node->options, std::move(tcOpt), [&](const auto& a) { return a.opt == tcOpt; });
            foundOpt.activeMask |= ii->mi->configBit;
            if (traits->isPublic) {
                foundOpt.publicMask |= ii->mi->configBit;
            }

        } else if (blockType == g_common->dependenciesKey) {
            // Expression inside dependencies block
            const ExpressionTraits* traits = evaluationTraits.cast<ExpressionTraits>();
            PLY_ASSERT(traits->visibilityTokenIdx >= 0);

            // Instantiate the dependency
            Repository::ModuleOrFunction* mod =
                ii->interp.base.returnValue.cast<Repository::ModuleOrFunction>();
            buildSteps::Node* dep =
                instantiateModuleForCurrentConfig(ii->mi, mod->stmt->customBlock()->name);
            if (!dep)
                return true;
            buildSteps::Node::Dependency& foundDep = appendOrFind(
                ii->node->dependencies, dep, [&](const auto& a) { return a.dep == dep; });
            foundDep.activeMask |= ii->mi->configBit;

        } else if (blockType == g_common->linkLibrariesKey) {
            // PLY_ASSERT(!evaluationTraits.data);
            // this->nodeConfig->opts.prebuiltLibs.append(
            //    *this->interp->returnValue.cast<String>());
        } else {
            // Shouldn't get here
            PLY_ASSERT(0);
        }
    }
    return false;
}

MethodResult doJoinPath(const MethodArgs& args) {
    Array<StringView> parts;
    parts.reserve(args.args.numItems);
    for (const AnyObject& arg : args.args) {
        if (!arg.is<String>())
            PLY_FORCE_CRASH();
        parts.append(*arg.cast<String>());
    }
    String result = PathFormat{false}.joinAndNormalize(parts);
    AnyObject* resultStorage =
        args.base->localVariableStorage.appendObject(getTypeDescriptor(&result));
    *resultStorage->cast<String>() = std::move(result);
    args.base->returnValue = *resultStorage;
    return MethodResult::OK;
}

MethodResult getExternFolder(const MethodArgs& args) {
    if (args.args.numItems != 1) {
        args.base->error(String::format("'get_extern_folder' expects 1 argument"));
        return MethodResult::Error;
    }
    String* name = args.args[0].safeCast<String>();
    if (!name) {
        args.base->error(String::format("'get_extern_folder' argument must be a string"));
        return MethodResult::Error;
    }
    
    AnyObject* resultStorage =
        args.base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *resultStorage->cast<String>() = "...";
    args.base->returnValue = *resultStorage;
    return MethodResult::OK;
}

struct ReadOnlyDict {
    String name;
    LabelMap<AnyOwnedObject> map;
    
    ReadOnlyDict(StringView name) : name{name} {
    }
};

}
}
PLY_DECLARE_TYPE_DESCRIPTOR(build::latest::ReadOnlyDict)
namespace build {
namespace latest {

PLY_NO_INLINE MethodTable getMethodTable_ReadOnlyDict() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> MethodResult {
        auto* dict = obj.cast<ReadOnlyDict>();
        Label label = g_labelStorage.find(propertyName);
        if (label) {
            AnyObject* prop = dict->map.find(label);
            if (prop) {
                interp->returnValue = *prop;
                return MethodResult::OK;
            }
        }

        interp->returnValue = {};
        interp->error(
            String::format("property '{}' not found in '{}'", propertyName, dict->name));
        return MethodResult::Error;
    };
    return methods;
}

buildSteps::Node* instantiateModuleForCurrentConfig(ModuleInstantiator* mi, Label moduleLabel) {
    StringView moduleName = g_labelStorage.view(moduleLabel);

    // Check if a buildSteps::Node was already created for this moduleName.
    buildSteps::Node* node;
    {
        auto instCursor = mi->modules.insertOrFind(moduleName);
        if (!instCursor.wasFound()) {
            // No. Create a new one.
            instCursor->node = new buildSteps::Node;
            instCursor->node->name = moduleName;
        } else {
            // Yes. If the module was already fully instantiated in this config, return it.
            if (instCursor->statusInCurrentConfig == ModuleInstantiator::Instantiated)
                return instCursor->node;
            // Circular dependency check. FIXME: Handle gracefully
            if (instCursor->statusInCurrentConfig == ModuleInstantiator::Instantiating) {
                PLY_FORCE_CRASH();
            }
            PLY_ASSERT(instCursor->statusInCurrentConfig == ModuleInstantiator::NotInstantiated);
        }
        // Set this module's status as Instantiating so that circular dependencies can be detected.
        instCursor->statusInCurrentConfig = ModuleInstantiator::Instantiating;
        node = instCursor->node;
    }

    // Set node as active in this config.
    PLY_ASSERT(mi->configBit);
    node->configMask |= mi->configBit;

    // Find module function by name.
    latest::Repository::ModuleOrFunction** mod = g_repository->globalScope.find(moduleLabel);
    if (!mod || !(*mod)->stmt->customBlock()) {
        PLY_FORCE_CRASH(); // FIXME: Handle gracefully
    }
    const crowbar::Statement::CustomBlock* moduleDef = (*mod)->stmt->customBlock().get();
    if (moduleDef->type == g_common->executableKey) {
        node->type = buildSteps::Node::Type::Executable;
    }

    // Create new interpreter.
    InstantiatingInterpreter ii;
    ii.interp.base.error = [&ii](StringView message) {
        OutStream outs = StdErr::text();
        logErrorWithStack(&outs, &ii.interp, message);
    };
    ii.interp.hooks.customBlock = {customBlock, &ii};
    ii.interp.hooks.onEvaluate = {onEvaluate, &ii};
    ii.mi = mi;
    ii.currentModule = *mod;
    ii.node = node;

    // Populate global & module namespaces.
    *ii.interp.builtIns.insert(g_labelStorage.insert("join_path")) = AnyObject::bind(doJoinPath);
    *ii.interp.builtIns.insert(g_labelStorage.insert("build_folder")) =
        AnyObject::bind(&ii.mi->buildFolderPath);
    ReadOnlyDict dict{"build"};
    *dict.map.insert(g_labelStorage.insert("arch")) = AnyOwnedObject::create<String>("x64");
    *ii.interp.builtIns.insert(g_labelStorage.insert("build")) = AnyObject::bind(&dict);
    ReadOnlyDict sysDict{"sys"};
    *sysDict.map.insert(g_labelStorage.insert("get_extern_folder")) = AnyObject::bind(getExternFolder);
    *ii.interp.builtIns.insert(g_labelStorage.insert("sys")) = AnyObject::bind(&sysDict);
    AnyObject::bind(&ii.mi->buildFolderPath);
    ii.interp.hooks.resolveName = [&ii](Label identifier) -> AnyObject {
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
    frame.interp = &ii.interp;
    frame.desc = [moduleDef]() -> HybridString {
        return String::format("module '{}'", g_labelStorage.view(moduleDef->name));
    };
    frame.tkr = &(*mod)->plyfile->tkr;
    MethodResult result = execFunction(&frame, moduleDef->body);
    if (result == MethodResult::Error)
        return nullptr;

    mi->modules.find(moduleName)->statusInCurrentConfig = ModuleInstantiator::Instantiated;
    return node;
}

TypeKey TypeKey_ReadOnlyDict{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "ReadOnlyDict";
    },

    // write
    nullptr, // Unimplemented

    // writeFormat
    nullptr, // Unimplemented

    // read
    nullptr, // Unimplemented

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace latest
} // namespace build
} // namespace ply

PLY_DEFINE_TYPE_DESCRIPTOR(ply::build::latest::ReadOnlyDict) {
    static TypeDescriptor typeDesc{
        &ply::build::latest::TypeKey_ReadOnlyDict,
        (ply::build::latest::ReadOnlyDict*) nullptr,
        NativeBindings::make<build::latest::ReadOnlyDict>()
            PLY_METHOD_TABLES_ONLY(, build::latest::getMethodTable_ReadOnlyDict())};
    return &typeDesc;
}
