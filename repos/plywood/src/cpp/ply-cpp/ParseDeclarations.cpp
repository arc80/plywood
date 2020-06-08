/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/RestorePoint.h>

namespace ply {
namespace cpp {

PLY_NO_INLINE Token parseRequiredSemicolon(Parser* parser) {
    Token semiToken = readToken(parser);
    if (semiToken.type == Token::Semicolon) {
        parser->stopMutingErrors();
        return semiToken;
    } else {
        // expected ;
        parser->error(true, {ParseError::Expected, semiToken, ExpectedToken::Semicolon});
        pushBackToken(parser, semiToken);
        return {};
    }
}

PLY_NO_INLINE bool isTypeDeclaration(const grammar::Declaration::Simple& simple) {
    for (const grammar::DeclSpecifier* declSpec : simple.declSpecifierSeq) {
        switch (declSpec->id) {
            case grammar::DeclSpecifier::ID::Record:
            case grammar::DeclSpecifier::ID::Enum_:
                return true;
            default:
                break;
        }
    }
    return false;
}

PLY_NO_INLINE void parseEnumBody(Parser* parser, grammar::DeclSpecifier::Enum_* en) {
    parser->stopMutingErrors();
    SetAcceptFlagsInScope acceptScope{parser, Token::OpenCurly};

    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::CloseCurly) {
            // Done
            parser->stopMutingErrors();
            en->closeCurly = token;
            break;
        } else if (token.type == Token::Identifier) {
            parser->stopMutingErrors();

            // Create enumerator
            grammar::InitEnumeratorWithComma* initEnor =
                en->enumerators.append(new grammar::InitEnumeratorWithComma);
            initEnor->identifier = token;
            parseOptionalVariableInitializer(parser, initEnor->init, false);
            Token token2 = readToken(parser);
            bool done = false;
            if (token2.type == Token::Comma) {
                parser->stopMutingErrors();
                initEnor->comma = token2;
            } else if (token2.type == Token::CloseCurly) {
                // Done
                parser->stopMutingErrors();
                en->closeCurly = token;
                done = true;
            } else {
                // expected , or } after enum member
                if (token2.type == Token::Identifier) {
                    parser->error(true,
                                  {ParseError::MissingCommaAfterEnumerator, token2, {}, token});
                }
                // Other tokens will generate an error on next loop iteration
                pushBackToken(parser, token2);
            }
            parser->visor->onGotEnumerator(initEnor);
            if (done)
                break;
        } else {
            // expected enumerator or }
            parser->error(true,
                          {ParseError::Expected, token, ExpectedToken::EnumeratorOrCloseCurly});
            if (!handleUnexpectedToken(parser, nullptr, token))
                return;
        }
    }
}

PLY_NO_INLINE void parseSimpleDeclaration(Parser* parser, StringView enclosingClassName) {
    Token startLoc = readToken(parser);
    pushBackToken(parser, startLoc);
    ParseActivity pa{parser};
    grammar::Declaration::Simple simple;
    parseSpecifiersAndDeclarators(parser, simple,
                                  {SpecDcorMode::GlobalOrMember, enclosingClassName});
    if (!pa.errorOccurred()) {
        parser->stopMutingErrors();
        if (simple.initDeclarators.isEmpty() && !isTypeDeclaration(simple)) {
            // FIXME: It feels like this should be a higher-level error (or just a warning
            // actually!), since it's more of a semantic than a syntactic error. Similarly, it
            // should not be possible to define a class inside a function parameter!
            parser->error(false, {ParseError::MissingDeclaration, startLoc});
        }
    }

    bool requiresTrailingSemicolon = false;
    if (simple.initDeclarators.numItems() > 0) {
        const grammar::InitDeclaratorWithComma& initDcor = simple.initDeclarators.back();
        requiresTrailingSemicolon = !initDcor.init.functionBody();
    }

    if (requiresTrailingSemicolon) {
        simple.semicolon = parseRequiredSemicolon(parser);
    }

    parser->visor->gotDeclaration(std::move(simple));
}

// Returns false if no input was read.
PLY_NO_INLINE bool parseDeclaration(Parser* parser, StringView enclosingClassName) {
    Token token = readToken(parser);

    PLY_SET_IN_SCOPE(parser->atDeclarationScope, false);
    if (token.type == Token::Identifier) {
        if (token.identifier == "extern") {
            // Possible linkage specification
            parser->stopMutingErrors();
            pushBackToken(parser, token);
            RestorePoint rp{parser};
            token = readToken(parser);
            grammar::Declaration::Linkage linkage;
            linkage.extern_ = token;

            token = readToken(parser);
            if (token.type != Token::StringLiteral) {
                rp.backtrack();
                parseSimpleDeclaration(parser, enclosingClassName);
            } else {
                linkage.literal = token;

                token = readToken(parser);
                if (token.type == Token::OpenCurly) {
                    // It's a linkage specification block, such as
                    //      extern "C" {
                    //          ...
                    //      }
                    rp.cancel();
                    linkage.openCurly = token;
                    parser->visor->doEnter(TypedPtr::bind(&linkage));
                    parseDeclarationList(parser, &linkage.closeCurly, {});
                    parser->visor->doExit(TypedPtr::bind(&linkage));
                    parser->visor->gotDeclaration(std::move(linkage));
                } else {
                    // It's a linkage specifier for the current declaration, such as
                    //      extern "C" void foo();
                    //      ^^^^^^^^^^
                    // FIXME: Make Declaration type for this
                    rp.backtrack();
                    parseSimpleDeclaration(parser, enclosingClassName);
                }
            }
        } else if (token.identifier == "public" || token.identifier == "private" ||
                   token.identifier == "protected") {
            // Access specifier
            parser->stopMutingErrors();
            Token puncToken = readToken(parser);
            if (puncToken.type == Token::SingleColon) {
                parser->visor->gotDeclaration(
                    grammar::Declaration::AccessSpecifier{token, puncToken});
            } else {
                // expected :
                parser->error(true, {ParseError::Expected, puncToken, ExpectedToken::Colon, token});
                pushBackToken(parser, puncToken);
            }
        } else if (token.identifier == "static_assert") {
            // static_assert
            parser->stopMutingErrors();
            Token puncToken = readToken(parser);
            if (puncToken.type != Token::OpenParen) {
                // expected (
                parser->error(true,
                              {ParseError::Expected, puncToken, ExpectedToken::OpenParen, token});
                pushBackToken(parser, puncToken);
            } else {
                Token closeToken;
                bool continueNormally = skipAnyScope(parser, &closeToken, puncToken);
                if (continueNormally) {
                    grammar::Declaration::StaticAssert sa;
                    sa.keyword = token;
                    sa.argList.openParen = puncToken;
                    sa.argList.closeParen = closeToken;
                    sa.semicolon = parseRequiredSemicolon(parser);
                    parser->visor->gotDeclaration(grammar::Declaration{std::move(sa)});
                }
            }
        } else if (token.identifier == "namespace") {
            // namespace
            parser->stopMutingErrors();
            grammar::Declaration::Namespace_ ns;
            ns.keyword = token;

            Token token = readToken(parser);
            if (token.type == Token::Identifier) {
                // FIXME: Ensure it's not a reserved word
                pushBackToken(parser, token);
                ns.qid = parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
                token = readToken(parser);
            }

            if (token.type == Token::OpenCurly) {
                ns.openCurly = token;
                parser->visor->doEnter(TypedPtr::bind(&ns));
                parseDeclarationList(parser, &ns.closeCurly, {});
                parser->visor->doExit(TypedPtr::bind(&ns));
            } else {
                // expected {
                parser->error(true, {ParseError::Expected, token, ExpectedToken::OpenCurly});
                pushBackToken(parser, token);
            }
            parser->visor->gotDeclaration(std::move(ns));
        } else if (token.identifier == "template") {
            // template
            parser->stopMutingErrors();
            grammar::Declaration::Template_ tmpl;
            tmpl.keyword = token;
            Token token2 = readToken(parser);
            if (token2.type == Token::OpenAngle) {
                pushBackToken(parser, token2);
                {
                    PLY_SET_IN_SCOPE(parser->pp->tokenizeCloseAnglesOnly, true);
                    parseParameterDeclarationList(parser, tmpl.params, true);
                }
            } else {
                pushBackToken(parser, token2);
            }
            parser->visor->doEnter(TypedPtr::bind(&tmpl));
            parseDeclaration(parser, enclosingClassName);
            parser->visor->doExit(TypedPtr::bind(&tmpl));
            parser->visor->gotDeclaration(grammar::Declaration{std::move(tmpl)});
        } else if (token.identifier == "using") {
            // using directive or type alias
            parser->stopMutingErrors();
            Token token2 = readToken(parser);
            if (token2.type == Token::Identifier && token2.identifier == "namespace") {
                grammar::Declaration::UsingDirective usingDir;
                usingDir.using_ = token;
                usingDir.namespace_ = token2;

                usingDir.qid = parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
                usingDir.semicolon = parseRequiredSemicolon(parser);

                parser->visor->gotDeclaration(grammar::Declaration{std::move(usingDir)});
            } else {
                grammar::Declaration::Alias alias;
                alias.using_ = token;
                alias.name = token2;

                Token equalToken = readToken(parser);
                if (equalToken.type != Token::SingleEqual) {
                    // expected =
                    parser->error(true,
                                  {ParseError::Expected, equalToken, ExpectedToken::Equal, token2});
                    pushBackToken(parser, equalToken);
                } else {
                    alias.equals = equalToken;

                    grammar::Declaration::Simple simple;
                    parseSpecifiersAndDeclarators(parser, simple, SpecDcorMode::TypeID);
                    alias.declSpecifierSeq = std::move(simple.declSpecifierSeq);
                    PLY_ASSERT(simple.initDeclarators.numItems() ==
                               1); // because SpecDcorMode::TypeID
                    alias.dcor = std::move(simple.initDeclarators[0].dcor);
                    alias.semicolon = parseRequiredSemicolon(parser);

                    parser->visor->gotDeclaration(grammar::Declaration{std::move(alias)});
                }
            }
        } else {
            pushBackToken(parser, token);
            parseSimpleDeclaration(parser, enclosingClassName);
        }
    } else if (token.type == Token::Semicolon) {
        grammar::Declaration::Empty empty;
        empty.semicolon = token;
        parser->visor->gotDeclaration(grammar::Declaration{std::move(empty)});
    } else if (token.type == Token::Tilde) {
        pushBackToken(parser, token);
        parseSimpleDeclaration(parser, enclosingClassName);
    } else {
        // Can't handle this token.
        pushBackToken(parser, token);
        return false;
    }
    return true;
}

void parseDeclarationList(Parser* parser, Token* outCloseCurly, StringView enclosingClassName) {
    // Always handle close curly at this scope, even if it's file scope:
    SetAcceptFlagsInScope acceptScope{parser, Token::OpenCurly};
    PLY_SET_IN_SCOPE(parser->atDeclarationScope, true);

    for (;;) {
        Token token = readToken(parser);
        if (token.type == (outCloseCurly ? Token::CloseCurly : Token::EndOfFile)) {
            if (outCloseCurly) {
                *outCloseCurly = token;
            }
            break;
        }
        pushBackToken(parser, token);
        if (!parseDeclaration(parser, enclosingClassName)) {
            token = readToken(parser);
            parser->error(true, {ParseError::Expected, token, ExpectedToken::Declaration});
            if (token.type != Token::CloseCurly) { // consume close curlies after logging the error
                if (!handleUnexpectedToken(parser, nullptr, token)) {
                    PLY_ASSERT(token.type == Token::EndOfFile); // can only be EOF here
                    break;
                }
            }
        }
    }
}

grammar::TranslationUnit parseTranslationUnit(Parser* parser) {
    grammar::TranslationUnit tu;
    parser->visor->doEnter(TypedPtr::bind(&tu));
    parseDeclarationList(parser, nullptr, {});
    Token eofTok = readToken(parser);
    PLY_ASSERT(eofTok.type == Token::EndOfFile); // EOF is the only possible token here
    PLY_UNUSED(eofTok);
    parser->visor->doExit(TypedPtr::bind(&tu));
    return tu;
}

} // namespace cpp
} // namespace ply
