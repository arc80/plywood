/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Repository.h>
#include <ply-crowbar/Parser.h>

namespace ply {
namespace build {
namespace latest {

// ParseHooks implements hooks that are used to extend the parser. Recognizes custom blocks like
// 'module' and expression traits like 'public'.
struct ParseHooks : crowbar::Parser::Hooks {
    crowbar::Parser* parser = nullptr;
    StringView srcFilePath;

    // These data members are modified by the parser:
    Array<crowbar::Statement::CustomBlock*> moduleBlocks;
    MemOutStream errorOut;
    u32 errorCount = 0;

    bool isInModuleOrExecutableBlock() const {
        Label type = this->parser->context.customBlock->type;
        const Common* common = &Repository::instance->common;
        return (type == common->moduleKey) || (type == common->executableKey);
    }

    virtual bool tryParseCustomBlock(crowbar::StatementBlock* stmtBlock) override {
        const Common* common = &Repository::instance->common;

        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

        if ((kwToken.label == common->moduleKey) ||
            (kwToken.label == common->executableKey)) {
            // module/executable { ... } block
            if (this->parser->context.customBlock || this->parser->context.func) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      String::format("{} must be defined at file scope", kwToken.text));
                this->parser->recovery.muteErrors = false;
            }
            auto moduleStmt = Owned<crowbar::Statement>::create();
            auto* customBlock = moduleStmt->customBlock().switchTo().get();
            customBlock->type = kwToken.label;

            // Parse module name.
            crowbar::ExpandedToken nameToken = parser->tkr->readToken();
            if (nameToken.type == crowbar::TokenType::Identifier) {
                customBlock->name = nameToken.label;
            } else {
                error(parser, nameToken, crowbar::ErrorTokenAction::PushBack,
                      String::format("expected {} name after '{}'; got {}", kwToken.text,
                                     kwToken.text, nameToken.desc()));
            }

            // Parse nested block.
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, customBlock);
            customBlock->body = parseStatementBlock(
                this->parser, {kwToken.text, kwToken.text + " module name", true});
            stmtBlock->statements.append(std::move(moduleStmt));
            this->moduleBlocks.append(customBlock);
            return true;
        }

        if (kwToken.label == common->sourceFilesKey) {
            // source_files { ... } block
            if (!this->isInModuleOrExecutableBlock()) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "source_files block can only be used within a module or executable block");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->sourceFilesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser, {"source_files", "source_files", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == common->includeDirectoriesKey) {
            // include_directories { ... } block
            if (!this->isInModuleOrExecutableBlock()) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "include_directories block can only be used within a module or executable "
                      "block");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->includeDirectoriesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser,
                                           {"include_directories", "include_directories", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == common->dependenciesKey) {
            // dependencies { ... } block
            if (!this->isInModuleOrExecutableBlock()) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "dependencies block can only be used within a module or executable block");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->dependenciesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser, {"dependencies", "dependencies", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == common->linkLibrariesKey) {
            // dependencies { ... } block
            if (!this->isInModuleOrExecutableBlock()) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "link_libraries block can only be used within a module or executable block");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = common->linkLibrariesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body =
                parseStatementBlock(this->parser, {"link_libraries", "link_libraries", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        this->parser->tkr->rewindTo(kwToken.tokenIdx);
        return false;
    }

    virtual bool tryParseExpressionTrait(AnyOwnedObject* expressionTraits) override {
        const Common* common = &Repository::instance->common;

        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

        if ((kwToken.label == common->publicKey) ||
            (kwToken.label == common->privateKey)) {
            // public / private keywords
            bool legal = false;
            if (crowbar::Statement::CustomBlock* cb = this->parser->context.customBlock) {
                legal = (cb->type == common->includeDirectoriesKey) ||
                        (cb->type == common->dependenciesKey);
            }
            if (legal) {
                if (!expressionTraits->data) {
                    *expressionTraits = AnyOwnedObject::create<ExpressionTraits>();
                }
                auto traits = expressionTraits->cast<ExpressionTraits>();
                traits->visibilityTokenIdx = kwToken.tokenIdx;
                traits->isPublic = (kwToken.label == common->publicKey);
            } else {
                error(
                    this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                    String::format(
                        "'{}' can only be used inside an include_directories or dependencies block",
                        kwToken.text));
                this->parser->recovery.muteErrors = false;
            }
            return true;
        }

        this->parser->tkr->rewindTo(kwToken.tokenIdx);
        return false;
    }

    virtual void onError(StringView errorMsg) override {
        this->errorOut << this->srcFilePath << errorMsg;
        this->errorCount++;
    }
};

bool parsePlyfile(StringView path) {
    Repository* repo = Repository::instance;
    String src = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile = repo->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->path = path;
    plyfile->tkr.setSourceInput(src);
    crowbar::Parser parser;
    ParseHooks parserHooks;
    parserHooks.parser = &parser;
    parserHooks.srcFilePath = path;
    parser.hooks = &parserHooks;
    parser.tkr = &plyfile->tkr;

    // Parse the script and check for errors.
    Owned<crowbar::StatementBlock> file = parser.parseFile();
    if (parserHooks.errorCount > 0) {
        StdErr::text().write(parserHooks.errorOut.moveToString());
        return false;
    }

    // Success. Add parsed modules to the repository.
    plyfile->contents = std::move(file);
    for (crowbar::Statement::CustomBlock* customBlock : parserHooks.moduleBlocks) {
        auto cursor = repo->moduleMap.insertOrFind(customBlock->name);
        if (cursor.wasFound()) {
            PLY_FORCE_CRASH(); // Duplicate name
        }
        *cursor = Owned<Repository::Module>::create();
        (*cursor)->plyfile = plyfile;
        (*cursor)->block = customBlock;
    }

    return true;
}

} // namespace latest
} // namespace build
} // namespace ply
