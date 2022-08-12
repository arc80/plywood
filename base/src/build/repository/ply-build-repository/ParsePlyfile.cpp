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
    Repository* repo = nullptr;
    Repository::Plyfile* plyfile = nullptr;
    MemOutStream errorOut;
    u32 errorCount = 0;

    virtual bool tryParseCustomBlock(crowbar::StatementBlock* stmtBlock) override {
        Common* common = &this->repo->common;
        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

        if (kwToken.label == common->moduleKey) {
            // module { ... } block
            if (this->parser->context.customBlock || this->parser->context.func) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "modules must be defined at file scope");
                this->parser->recovery.muteErrors = false;
            }
            auto moduleStmt = Owned<crowbar::Statement>::create();
            auto* customBlock = moduleStmt->customBlock().switchTo().get();
            customBlock->type = common->moduleKey;

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
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "source_files block can only be used within a module");
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
            if (this->parser->context.customBlock->type != common->moduleKey) {
                error(this->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                      "include_directories block can only be used within a module");
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

        this->parser->tkr->rewindTo(kwToken.tokenIdx);
        return false;
    }

    virtual bool tryParseExpressionTrait(AnyOwnedObject* expressionTraits) override {
        Common* common = &this->repo->common;
        crowbar::ExpandedToken kwToken = this->parser->tkr->readToken();

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
    parserHooks.repo = repo;
    parserHooks.plyfile = plyfile;
    parser.hooks = &parserHooks;
    parser.tkr = &plyfile->tkr;

    // Parse the script. This adds modules to the Repository.
    Owned<crowbar::StatementBlock> file = parser.parseFile();
    if (parserHooks.errorCount > 0) {
        StdErr::text().write(parserHooks.errorOut.moveToString());
        return false;
    }
    plyfile->contents = std::move(file);
    return true;
}

} // namespace latest
} // namespace build
} // namespace ply
