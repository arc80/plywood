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

struct ExtendedParser {
    crowbar::Parser* parser = nullptr;
    LabelMap<Functor<crowbar::CustomBlockHandler>> customBlocks;
    LabelMap<Functor<crowbar::ExpressionTraitHandler>> exprTraits;

    // Current Plyfile and module being parsed:
    Repository::Plyfile* currentPlyfile = nullptr;
    Repository::Module* currentModule = nullptr;
};

bool parseModuleLikeBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                          crowbar::StatementBlock* stmtBlock) {
    // module/executable { ... } block
    if (ep->parser->context.customBlock || ep->parser->context.func) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              String::format("{} must be defined at file scope", kwToken.text));
        ep->parser->recovery.muteErrors = false;
    }

    // Create new Repository::Module
    auto ownedMod = Owned<Repository::Module>::create();
    Repository::Module* mod = ownedMod;
    mod->plyfile = ep->currentPlyfile;
    mod->fileOffset = kwToken.fileOffset;

    auto moduleStmt = Owned<crowbar::Statement>::create();
    mod->block = moduleStmt->customBlock().switchTo().get();
    mod->block->type = kwToken.label;

    // Parse module name.
    crowbar::ExpandedToken nameToken = ep->parser->tkr->readToken();
    if (nameToken.type == crowbar::TokenType::Identifier) {
        mod->block->name = nameToken.label;
    } else {
        error(ep->parser, nameToken, crowbar::ErrorTokenAction::PushBack,
              String::format("expected {} name after '{}'; got {}", kwToken.text, kwToken.text,
                             nameToken.desc()));
    }

    // Add to Repository
    auto cursor = Repository::instance->moduleMap.insertOrFind(mod->block->name);
    if (cursor.wasFound()) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              String::format("'{}' was already defined as {}", kwToken.text,
                             g_labelStorage.view((*cursor)->block->type)));
        ep->parser->recovery.muteErrors = false;
        ep->parser->errorOut->format(
            "{}: ... see previous definition\n",
            (*cursor)->plyfile->tkr.fileLocationMap.formatFileLocation((*cursor)->fileOffset));
    }
    (*cursor) = std::move(ownedMod);

    // Parse nested block.
    PLY_SET_IN_SCOPE(ep->currentModule, mod);
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, mod->block);
    mod->block->body =
        parseStatementBlock(ep->parser, {kwToken.text, kwToken.text + " name", true});
    stmtBlock->statements.append(std::move(moduleStmt));
    return true;
}

bool parseSourceFilesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                           crowbar::StatementBlock* stmtBlock) {
    if (!ep->currentModule) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "source_files block can only be used within a module or executable block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->sourceFilesKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"source_files", "source_files", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseIncludeDirectoriesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                                  crowbar::StatementBlock* stmtBlock) {
    if (!ep->currentModule) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "include_directories block can only be used within a module or executable "
              "block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->includeDirectoriesKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body =
        parseStatementBlock(ep->parser, {"include_directories", "include_directories", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseDependenciesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                            crowbar::StatementBlock* stmtBlock) {
    // dependencies { ... } block
    if (!ep->currentModule) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "dependencies block can only be used within a module or executable block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->dependenciesKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"dependencies", "dependencies", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseLinkLibrariesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                             crowbar::StatementBlock* stmtBlock) {
    if (!ep->currentModule) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "link_libraries block can only be used within a module or executable block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->linkLibrariesKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"link_libraries", "link_libraries", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseConfigOptionsBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                             crowbar::StatementBlock* stmtBlock) {
    if (ep->currentModule) {
        if (ep->currentModule->configBlock) {
            error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                  String::format("{} '{}' already has a config_options block",
                                 g_labelStorage.view(ep->currentModule->block->type),
                                 g_labelStorage.view(ep->currentModule->block->name)));
            ep->parser->recovery.muteErrors = false;
            ep->parser->errorOut->format(
                "{}: see previous definition\n",
                ep->currentModule->plyfile->tkr.fileLocationMap.formatFileLocation(
                    ep->currentModule->configBlock->fileOffset));
        }
    } else {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "config_options block can only be used within a module or executable block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configOptionsKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"config_options", "config_options", true});
    if (ep->currentModule) {
        ep->currentModule->configBlock = std::move(customBlock);
    }
    return true;
}

bool parseConfigListBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                          crowbar::StatementBlock* stmtBlock) {
    if (ep->parser->context.customBlock || ep->parser->context.func) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "conflig_list must be defined at file scope");
        ep->parser->recovery.muteErrors = false;
    }

    Repository::ConfigList* configList = Repository::instance->configList;
    if (configList) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "a conflig_list was already defined");
        ep->parser->recovery.muteErrors = false;
        ep->parser->errorOut->format(
            "{}: see previous definition\n",
            configList->plyfile->tkr.fileLocationMap.formatFileLocation(configList->fileOffset));
    }

    Repository::instance->configList = Owned<Repository::ConfigList>::create();
    configList = Repository::instance->configList;
    configList->plyfile = ep->currentPlyfile;
    configList->fileOffset = kwToken.fileOffset;

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configListKey;
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    configList->block = parseStatementBlock(ep->parser, {"config_list", "config_list", true});

    return true;
}

bool parseConfigBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                      crowbar::StatementBlock* stmtBlock) {
    crowbar::Statement::CustomBlock* configListBlock = nullptr;
    if (ep->parser->context.customBlock &&
        (ep->parser->context.customBlock->type == g_common->configListKey)) {
        configListBlock = ep->parser->context.customBlock;
    }

    if (!configListBlock) {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              "config must be defined inside a config_list block");
        ep->parser->recovery.muteErrors = false;
    }

    Owned<crowbar::Expression> expr = ep->parser->parseExpression();

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configKey;
    cb->expr = std::move(expr);
    PLY_SET_IN_SCOPE(ep->parser->context.customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"config", "config", true});

    if (configListBlock) {
        stmtBlock->statements.append(std::move(customBlock));
    }
    return true;
}

bool parsePublicPrivateExpressionTrait(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                                       AnyOwnedObject* expressionTraits) {
    bool legal = false;
    if (crowbar::Statement::CustomBlock* cb = ep->parser->context.customBlock) {
        legal = (cb->type == g_common->includeDirectoriesKey) ||
                (cb->type == g_common->dependenciesKey);
    }
    if (legal) {
        if (!expressionTraits->data) {
            *expressionTraits = AnyOwnedObject::create<ExpressionTraits>();
        }
        auto traits = expressionTraits->cast<ExpressionTraits>();
        traits->visibilityTokenIdx = kwToken.tokenIdx;
        traits->isPublic = (kwToken.label == g_common->publicKey);
    } else {
        error(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
              String::format(
                  "'{}' can only be used inside an include_directories or dependencies block",
                  kwToken.text));
        ep->parser->recovery.muteErrors = false;
    }
    return true;
}

bool parsePlyfile(StringView path) {
    Repository* repo = Repository::instance;
    String src = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile = repo->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->tkr.setSourceInput(path, src);
    crowbar::Parser parser;

    // Extend the parser
    ExtendedParser ph;
    ph.parser = &parser;
    ph.currentPlyfile = plyfile;
    *ph.customBlocks.insert(g_common->moduleKey) = {parseModuleLikeBlock, &ph};
    *ph.customBlocks.insert(g_common->executableKey) = {parseModuleLikeBlock, &ph};
    *ph.customBlocks.insert(g_common->externKey) = {parseModuleLikeBlock, &ph};
    *ph.customBlocks.insert(g_common->sourceFilesKey) = {parseSourceFilesBlock, &ph};
    *ph.customBlocks.insert(g_common->includeDirectoriesKey) = {parseIncludeDirectoriesBlock, &ph};
    *ph.customBlocks.insert(g_common->dependenciesKey) = {parseDependenciesBlock, &ph};
    *ph.customBlocks.insert(g_common->linkLibrariesKey) = {parseLinkLibrariesBlock, &ph};
    *ph.customBlocks.insert(g_common->configOptionsKey) = {parseConfigOptionsBlock, &ph};
    *ph.customBlocks.insert(g_common->configListKey) = {parseConfigListBlock, &ph};
    *ph.customBlocks.insert(g_common->configKey) = {parseConfigBlock, &ph};
    *ph.exprTraits.insert(g_common->publicKey) = {parsePublicPrivateExpressionTrait, &ph};
    *ph.exprTraits.insert(g_common->privateKey) = {parsePublicPrivateExpressionTrait, &ph};
    parser.customBlockHandlers = &ph.customBlocks;
    parser.exprTraitHandlers = &ph.exprTraits;

    parser.tkr = &plyfile->tkr;
    OutStream errorOut = StdErr::text();
    parser.errorOut = &errorOut;

    // Parse the script and check for errors.
    Owned<crowbar::StatementBlock> file = parser.parseFile();
    if (parser.errorCount > 0) {
        return false;
    }

    // Success.
    plyfile->contents = std::move(file);
    return true;
}

} // namespace latest
} // namespace build
} // namespace ply
