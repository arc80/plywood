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

    // Information about the current parsing context:
    Repository::Plyfile* currentPlyfile = nullptr;
    Repository::Module* currentModule = nullptr;
    crowbar::Statement::CustomBlock* customBlock = nullptr;
};

bool parseModuleLikeBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                          crowbar::StatementBlock* stmtBlock) {
    // module/executable/extern { ... } block
    if (ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
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
        errorAtToken(ep->parser, nameToken, crowbar::ErrorTokenAction::PushBack,
                     String::format("expected {} name after '{}'; got {}", kwToken.text,
                                    kwToken.text, nameToken.desc()));
    }

    // Add to Repository
    auto cursor = g_repository->moduleMap.insertOrFind(mod->block->name);
    if (cursor.wasFound()) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     String::format("'{}' was already defined as {}", kwToken.text,
                                    g_labelStorage.view((*cursor)->block->type)));
        ep->parser->recovery.muteErrors = false;
        ep->parser->error(String::format(
            "{}: ... see previous definition\n",
            (*cursor)->plyfile->tkr.fileLocationMap.formatFileLocation((*cursor)->fileOffset)));
    }
    (*cursor) = std::move(ownedMod);

    // Parse nested block.
    PLY_SET_IN_SCOPE(ep->parser->functionLikeScope, moduleStmt);
    PLY_SET_IN_SCOPE(ep->currentModule, mod);
    mod->block->body =
        parseStatementBlock(ep->parser, {kwToken.text, kwToken.text + " name", true});
    stmtBlock->statements.append(std::move(moduleStmt));
    return true;
}

bool parseSourceFilesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                           crowbar::StatementBlock* stmtBlock) {
    if (!ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "source_files block can only be used within a function, module, executable or "
                     "extern block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->sourceFilesKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"source_files", "source_files", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseIncludeDirectoriesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                                  crowbar::StatementBlock* stmtBlock) {
    // include_directories { ... } block
    if (!ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "include_directories block can only be used within a function, module, "
                     "executable or extern block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->includeDirectoriesKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body =
        parseStatementBlock(ep->parser, {"include_directories", "include_directories", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseDependenciesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                            crowbar::StatementBlock* stmtBlock) {
    // dependencies { ... } block
    if (!ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "dependencies block can only be used within a function, module, executable or "
                     "extern block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->dependenciesKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"dependencies", "dependencies", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseLinkLibrariesBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                             crowbar::StatementBlock* stmtBlock) {
    // link_libraries { ... } block
    if (!ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "link_libraries block can only be used within a function, module, executable "
                     "or extern block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->linkLibrariesKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"link_libraries", "link_libraries", true});
    stmtBlock->statements.append(std::move(customBlock));
    return true;
}

bool parseConfigOptionsBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                             crowbar::StatementBlock* stmtBlock) {
    // config_options { ... } block
    if (!ep->currentModule) {
        errorAtToken(
            ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
            "config_options block can only be used within a module, executable or extern block");
        ep->parser->recovery.muteErrors = false;
    }

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    crowbar::Statement::CustomBlock* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configOptionsKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"config_options", "config_options", true});
    if (ep->currentModule) {
        g_repository->moduleConfigBlocks.append({ep->currentModule, std::move(customBlock)});
    }
    return true;
}

bool parseConfigListBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                          crowbar::StatementBlock* stmtBlock) {
    if (ep->parser->functionLikeScope) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "conflig_list must be defined at file scope");
        ep->parser->recovery.muteErrors = false;
    }

    Repository::ConfigList* configList = g_repository->configList;
    if (configList) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "a conflig_list was already defined");
        ep->parser->recovery.muteErrors = false;
        ep->parser->error(String::format(
            "{}: see previous definition\n",
            configList->plyfile->tkr.fileLocationMap.formatFileLocation(configList->fileOffset)));
    }

    g_repository->configList = Owned<Repository::ConfigList>::create();
    configList = g_repository->configList;
    configList->plyfile = ep->currentPlyfile;
    configList->fileOffset = kwToken.fileOffset;

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configListKey;
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    configList->block = parseStatementBlock(ep->parser, {"config_list", "config_list", true});

    return true;
}

bool parseConfigBlock(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                      crowbar::StatementBlock* stmtBlock) {
    crowbar::Statement::CustomBlock* configListBlock = nullptr;
    if (ep->customBlock && (ep->customBlock->type == g_common->configListKey)) {
        configListBlock = ep->customBlock;
    }

    if (!configListBlock) {
        errorAtToken(ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
                     "config must be defined inside a config_list block");
        ep->parser->recovery.muteErrors = false;
    }

    Owned<crowbar::Expression> expr = ep->parser->parseExpression();

    auto customBlock = Owned<crowbar::Statement>::create();
    customBlock->fileOffset = kwToken.fileOffset;
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = g_common->configKey;
    cb->expr = std::move(expr);
    PLY_SET_IN_SCOPE(ep->customBlock, cb);
    cb->body = parseStatementBlock(ep->parser, {"config", "config", true});

    if (configListBlock) {
        stmtBlock->statements.append(std::move(customBlock));
    }
    return true;
}

bool parsePublicPrivateExpressionTrait(ExtendedParser* ep, const crowbar::ExpandedToken& kwToken,
                                       AnyOwnedObject* expressionTraits) {
    bool isLegal = false;
    if (crowbar::Statement::CustomBlock* cb = ep->customBlock) {
        isLegal = (cb->type == g_common->includeDirectoriesKey) ||
                  (cb->type == g_common->dependenciesKey);
    }
    if (isLegal) {
        if (!expressionTraits->data) {
            *expressionTraits = AnyOwnedObject::create<ExpressionTraits>();
        }
        auto traits = expressionTraits->cast<ExpressionTraits>();
        traits->visibilityTokenIdx = kwToken.tokenIdx;
        traits->isPublic = (kwToken.label == g_common->publicKey);
    } else {
        errorAtToken(
            ep->parser, kwToken, crowbar::ErrorTokenAction::DoNothing,
            String::format(
                "'{}' can only be used inside an include_directories or dependencies block",
                kwToken.text));
        ep->parser->recovery.muteErrors = false;
    }
    return true;
}

bool parsePlyfile(StringView path) {
    String src = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile =
        g_repository->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->tkr.setSourceInput(path, src);
    crowbar::Parser parser;

    // Extend the parser
    ExtendedParser ep;
    ep.parser = &parser;
    ep.currentPlyfile = plyfile;
    *ep.customBlocks.insert(g_common->moduleKey) = {parseModuleLikeBlock, &ep};
    *ep.customBlocks.insert(g_common->executableKey) = {parseModuleLikeBlock, &ep};
    *ep.customBlocks.insert(g_common->externKey) = {parseModuleLikeBlock, &ep};
    *ep.customBlocks.insert(g_common->sourceFilesKey) = {parseSourceFilesBlock, &ep};
    *ep.customBlocks.insert(g_common->includeDirectoriesKey) = {parseIncludeDirectoriesBlock, &ep};
    *ep.customBlocks.insert(g_common->dependenciesKey) = {parseDependenciesBlock, &ep};
    *ep.customBlocks.insert(g_common->linkLibrariesKey) = {parseLinkLibrariesBlock, &ep};
    *ep.customBlocks.insert(g_common->configOptionsKey) = {parseConfigOptionsBlock, &ep};
    *ep.customBlocks.insert(g_common->configListKey) = {parseConfigListBlock, &ep};
    *ep.customBlocks.insert(g_common->configKey) = {parseConfigBlock, &ep};
    *ep.exprTraits.insert(g_common->publicKey) = {parsePublicPrivateExpressionTrait, &ep};
    *ep.exprTraits.insert(g_common->privateKey) = {parsePublicPrivateExpressionTrait, &ep};
    parser.customBlockHandlers = &ep.customBlocks;
    parser.exprTraitHandlers = &ep.exprTraits;

    parser.tkr = &plyfile->tkr;
    parser.error = [](StringView message) { StdErr::text() << message; };

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
