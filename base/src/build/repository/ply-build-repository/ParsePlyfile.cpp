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
    const Common* common = nullptr;
    StringView srcFilePath;

    // These data members are modified by the parser:
    Array<crowbar::Statement::CustomBlock*> moduleBlocks;
    MemOutStream errorOut;
    u32 errorCount = 0;

    virtual bool tryParseCustomBlock(crowbar::StatementBlock* stmtBlock) override {
        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

        if (kwToken.label == this->common->moduleKey) {
            // module { ... } block
            if (this->parser->context.customBlock || this->parser->context.func) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "modules must be defined at file scope");
                this->parser->recovery.muteErrors = false;
            }
            auto moduleStmt = Owned<crowbar::Statement>::create();
            auto* customBlock = moduleStmt->customBlock().switchTo().get();
            customBlock->type = this->common->moduleKey;

            // Parse module name.
            crowbar::ExpandedToken nameToken = parser->tkr->readToken();
            if (nameToken.type == crowbar::TokenType::Identifier) {
                customBlock->name = nameToken.label;
            } else {
                error(parser, nameToken, crowbar::ErrorTokenAction::PushBack,
                      String::format("expected module name after 'module'; got {}",
                                     nameToken.desc()));
            }

            // Parse nested block.
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, customBlock);
            customBlock->body = parseStatementBlock(this->parser, {"module", "module name", true});
            stmtBlock->statements.append(std::move(moduleStmt));
            this->moduleBlocks.append(customBlock);
            return true;
        }

        if (kwToken.label == this->common->sourceFilesKey) {
            // source_files { ... } block
            if (this->parser->context.customBlock->type != this->common->moduleKey) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "source_files block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = this->common->sourceFilesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser, {"source_files", "source_files", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == this->common->includeDirectoriesKey) {
            // include_directories { ... } block
            if (this->parser->context.customBlock->type != this->common->moduleKey) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "include_directories block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = this->common->includeDirectoriesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser,
                                           {"include_directories", "include_directories", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == this->common->dependenciesKey) {
            // dependencies { ... } block
            if (this->parser->context.customBlock->type != this->common->moduleKey) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "dependencies block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = this->common->dependenciesKey;
            PLY_SET_IN_SCOPE(this->parser->context.customBlock, cb);
            cb->body = parseStatementBlock(this->parser, {"dependencies", "dependencies", true});
            stmtBlock->statements.append(std::move(customBlock));
            return true;
        }

        if (kwToken.label == this->common->linkLibrariesKey) {
            // dependencies { ... } block
            if (this->parser->context.customBlock->type != this->common->moduleKey) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "link_libraries block can only be used within a module");
                this->parser->recovery.muteErrors = false;
            }

            auto customBlock = Owned<crowbar::Statement>::create();
            auto* cb = customBlock->customBlock().switchTo().get();
            cb->type = this->common->linkLibrariesKey;
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
        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

        if ((kwToken.label == this->common->publicKey) ||
            (kwToken.label == this->common->privateKey)) {
            // public / private keywords
            bool legal = false;
            if (crowbar::Statement::CustomBlock* cb = this->parser->context.customBlock) {
                legal = (cb->type == this->common->includeDirectoriesKey) ||
                        (cb->type == this->common->dependenciesKey);
            }
            if (legal) {
                if (!expressionTraits->data) {
                    *expressionTraits = AnyOwnedObject::create<ExpressionTraits>();
                }
                auto traits = expressionTraits->cast<ExpressionTraits>();
                traits->visibilityTokenIdx = kwToken.tokenIdx;
                traits->isPublic = (kwToken.label == this->common->publicKey);
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
    parserHooks.common = &repo->common;
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
