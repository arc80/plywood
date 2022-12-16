/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Repository.h>
#include <ply-biscuit/Parser.h>

namespace ply {
namespace build2 {

struct ExtendedParser {
    biscuit::Parser* parser = nullptr;
    Repository::Plyfile* currentPlyfile = nullptr;
    Repository::ModuleOrFunction* currentModule = nullptr;
};

Owned<biscuit::Statement> parseCustomBlock(ExtendedParser* ep,
                                           const biscuit::Parser::Filter& filter, Label type,
                                           StringView trailing = {}) {
    auto customBlock = Owned<biscuit::Statement>::create();
    auto* cb = customBlock->customBlock().switchTo().get();
    cb->type = type;
    PLY_SET_IN_SCOPE(ep->parser->outerScope, customBlock);
    PLY_SET_IN_SCOPE(ep->parser->filter, filter);
    StringView typeStr = g_labelStorage.view(type);
    cb->body = parseStatementBlock(ep->parser, {typeStr, trailing ? trailing : typeStr, true});
    return customBlock;
}

biscuit::KeywordResult handleKeywordInsideModuleOrFunction(ExtendedParser* ep,
                                                           const biscuit::KeywordParams& kp);
biscuit::KeywordResult handleKeywordInsideConfigList(ExtendedParser* ep,
                                                     const biscuit::KeywordParams& kp);
biscuit::KeywordResult handleKeywordPublicPrivate(ExtendedParser* ep,
                                                  const biscuit::KeywordParams& kp);

biscuit::KeywordResult handleKeywordAtFileScope(ExtendedParser* ep,
                                                const biscuit::KeywordParams& kp) {
    if ((kp.kwToken.label == g_common->moduleKey) ||
        (kp.kwToken.label == g_common->executableKey)) {
        // Create new Repository::Module
        auto ownedMod = Owned<Repository::ModuleOrFunction>::create();
        ownedMod->plyfile = ep->currentPlyfile;
        ownedMod->stmt = Owned<biscuit::Statement>::create();
        auto cb = ownedMod->stmt->customBlock().switchTo();
        cb->type = kp.kwToken.label;

        // Parse module name.
        biscuit::ExpandedToken nameToken = ep->parser->tkr->readToken();
        if (nameToken.type == biscuit::TokenType::Identifier) {
            cb->name = nameToken.label;
        } else {
            errorAtToken(ep->parser, nameToken, biscuit::ErrorTokenAction::PushBack,
                         String::format("expected {} name after '{}'; got {}", kp.kwToken.text,
                                        kp.kwToken.text, nameToken.desc()));
        }

        // Add to Repository
        Repository::ModuleOrFunction** prevMod = nullptr;
        if (!g_repository->globalScope.insertOrFind(cb->name, &prevMod)) {
            StringView type = "function";
            if (auto prevCB = (*prevMod)->stmt->customBlock()) {
                type = g_labelStorage.view(prevCB->type);
            }
            errorAtToken(ep->parser, kp.kwToken, biscuit::ErrorTokenAction::DoNothing,
                         String::format("'{}' was already defined as {}", kp.kwToken.text, type));
            ep->parser->recovery.muteErrors = false;
            ep->parser->error(
                String::format("{}: ... see previous definition\n",
                               (*prevMod)->plyfile->tkr.fileLocationMap.formatFileLocation(
                                   (*prevMod)->stmt->fileOffset)));
        }
        (*prevMod) = ownedMod;

        PLY_SET_IN_SCOPE(ep->currentModule, ownedMod);
        biscuit::Parser::Filter filter;
        filter.keywordHandler = {handleKeywordInsideModuleOrFunction, ep};
        filter.allowInstructions = true;
        ownedMod->stmt->customBlock()->body =
            std::move(parseCustomBlock(ep, filter, kp.kwToken.label, kp.kwToken.text + " name")
                          ->customBlock()
                          ->body);
        g_repository->modules.append(std::move(ownedMod));
        return biscuit::KeywordResult::Block;
    } else if (kp.kwToken.label == g_common->configListKey) {
        Repository::ConfigList* configList = g_repository->configList;
        if (configList) {
            errorAtToken(ep->parser, kp.kwToken, biscuit::ErrorTokenAction::DoNothing,
                         "a config_list was already defined");
            ep->parser->recovery.muteErrors = false;
            ep->parser->error(
                String::format("{}: see previous definition\n",
                               configList->plyfile->tkr.fileLocationMap.formatFileLocation(
                                   configList->fileOffset)));
        }

        g_repository->configList = Owned<Repository::ConfigList>::create();
        configList = g_repository->configList;
        configList->plyfile = ep->currentPlyfile;
        configList->fileOffset = kp.kwToken.fileOffset;

        biscuit::Parser::Filter filter;
        filter.keywordHandler = {handleKeywordInsideConfigList, ep};
        filter.allowInstructions = true;
        configList->blockStmt = parseCustomBlock(ep, filter, kp.kwToken.label);
        return biscuit::KeywordResult::Block;
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult handleKeywordInsideModuleOrFunction(ExtendedParser* ep,
                                                           const biscuit::KeywordParams& kp) {
    if ((kp.kwToken.label == g_common->sourceFilesKey) ||
        (kp.kwToken.label == g_common->includeDirectoriesKey) ||
        (kp.kwToken.label == g_common->preprocessorDefinitionsKey) ||
        (kp.kwToken.label == g_common->dependenciesKey) ||
        (kp.kwToken.label == g_common->linkLibrariesKey) ||
        (kp.kwToken.label == g_common->compileOptionsKey)) {
        biscuit::Parser::Filter filter;
        filter.keywordHandler = {handleKeywordPublicPrivate, ep};
        filter.allowInstructions = true;
        kp.stmtBlock->statements.append(parseCustomBlock(ep, filter, kp.kwToken.label));
        return biscuit::KeywordResult::Block;
    } else if (kp.kwToken.label == g_common->configOptionsKey) {
        if (ep->currentModule) {
            biscuit::Parser::Filter filter;
            filter.keywordHandler = [](const biscuit::KeywordParams&) {
                return biscuit::KeywordResult::Illegal;
            };
            filter.allowInstructions = true;
            Owned<biscuit::Statement> customBlock = parseCustomBlock(ep, filter, kp.kwToken.label);
            g_repository->moduleConfigBlocks.append({ep->currentModule, std::move(customBlock)});
            return biscuit::KeywordResult::Block;
        }
    } else if (kp.kwToken.label == g_common->generateKey) {
        if (ep->currentModule) {
            biscuit::Parser::Filter filter;
            filter.keywordHandler = [](const biscuit::KeywordParams&) {
                return biscuit::KeywordResult::Illegal;
            };
            filter.allowInstructions = true;
            ep->currentModule->generateBlock = parseCustomBlock(ep, filter, kp.kwToken.label);
            return biscuit::KeywordResult::Block;
        }
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult handleKeywordInsideConfigList(ExtendedParser* ep,
                                                     const biscuit::KeywordParams& kp) {
    if (kp.kwToken.label == g_common->configKey) {
        PLY_ASSERT(ep->parser->outerScope->customBlock()->type == g_common->configListKey);

        Owned<biscuit::Expression> expr = ep->parser->parseExpression();

        biscuit::Parser::Filter filter;
        filter.keywordHandler = {handleKeywordInsideModuleOrFunction, ep};
        filter.allowInstructions = true;
        Owned<biscuit::Statement> cb = parseCustomBlock(ep, filter, kp.kwToken.label);
        cb->customBlock()->expr = std::move(expr);
        kp.stmtBlock->statements.append(std::move(cb));
        return biscuit::KeywordResult::Block;
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult handleKeywordPublicPrivate(ExtendedParser* ep,
                                                  const biscuit::KeywordParams& kp) {
    auto cb = ep->parser->outerScope->customBlock();

    bool isLegal = (cb->type == g_common->includeDirectoriesKey) ||
                   (cb->type == g_common->preprocessorDefinitionsKey) ||
                   (cb->type == g_common->dependenciesKey) ||
                   (cb->type == g_common->compileOptionsKey);
    isLegal = isLegal && ((kp.kwToken.label == g_common->publicKey) ||
                          (kp.kwToken.label == g_common->privateKey));
    if (isLegal) {
        if (!kp.attributes->data) {
            *kp.attributes = AnyOwnedObject::create<StatementAttributes>();
        }
        auto traits = kp.attributes->cast<StatementAttributes>();
        traits->visibilityTokenIdx = kp.kwToken.tokenIdx;
        traits->isPublic = (kp.kwToken.label == g_common->publicKey);
        return biscuit::KeywordResult::Attribute;
    }

    return biscuit::KeywordResult::Illegal;
}

void handlePlyfileFunction(ExtendedParser* ep, Owned<biscuit::Statement>&& stmt,
                           const biscuit::ExpandedToken& nameToken) {
    bool accept = true;
    auto fnDef = stmt->functionDefinition();
    Repository::ModuleOrFunction** prevMod = g_repository->globalScope.find(fnDef->name);
    if (prevMod) {
        StringView type = "function";
        if (auto prevCB = (*prevMod)->stmt->customBlock()) {
            type = g_labelStorage.view(prevCB->type);
        }
        errorAtToken(ep->parser, nameToken, biscuit::ErrorTokenAction::DoNothing,
                     String::format("'{}' was already defined as {}", nameToken.text, type));
        ep->parser->recovery.muteErrors = false;
        ep->parser->error(
            String::format("{}: ... see previous definition\n",
                           (*prevMod)->plyfile->tkr.fileLocationMap.formatFileLocation(
                               (*prevMod)->stmt->fileOffset)));
        accept = false;
    }

    if (!parseParameterList(ep->parser, fnDef.get()))
        return;

    // Parse function body.
    biscuit::Parser::Filter filter;
    filter.keywordHandler = {handleKeywordInsideModuleOrFunction, ep};
    filter.allowInstructions = true;
    PLY_SET_IN_SCOPE(ep->parser->filter, filter);
    fnDef->body = parseStatementBlock(ep->parser, {"function", "parameter list"});

    // Create module
    if (accept) {
        auto ownedMod = Owned<Repository::ModuleOrFunction>::create();
        ownedMod->plyfile = ep->currentPlyfile;
        ownedMod->stmt = std::move(stmt);

        *g_repository->globalScope.insert(fnDef->name) = ownedMod;
        g_repository->functions.append(std::move(ownedMod));
    }
}

bool parsePlyfile(StringView path) {
    String src = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile =
        g_repository->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->src = std::move(src);
    plyfile->tkr.setSourceInput(path, plyfile->src);
    biscuit::Parser parser;

    // Add parser keywords.
    *parser.keywords.insert(g_common->moduleKey) = true;
    *parser.keywords.insert(g_common->executableKey) = true;
    *parser.keywords.insert(g_common->sourceFilesKey) = true;
    *parser.keywords.insert(g_common->includeDirectoriesKey) = true;
    *parser.keywords.insert(g_common->preprocessorDefinitionsKey) = true;
    *parser.keywords.insert(g_common->dependenciesKey) = true;
    *parser.keywords.insert(g_common->linkLibrariesKey) = true;
    *parser.keywords.insert(g_common->configOptionsKey) = true;
    *parser.keywords.insert(g_common->configListKey) = true;
    *parser.keywords.insert(g_common->configKey) = true;
    *parser.keywords.insert(g_common->compileOptionsKey) = true;
    *parser.keywords.insert(g_common->publicKey) = true;
    *parser.keywords.insert(g_common->privateKey) = true;
    *parser.keywords.insert(g_common->generateKey) = true;

    // Extend the parser
    ExtendedParser ep;
    ep.parser = &parser;
    ep.currentPlyfile = plyfile;
    parser.tkr = &plyfile->tkr;
    parser.error = [](StringView message) { StdErr::text() << message; };
    biscuit::Parser::Filter filter;
    filter.keywordHandler = {handleKeywordAtFileScope, &ep};
    filter.allowFunctions = true;
    parser.filter = filter;
    parser.functionHandler = {handlePlyfileFunction, &ep};

    // Parse the script and check for errors.
    biscuit::StatementBlockProperties props{"file"};
    Owned<biscuit::StatementBlock> file = parseStatementBlockInner(&parser, props, true);
    if (parser.errorCount > 0) {
        return false;
    }

    // Success.
    plyfile->contents = std::move(file);
    return true;
}

} // namespace build2
} // namespace ply
