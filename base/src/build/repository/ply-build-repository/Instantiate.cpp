/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Interpreter.h>
#include <buildSteps/buildSteps.h>

namespace ply {
namespace build {
namespace latest {

struct ModuleInstantiator {
    enum Status { NotInstantiated, Instantiating, Instantiated };

    struct ModuleMapTraits {
        using Key = StringView;
        struct Item {
            Reference<buildSteps::Node> node;
            Status statusInCurrentConfig = NotInstantiated;
        };
        static bool match(const Item& item, StringView name) {
            return item.node->name == name;
        }
    };

    Repository* repo = nullptr;

    // The project is initialized by instantiating a set of root modules.
    buildSteps::Project project;

    // These members are only used while the proejct is being instantiated.
    HashMap<ModuleMapTraits> modules;
    u32 currentConfig = 0;
};

//--------------------------------------------------------------

// InterpreterHooks implements hooks that are used to extend the interpreter. Handles custom blocks
// like 'module' and expression traits like 'public'.
struct InterpreterHooks : crowbar::Interpreter::Hooks {
    crowbar::Interpreter* interp = nullptr;
    ModuleInstantiator* mi = nullptr;
    buildSteps::Node* node = nullptr;
    buildSteps::Node::Config* nodeConfig = nullptr;

    virtual ~InterpreterHooks() override {
    }
    virtual void enterCustomBlock(const crowbar::Statement::CustomBlock* enteringBlock) override {
        if (this->interp->customBlock) {
            PLY_FORCE_CRASH();
        }
    }
    virtual void onEvaluate(const AnyObject& evaluationTraits) override {
        const Common* common = &this->mi->repo->common;
        if (this->interp->customBlock) {
            Label blockType = this->interp->customBlock->type;
            if (blockType == common->sourceFilesKey) {
                PLY_ASSERT(!evaluationTraits.data);
                // FIXME: Only add the path once
                buildSteps::Node::SourceFiles& sf = this->node->sourceFiles.append();
                sf.root = *this->interp->returnValue.cast<String>();
            } else if (blockType == common->includeDirectoriesKey) {
                const ExpressionTraits* traits = evaluationTraits.cast<ExpressionTraits>();
                PLY_ASSERT(traits->visibilityTokenIdx >= 0);
                buildSteps::ToolchainOpt& tcOpt = this->nodeConfig->opts.tcOpts.append(
                    traits->isPublic ? buildSteps::Visibility::Public
                                     : buildSteps::Visibility::Private,
                    buildSteps::ToolchainOpt::IncludeDir,
                    *this->interp->returnValue.cast<String>());
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

buildSteps::Node* instantiateModuleForCurrentConfig(ModuleInstantiator* mi, Label moduleLabel) {
    Common* common = &mi->repo->common;
    StringView moduleName = LabelMap::instance.view(moduleLabel);

    // Check if a buildSteps::Node was already created for this moduleName.
    buildSteps::Node* node;
    {
        auto instCursor = mi->modules.insertOrFind(moduleName);
        if (!instCursor.wasFound()) {
            // No. Create a new one.
            instCursor->node = new buildSteps::Node;
            instCursor->node->name = moduleName;
            instCursor->node->configs.resize(mi->project.configs.numItems());
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

    // Find module function by name.
    auto funcCursor = mi->repo->moduleMap.find(moduleLabel);
    if (!funcCursor.wasFound()) {
        PLY_FORCE_CRASH();
    }

    // Create Crowbar interpreter.
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
    HashMap<crowbar::VariableMapTraits> globalNamespace;
    globalNamespace.insertOrFind(LabelMap::instance.insertOrFind("join_path"))->obj =
        AnyObject::bind(doJoinPath);
    String buildFolder = "C:/Jeff/plywood-fresh/plywood/data/build/delete_me";
    globalNamespace.insertOrFind(LabelMap::instance.insertOrFind("build_folder"))->obj =
        AnyObject::bind(&buildFolder);
    interp.outerNameSpaces.append(&globalNamespace);

    // Extend the interpreter with support for custom blocks and expression attributes used by the
    // build system.
    InterpreterHooks interpHooks;
    interpHooks.interp = &interp;
    interpHooks.mi = mi;
    interpHooks.node = node;
    interpHooks.nodeConfig = &node->configs[mi->currentConfig];
    interp.hooks = &interpHooks;

    // Create built-in namespace of objects that module functions use.
    HashMap<crowbar::VariableMapTraits> builtins;
    String configName = "Debug";
    builtins.insertOrFind(LabelMap::instance.insertOrFind("config"))->obj =
        AnyObject::bind(&configName);

    // Invoke module function.
    crowbar::Interpreter::StackFrame frame;
    frame.interp = &interp;
    const crowbar::Statement::CustomBlock* moduleDef = (*funcCursor)->block;
    PLY_ASSERT(moduleDef->type == common->moduleKey);
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
