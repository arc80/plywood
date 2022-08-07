/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/Interpreter.h>
#include <buildSteps/BuildSteps.h>
#include <ply-runtime/algorithm/Find.h>

using namespace ply;
using namespace ply::crowbar;

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

namespace buildSystem {
namespace scripting {

// Common contains data that is shared between the parser and the interpreter.
struct Common {
    // String keys for all the extra keywords used by build system scripts: 'module',
    // 'include_directories', 'dependencies', etc.
    Label moduleKey;
    Label includeDirectoriesKey;
    Label sourceFilesKey;
    Label publicKey;
    Label privateKey;

    Common() {
        this->moduleKey = LabelMap::instance.insertOrFind("module");
        this->includeDirectoriesKey = LabelMap::instance.insertOrFind("include_directories");
        this->sourceFilesKey = LabelMap::instance.insertOrFind("source_files");
        this->publicKey = LabelMap::instance.insertOrFind("public");
        this->privateKey = LabelMap::instance.insertOrFind("private");
    }
};

//--------------------------------------------------------

struct Repository {
    struct Plyfile {
        String path;
        Tokenizer tkr; // For tokenData and FileLocationMap (only)
        Owned<StatementBlock> contents;
    };

    struct Module {
        Plyfile* plyfile;
        Statement::CustomBlock* block;
    };

    struct ModuleMapTraits {
        using Key = Label;
        using Item = Owned<Module>;
        static bool match(const Item& item, Label name) {
            return item->block->name == name;
        }
    };

    Common common;
    Array<Owned<Plyfile>> plyfiles;
    HashMap<ModuleMapTraits> moduleMap;
};

//--------------------------------------------------------

// ExpressionTraits is a type of object created at parse time (via ParseHooks) and consumed by
// the interpreter (via InterpreterHooks). It contains extra information about each entry inside
// a 'include_directories' or 'dependencies' block.
struct ExpressionTraits {
    PLY_REFLECT()
    s32 visibilityTokenIdx = -1;
    bool isPublic = false;
    // ply reflect off
};

// ParseHooks implements hooks that are used to extend the parser. Recognizes custom blocks like
// 'module' and expression traits like 'public'.
struct ParseHooks : Parser::Hooks {
    Parser* parser = nullptr;
    Repository* repo = nullptr;
    Repository::Plyfile* plyfile = nullptr;
    MemOutStream errorOut;
    u32 errorCount = 0;

    virtual bool tryParseCustomBlock(StatementBlock* stmtBlock) override {
        Common* common = &this->repo->common;
        ExpandedToken kwToken = this->parser->tkr->readToken();

        if (kwToken.label == common->moduleKey) {
            // module { ... } block
            if (this->parser->context.customBlock || this->parser->context.func) {
                error(this->parser, kwToken, ErrorTokenAction::DoNothing,
                      "modules must be defined at file scope");
                this->parser->recovery.muteErrors = false;
            }
            auto moduleStmt = Owned<Statement>::create();
            auto* customBlock = moduleStmt->customBlock().switchTo().get();
            customBlock->type = common->moduleKey;

            // Parse module name.
            ExpandedToken nameToken = parser->tkr->readToken();
            if (nameToken.type == TokenType::Identifier) {
                customBlock->name = nameToken.label;
            } else {
                error(parser, nameToken, ErrorTokenAction::PushBack,
                      String::format("expected module name after 'module'; got {}",
                                     nameToken.desc()));
            }

            // Parse nested block.
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, customBlock);
            customBlock->body = parseStatementBlock(this->parser, {"module", "module name", true});
            // Note: Adding the module block to stmtBlock->statements is not that useful, but we
            // currently do it so that stmtBlock is the owner of the module. We could instead the
            // Repository the owner, and even store the module in something other than a
            // CustomBlock.
            stmtBlock->statements.append(std::move(moduleStmt));

            // Add to module map.
            auto cursor = this->repo->moduleMap.insertOrFind(customBlock->name);
            if (cursor.wasFound()) {
                PLY_FORCE_CRASH(); // Duplicate name
            }
            *cursor = Owned<Repository::Module>::create();
            (*cursor)->plyfile = this->plyfile;
            (*cursor)->block = customBlock;

            return true;
        }

        if (kwToken.label == common->sourceFilesKey) {
            // source_files { ... } block
            if (this->parser->context.customBlock->type != common->moduleKey) {
                error(this->parser, kwToken, ErrorTokenAction::DoNothing,
                      "source_files block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->sourceFilesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser, {"source_files", "source_files", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == common->includeDirectoriesKey) {
            // include_directories { ... } block
            if (this->parser->context.customBlock->type != common->moduleKey) {
                error(this->parser, kwToken, ErrorTokenAction::DoNothing,
                      "include_directories block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->includeDirectoriesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser,
                                           {"include_directories", "include_directories", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        this->parser->tkr->rewindTo(kwToken.tokenIdx);
        return false;
    }

    virtual bool tryParseExpressionTrait(AnyOwnedObject* expressionTraits) override {
        Common* common = &this->repo->common;
        ExpandedToken kwToken = this->parser->tkr->readToken();

        if ((kwToken.label == common->publicKey) || (kwToken.label == common->privateKey)) {
            // public / private keywords
            if (!expressionTraits->data) {
                *expressionTraits = AnyOwnedObject::create<ExpressionTraits>();
            }
            auto traits = expressionTraits->cast<ExpressionTraits>();
            traits->visibilityTokenIdx = kwToken.tokenIdx;
            traits->isPublic = (kwToken.label == common->publicKey);
            return true;
        }

        this->parser->tkr->rewindTo(kwToken.tokenIdx);
        return false;
    }

    virtual void onError(StringView errorMsg) override {
        this->errorOut << errorMsg;
        this->errorCount++;
    }
};

bool parsePlyfile(Repository* repo, StringView path) {
    String src = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile = repo->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->path = path;
    plyfile->tkr.setSourceInput(src);
    Parser parser;
    buildSystem::scripting::ParseHooks parserHooks;
    parserHooks.parser = &parser;
    parserHooks.repo = repo;
    parserHooks.plyfile = plyfile;
    parser.hooks = &parserHooks;
    parser.tkr = &plyfile->tkr;

    // Parse the script. This adds modules to the Repository.
    Owned<StatementBlock> file = parser.parseFile();
    if (parserHooks.errorCount > 0) {
        StdErr::text().write(parserHooks.errorOut.moveToString());
        return false;
    }
    plyfile->contents = std::move(file);
    return true;
}

//--------------------------------------------------------------

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
struct InterpreterHooks : Interpreter::Hooks {
    Interpreter* interp = nullptr;
    ModuleInstantiator* mi = nullptr;
    buildSteps::Node* node = nullptr;
    buildSteps::Node::Config* nodeConfig = nullptr;

    virtual ~InterpreterHooks() override {
    }
    virtual void enterCustomBlock(const Statement::CustomBlock* enteringBlock) override {
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
    Interpreter interp;
    interp.outs = &outs;
    interp.error = [](BaseInterpreter* base, StringView message) {
        Interpreter* interp = static_cast<Interpreter*>(base);
        interp->outs->format("error: {}\n", message);
        bool first = true;
        for (Interpreter::StackFrame* frame = interp->currentFrame; frame;
             frame = frame->prevFrame) {
            ExpandedToken expToken = interp->tkr->expandToken(frame->tokenIdx);
            interp->outs->format(
                "{} {} {}\n", interp->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                first ? "in" : "called from", frame->desc());
            first = false;
        }
    };
    interp.tkr = &(*funcCursor)->plyfile->tkr;
    HashMap<VariableMapTraits> globalNamespace;
    globalNamespace.insertOrFind(LabelMap::instance.insertOrFind("join_path"))->obj =
        AnyObject::bind(doJoinPath);
    String buildFolder = "C:/Jeff/plywood-fresh/plywood/data/build/delete_me";
    globalNamespace.insertOrFind(LabelMap::instance.insertOrFind("build_folder"))->obj =
        AnyObject::bind(&buildFolder);
    interp.outerNameSpaces.append(&globalNamespace);

    // Extend the interpreter with support for custom blocks and expression attributes used by the
    // build system.
    buildSystem::scripting::InterpreterHooks interpHooks;
    interpHooks.interp = &interp;
    interpHooks.mi = mi;
    interpHooks.node = node;
    interpHooks.nodeConfig = &node->configs[mi->currentConfig];
    interp.hooks = &interpHooks;

    // Create built-in namespace of objects that module functions use.
    HashMap<VariableMapTraits> builtins;
    String configName = "Debug";
    builtins.insertOrFind(LabelMap::instance.insertOrFind("config"))->obj =
        AnyObject::bind(&configName);

    // Invoke module function.
    Interpreter::StackFrame frame;
    frame.interp = &interp;
    const Statement::CustomBlock* moduleDef = (*funcCursor)->block;
    PLY_ASSERT(moduleDef->type == common->moduleKey);
    frame.desc = {[](const Statement::CustomBlock* moduleDef) -> HybridString {
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

bool test() {
    Repository repo;
    String plyfilePath =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/platform/Plyfile");
    if (!parsePlyfile(&repo, plyfilePath))
        return false;

    ModuleInstantiator mi;
    mi.repo = &repo;
    mi.project.name = "Test";

    // Add configs
    mi.project.configs.resize(3);
    mi.project.configs[0].name = "Debug";
    mi.project.configs[1].name = "RelWithAsserts";
    mi.project.configs[2].name = "RelWithDebInfo";

    // For each config, instantiate root modules
    for (u32 i = 0; i < mi.project.configs.numItems(); i++) {
        mi.currentConfig = i;
        if (!instantiateModuleForCurrentConfig(&mi, LabelMap::instance.find("platform")))
            return false;
    }

    Owned<buildSteps::MetaProject> mp = expand(&mi.project);
    MemOutStream outs;
    writeCMakeLists(&outs, mp);

    FileSystem::native()->makeDirsAndSaveTextIfDifferent("out.txt", outs.moveToString(),
                                                         TextFormat::platformPreference());

    return true;
}

} // namespace scripting
} // namespace buildSystem

int main() {
    return buildSystem::scripting::test() ? 0 : 1;
}

#include "codegen/Main.inl"
