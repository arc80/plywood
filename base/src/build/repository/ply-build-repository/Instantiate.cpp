/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
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

// InterpreterHooks implements hooks that are used to extend the interpreter. Handles custom blocks
// like 'module' and expression traits like 'public'.
struct InterpreterHooks : crowbar::Interpreter::Hooks {
    crowbar::Interpreter* interp = nullptr;
    ModuleInstantiator* mi = nullptr;
    Repository::Module* currentModule = nullptr;
    buildSteps::Node* node = nullptr;

    virtual ~InterpreterHooks() override {
    }

    virtual void enterCustomBlock(const crowbar::Statement::CustomBlock* enteringBlock) override {
        if (this->interp->currentFrame->customBlock) {
            PLY_FORCE_CRASH();
        }
    }

    String makeAbsPath(StringView relPath) const {
        StringView plyfilePath = NativePath::split(this->currentModule->plyfile->path).first;
        return NativePath::join(plyfilePath, relPath);
    }

    virtual void onEvaluate(const AnyObject& evaluationTraits) override {
        const Common* common = &Repository::instance->common;
        if (this->interp->currentFrame->customBlock) {
            Label blockType = this->interp->currentFrame->customBlock->type;

            if (blockType == common->sourceFilesKey) {
                // Expression inside source_files block
                PLY_ASSERT(!evaluationTraits.data);
                String absPath = this->makeAbsPath(*this->interp->returnValue.cast<String>());
                buildSteps::Node::SourceFilePath& srcFilePath =
                    appendOrFind(this->node->sourceFilePaths, std::move(absPath),
                                 [&](const auto& a) { return a.path == absPath; });
                srcFilePath.activeMask |= this->mi->configBit;

            } else if (blockType == common->includeDirectoriesKey) {
                // Expression inside include_directories block
                const ExpressionTraits* traits = evaluationTraits.cast<ExpressionTraits>();
                PLY_ASSERT(traits->visibilityTokenIdx >= 0);
                buildSteps::ToolchainOpt tcOpt{
                    buildSteps::ToolchainOpt::Type::IncludeDir,
                    this->makeAbsPath(*this->interp->returnValue.cast<String>())};
                buildSteps::Node::Option& foundOpt =
                    appendOrFind(this->node->options, std::move(tcOpt),
                                 [&](const auto& a) { return a.opt == tcOpt; });
                foundOpt.activeMask |= this->mi->configBit;
                if (traits->isPublic) {
                    foundOpt.publicMask |= this->mi->configBit;
                }

            } else if (blockType == common->dependenciesKey) {
                // Expression inside dependencies block
                const ExpressionTraits* traits = evaluationTraits.cast<ExpressionTraits>();
                PLY_ASSERT(traits->visibilityTokenIdx >= 0);

                // Instantiate the dependency
                Repository::Module* mod = this->interp->returnValue.cast<Repository::Module>();
                buildSteps::Node* dep = instantiateModuleForCurrentConfig(mi, mod->block->name);
                buildSteps::Node::Dependency& foundDep =
                    appendOrFind(this->node->dependencies, dep, [&](const auto& a) { return a.dep == dep; });
                foundDep.activeMask |= this->mi->configBit;

            } else if (blockType == common->linkLibrariesKey) {
                // PLY_ASSERT(!evaluationTraits.data);
                // this->nodeConfig->opts.prebuiltLibs.append(
                //    *this->interp->returnValue.cast<String>());
            }
        }
    }
};

PLY_NO_INLINE MethodResult doJoinPath(BaseInterpreter* interp, const AnyObject&,
                                      ArrayView<const AnyObject> args) {
    Array<StringView> parts;
    parts.reserve(args.numItems);
    for (const AnyObject& arg : args) {
        if (!arg.is<String>())
            PLY_FORCE_CRASH();
        parts.append(*arg.cast<String>());
    }
    String result = PathFormat{false}.joinAndNormalize(parts);
    AnyObject* resultStorage =
        interp->localVariableStorage.appendObject(getTypeDescriptor(&result));
    *resultStorage->cast<String>() = std::move(result);
    interp->returnValue = *resultStorage;
    return MethodResult::OK;
}

struct ModuleNamespace : crowbar::INamespace {
    virtual AnyObject find(Label identifier) const {
        auto cursor = Repository::instance->moduleMap.find(identifier);
        if (cursor.wasFound()) {
            return AnyObject::bind(cursor->get());
        }
        return {};
    }
};

ModuleInstantiator::ModuleInstantiator(StringView buildFolderPath)
    : buildFolderPath{buildFolderPath} {
    Owned<crowbar::MapNamespace> globalNamespace = Owned<crowbar::MapNamespace>::create();
    globalNamespace->map.insertOrFind(LabelMap::instance.insertOrFind("join_path"))->obj =
        AnyObject::bind(doJoinPath);
    globalNamespace->map.insertOrFind(LabelMap::instance.insertOrFind("build_folder"))->obj =
        AnyObject::bind(&this->buildFolderPath);
    this->globalNamespace = std::move(globalNamespace);
}

buildSteps::Node* instantiateModuleForCurrentConfig(ModuleInstantiator* mi, Label moduleLabel) {
    Common* common = &Repository::instance->common;
    StringView moduleName = LabelMap::instance.view(moduleLabel);

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
            // Circular dependency check.
            if (instCursor->statusInCurrentConfig != ModuleInstantiator::NotInstantiated) {
                PLY_FORCE_CRASH();
            }
        }
        // Set this module's status as Instantiating so that circular dependencies can be detected.
        instCursor->statusInCurrentConfig = ModuleInstantiator::Instantiating;
        node = instCursor->node;
    }

    // Set node as active in this config.
    PLY_ASSERT(mi->configBit);
    node->configMask |= mi->configBit;

    // Find module function by name.
    auto funcCursor = Repository::instance->moduleMap.find(moduleLabel);
    if (!funcCursor.wasFound()) {
        PLY_FORCE_CRASH();
    }
    const crowbar::Statement::CustomBlock* moduleDef = (*funcCursor)->block;
    if (moduleDef->type == common->executableKey) {
        node->type = buildSteps::Node::Type::Executable;
    }

    // Create new interpreter.
    MemOutStream outs;
    crowbar::Interpreter interp;
    interp.outs = &outs;
    interp.error = [](BaseInterpreter* base, StringView message) {
        crowbar::Interpreter* interp = static_cast<crowbar::Interpreter*>(base);
        interp->outs->format("error: {}\n", message);
        bool first = true;
        for (crowbar::Interpreter::StackFrame* frame = interp->currentFrame; frame;
             frame = frame->prevFrame) {
            crowbar::ExpandedToken expToken = interp->tkr->expandToken(frame->tokenIdx);
            interp->outs->format(
                "{} {} {}\n", interp->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                first ? "in" : "called from", frame->desc());
            first = false;
        }
    };
    interp.tkr = &(*funcCursor)->plyfile->tkr;

    // Extend the interpreter with support for custom blocks and expression attributes used by the
    // build system.
    InterpreterHooks interpHooks;
    interpHooks.interp = &interp;
    interpHooks.mi = mi;
    interpHooks.currentModule = *funcCursor;
    interpHooks.node = node;
    interp.hooks = &interpHooks;

    // Add global & module namespaces.
    interp.outerNameSpaces.append(mi->globalNamespace);
    ModuleNamespace moduleNamespace;
    interp.outerNameSpaces.append(&moduleNamespace);

    // Invoke module function.
    crowbar::Interpreter::StackFrame frame;
    frame.interp = &interp;
    frame.desc = {[](const crowbar::Statement::CustomBlock* moduleDef) -> HybridString {
                      return String::format("module '{}'",
                                            LabelMap::instance.view(moduleDef->name));
                  },
                  moduleDef};
    MethodResult result = execFunction(&frame, moduleDef->body);
    if (result == MethodResult::Error) {
        StdErr::text() << outs.moveToString();
        return nullptr;
    }

    mi->modules.find(moduleName)->statusInCurrentConfig = ModuleInstantiator::Instantiated;
    return node;
}

} // namespace latest
} // namespace build
} // namespace ply
