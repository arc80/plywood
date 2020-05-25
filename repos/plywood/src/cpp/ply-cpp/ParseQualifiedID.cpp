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

// Consumes as much as it can; unrecognized tokens are returned to caller without logging an error
Array<grammar::NestedNameComponent> parseNestedNameSpecifier(Parser* parser) {
    // FIXME: Support leading ::
    Array<grammar::NestedNameComponent> nestedName;
    for (;;) {
        grammar::NestedNameComponent* comp = nullptr;

        Token token = readToken(parser);
        if (token.type != Token::Identifier) {
            pushBackToken(parser, token);
            break;
        }
        if (token.identifier == "operator" || token.identifier == "const" ||
            token.identifier == "volatile" || token.identifier == "inline" ||
            token.identifier == "static" || token.identifier == "friend") {
            pushBackToken(parser, token);
            break;
        }

        if (token.identifier == "decltype") {
            comp = &nestedName.append();
            auto declType = comp->type.declType().switchTo();
            declType->keyword = token;
            Token puncToken = readToken(parser);
            if (puncToken.type == Token::OpenParen) {
                declType->openParen = puncToken;
                skipAnyScope(parser, &declType->closeParen, puncToken);
            } else {
                // expected (
                parser->error(true,
                              {ParseError::Expected, puncToken, ExpectedToken::OpenParen, token});
                pushBackToken(parser, puncToken);
            }
        } else {
            comp = &nestedName.append();
            auto ident = comp->type.identifierOrTemplated().switchTo();
            ident->name = token;
            Token puncToken = readToken(parser);
            if (puncToken.type == Token::OpenAngle) {
                // FIXME: We should only parse < as the start of a template-argument list if we know
                // that the preceding name refers to a template function or type. For now, we assume
                // it always does. If we ever start parsing function bodies, we won't be able to
                // assume this.
                if (parser->passNumber <= 1) {
                    ident->openAngled = puncToken;

                    // Parse template-argument-list
                    SetAcceptFlagsInScope acceptScope{parser, Token::OpenAngle};
                    PLY_SET_IN_SCOPE(parser->pp->tokenizeCloseAnglesOnly, true);

                    for (;;) {
                        // FIXME: Parse constant expressions here instead of only allowing type IDs

                        // Try to parse a type ID
                        grammar::TemplateArgumentWithComma& templateArg = ident->args.append();
                        Token startToken = readToken(parser);
                        templateArg.type.unknown()->startToken = startToken;
                        pushBackToken(parser, startToken);
                        RestorePoint rp{parser};
                        grammar::Declaration::Simple simple;
                        parseSpecifiersAndDeclarators(parser, simple, SpecDcorMode::TypeID);
                        if (!rp.errorOccurred()) {
                            // Successfully parsed a type ID
                            auto typeID = templateArg.type.typeID().switchTo();
                            typeID->declSpecifierSeq = std::move(simple.declSpecifierSeq);
                            PLY_ASSERT(simple.initDeclarators.numItems() == 1);
                            PLY_ASSERT(simple.initDeclarators[0].dcor.qid.isEmpty());
                            typeID->abstractDcor = std::move(simple.initDeclarators[0].dcor.prod);
                        } else {
                            rp.backtrack();
                            rp.cancel();
                        }

                        auto unknown = templateArg.type.unknown();
                        for (;;) {
                            Token sepToken = readToken(parser);
                            if (sepToken.type == Token::CloseAngle) {
                                // End of template-argument-list
                                ident->closeAngled = sepToken;
                                parser->stopMutingErrors();
                                goto breakArgs;
                            } else if (sepToken.type == Token::Comma) {
                                // Comma
                                templateArg.comma = sepToken;
                                parser->stopMutingErrors();
                                break;
                            } else {
                                // Unexpected token
                                if (!unknown) {
                                    unknown.switchTo();
                                }
                                unknown->endToken = sepToken;
                                if (!handleUnexpectedToken(parser, &unknown->endToken, sepToken))
                                    goto breakOuter;
                            }
                        }
                    }
                breakArgs:;
                } else {
                    PLY_FORCE_CRASH(); // FIXME: implement this
                }
            } else {
                pushBackToken(parser, puncToken);
            }
        }

        PLY_ASSERT(comp);
        Token sepToken = readToken(parser);
        if (sepToken.type == Token::DoubleColon) {
            comp->sep = sepToken;
        } else {
            pushBackToken(parser, sepToken);
            break;
        }
    }

breakOuter:
    return nestedName;
}

// Consumes as much as it can; unrecognized tokens are returned to caller without logging an error
grammar::QualifiedID parseQualifiedID(Parser* parser, ParseQualifiedMode mode) {
    grammar::QualifiedID qid;
    qid.nestedName = parseNestedNameSpecifier(parser);
    if (qid.nestedName.numItems() > 0) {
        grammar::NestedNameComponent& tail = qid.nestedName.back();
        if (!tail.sep.isValid()) {
            if (auto ident = tail.type.identifierOrTemplated()) {
                if (ident->openAngled.isValid()) {
                    auto tmpl = qid.unqual.templateID().switchTo();
                    tmpl->name = ident->name;
                    tmpl->openAngled = ident->openAngled;
                    tmpl->closeAngled = ident->closeAngled;
                    tmpl->args = std::move(ident->args);
                } else {
                    auto id = qid.unqual.identifier().switchTo();
                    id->name = ident->name;
                }
            } else if (auto declType = tail.type.declType()) {
                auto dst = qid.unqual.declType().switchTo();
                dst->keyword = declType->keyword;
                dst->openParen = declType->openParen;
                dst->closeParen = declType->closeParen;
            }
            qid.nestedName.pop();
        }
    }
    if (qid.unqual.empty()) {
        Token token = readToken(parser);
        if (token.type == Token::Tilde) {
            Token token2 = readToken(parser);
            if (token2.type != Token::Identifier) {
                // Expected class name after ~
                parser->error(true, {ParseError::Expected, token2,
                                     ExpectedToken::DestructorClassName, token});
                pushBackToken(parser, token2);
            } else {
                auto dtor = qid.unqual.destructor().switchTo();
                PLY_ASSERT(token2.identifier != "decltype"); // FIXME: Support this
                dtor->tilde = token;
                dtor->name = token2;
            }
        } else if (token.type == Token::Identifier) {
            if (token.identifier == "operator") {
                auto opFunc = qid.unqual.operatorFunc().switchTo();
                opFunc->keyword = token;
                Token opToken = readToken(parser);
                switch (opToken.type) {
                    case Token::LeftShift:
                    case Token::RightShift:
                    case Token::SinglePlus:
                    case Token::DoublePlus:
                    case Token::SingleMinus:
                    case Token::DoubleMinus:
                    case Token::Star:
                    case Token::Arrow:
                    case Token::ForwardSlash:
                    case Token::SingleEqual:
                    case Token::DoubleEqual:
                    case Token::NotEqual:
                    case Token::PlusEqual:
                    case Token::MinusEqual:
                    case Token::StarEqual:
                    case Token::SlashEqual:
                    case Token::OpenAngle:
                    case Token::CloseAngle:
                    case Token::LessThanOrEqual:
                    case Token::GreaterThanOrEqual:
                    case Token::OpenParen:
                    case Token::OpenSquare: {
                        opFunc->punc = opToken;
                        if (opToken.type == Token::OpenParen) {
                            Token opToken2 = readToken(parser);
                            if (opToken2.type == Token::CloseParen) {
                                opFunc->punc2 = opToken2;
                            } else {
                                // Expected ) after (
                                parser->error(true, {ParseError::Expected, opToken2,
                                                     ExpectedToken::CloseParen, opToken});
                                pushBackToken(parser, opToken2);
                            }
                        } else if (opToken.type == Token::OpenSquare) {
                            Token opToken2 = readToken(parser);
                            if (opToken2.type == Token::CloseSquare) {
                                opFunc->punc2 = opToken2;
                            } else {
                                parser->error(true, {ParseError::Expected, opToken2,
                                                     ExpectedToken::CloseSquare, opToken});
                                pushBackToken(parser, opToken2);
                            }
                        }
                        break;
                    }

                    default: {
                        // Expected operator token
                        parser->error(true, {ParseError::Expected, opToken,
                                             ExpectedToken::OperatorToken, token});
                        parser->muteErrors = true;
                        pushBackToken(parser, opToken);
                        break;
                    };
                }
            } else {
                pushBackToken(parser, token);
            }
        } else {
            pushBackToken(parser, token);
        }
    }
    if (((mode == ParseQualifiedMode::RequireComplete) && !qid.isComplete()) ||
        ((mode == ParseQualifiedMode::RequireCompleteOrEmpty) && qid.isNestedNameOnly())) {
        // FIXME: Improve these error messages
        Token token = readToken(parser);
        parser->error(true, {ParseError::Expected, token, ExpectedToken::QualifiedID});
        pushBackToken(parser, token);
    }
    return qid;
}

} // namespace cpp
} // namespace ply
