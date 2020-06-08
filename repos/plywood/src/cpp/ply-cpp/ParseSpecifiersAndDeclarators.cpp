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

struct DeclaratorFlags {
    static const u32 AllowNamed = 1;
    static const u32 AllowAbstract = 2;
};

// Consumes as much as it can; unrecognized tokens are returned to caller without logging an error:
PLY_NO_INLINE void parseConversionTypeID2(Parser* parser, grammar::Declarator& dcor,
                                          grammar::DeclaratorProduction* nested) {
    dcor.prod = nested;
    bool allowQualifier = false;

    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::Star || token.type == Token::SingleAmpersand ||
            token.type == Token::DoubleAmpersand) {
            auto* prod = new grammar::DeclaratorProduction;
            auto ptrTo = prod->type.pointerTo().switchTo();
            ptrTo->punc = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
            allowQualifier = (token.type == Token::Star);
        } else if (token.type == Token::Ellipsis) {
            // FIXME: Make a Production rule for this
        } else if (token.type == Token::Identifier) {
            if (token.identifier == "const" || token.identifier == "volatile") {
                if (!allowQualifier) {
                    // Qualifier not allowed here (eg. immediately after comma in declarator
                    // list). This is not a breaking error; just ignore it and continue from here.
                    parser->error(false, {ParseError::QualifierNotAllowedHere, token});
                }

                auto* prod = new grammar::DeclaratorProduction;
                auto qualifier = prod->type.qualifier().switchTo();
                qualifier->keyword = token;
                prod->target = std::move(dcor.prod);
                dcor.prod = prod;
            } else {
                pushBackToken(parser, token);
                break;
            }
        } else {
            // Unrecognized tokens are returned to caller without raising an error here.
            pushBackToken(parser, token);
            break;
        }
    }
}

// Consumes as much as it can; unrecognized tokens are returned to caller without logging an error:
PLY_NO_INLINE void parseConversionTypeID(Parser* parser,
                                         grammar::UnqualifiedID::ConversionFunc* conv) {
    // Note: This has some similarities to parseSpecifiersAndDeclarators (with an optional
    // abstract declarator using & * &&), but for now, merging them appears too complex,
    // especially since this function does not accept () or []. (OR...? We could simply set a
    // flag to prevent those functions from consuming '(' or ']'.)
    bool gotTypeSpecifier = false;
    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "const" || token.identifier == "volatile") {
                conv->declSpecifierSeq.append(
                    new grammar::DeclSpecifier{grammar::DeclSpecifier::Keyword{token}});
            } else {
                pushBackToken(parser, token);
                grammar::QualifiedID qid =
                    parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
                if (gotTypeSpecifier) {
                    // We already got a type specifier.
                    // This is not a breaking error; just ignore it and continue from here.
                    parser->error(false, {ParseError::TooManyTypeSpecifiers, qid.getFirstToken()});
                } else {
                    gotTypeSpecifier = true;
                    PLY_ASSERT(!qid.isEmpty()); // Shouldn't happen because token was an identifier
                    conv->declSpecifierSeq.append(new grammar::DeclSpecifier{
                        grammar::DeclSpecifier::TypeID{{}, std::move(qid)}});
                }
            }
        } else {
            // This must be the optional (limited) abstract declarator part:
            pushBackToken(parser, token);
            grammar::Declarator dcor;
            parseConversionTypeID2(parser, dcor, nullptr);
            PLY_ASSERT(dcor.qid.isEmpty());
            conv->abstractDcor = std::move(dcor.prod);
            break;
        }
    }
}

PLY_NO_INLINE bool closeScope(Parser* parser, Token* outCloseToken, const Token& openToken) {
    Token closeToken = readToken(parser);
    if (closeToken.type == openToken.type + 1) {
        *outCloseToken = closeToken;
    } else {
        ExpectedToken exp = ExpectedToken::CloseParen;
        if (openToken.type == Token::OpenSquare) {
            exp = ExpectedToken::CloseSquare;
        } else {
            PLY_ASSERT(openToken.type == Token::OpenParen);
        }
        parser->error(true, {ParseError::Expected, closeToken, exp});
        pushBackToken(parser, closeToken);
        // Consume tokens up to the closing )
        if (!skipAnyScope(parser, nullptr, openToken)) {
            // We didn't get a closing ), but an outer scope will handle it
            PLY_ASSERT(parser->muteErrors);
            return false;
        }
        // Got closing )
        parser->stopMutingErrors();
    }
    return true;
}

void parseOptionalTrailingReturnType(Parser* parser, grammar::DeclaratorProduction* fnProd) {
    PLY_ASSERT(fnProd);
    auto function = fnProd->type.function();
    PLY_ASSERT(function);

    Token arrowToken = readToken(parser);
    if (arrowToken.type == Token::Arrow) {
        function->arrow = arrowToken;
        Token qidStartToken = readToken(parser);
        pushBackToken(parser, qidStartToken);
        // FIXME: Should parse a TypeID here, not just a qualified ID:
        function->trailingRetType = parseQualifiedID(parser, ParseQualifiedMode::AllowIncomplete);
        if (function->trailingRetType.isEmpty()) {
            parser->error(true, {ParseError::Expected, qidStartToken,
                                 ExpectedToken::TrailingReturnType, function->arrow});
        }
    } else {
        pushBackToken(parser, arrowToken);
    }
}

//-------------------------------------------------------------------------------------
// parseDeclarator
//
// When bad tokens are encountered, it consumes them until it encounters a token that an outer scope
// is expected to handle, as determined by parser->outerAcceptFlags. In that case, it returns early.
// If the bad token is one of { ( or [, it calls skipAnyScope().
//
// The first bad token sets parser->muteErrors to true. muteErrors remains true until it reaches the
// next good token. muteErrors may remain true when we return; this can happen, for example, when }
// is encountered, causing us to return early.
//-------------------------------------------------------------------------------------
void parseDeclarator(Parser* parser, grammar::Declarator& dcor,
                     grammar::DeclaratorProduction* nested, u32 dcorFlags) {
    dcor.prod = nested;
    bool allowQualifier = false;
    Owned<grammar::DeclaratorProduction>* prodToModify = nullptr; // Used in phase two
    bool expectingQualifiedID = false;

    // This is the first phase of parsing a declarator. It handles everything up to trailing
    // function parameter lists and array subscripts.
    //
    // As it reads pointer, reference symbols and cv-qualifiers, it inserts new
    // DeclaratorProductions at the *head* of the current DeclarationProduction chain
    // (dcor.prod) so that they are effectively read right-to-left. For example,
    //      * const &
    // becomes "reference to const pointer" in the DeclarationProduction chain.
    //
    // Pointers can also have nested name specifiers, making them pointer-to-members:
    //      Foo::*
    //
    // If an open parenthesis is encountered during this phase, and the AllowAbstract flags is
    // set, it first tries to parse a function parameter list; otherwise, or if that fails, it
    // tries to parse a nested declarator. If it's a nested declarator, nested
    // DeclarationProductions are inserted at the head of the current DeclarationProduction
    // chain. In either case, no further pointer/reference/cv-qualifiers are expected after the
    // closing parenthesis, so we break out of the loop and proceed to the second phase.
    //
    // When a qualified ID is encountered, it's considered the name of the declarator (in other
    // words, the declarator is not abstract), and we break out of the loop and proceed to the
    // second phase.

    for (;;) {
        // Try to tokenize a qualified ID.
        grammar::QualifiedID qid = parseQualifiedID(parser, ParseQualifiedMode::AllowIncomplete);
        if (!qid.unqual.empty()) {
            dcor.qid = std::move(qid);
            if ((dcorFlags & DeclaratorFlags::AllowNamed) == 0) {
                // Qualified ID is not allowed here
                // FIXME: Should rewind instead of consuming the qualified-id????
                // The caller may log a more informative error at this token! (check test suite)
                parser->error(false, {ParseError::TypeIDCannotHaveName, qid.getFirstToken()});
                // Don't mute errors
            }
            break; // Got qualified-id
        }
        // qid.unqual is empty, but qid.nestedName might be a pointer prefix (as in a
        // pointer-to-member).

        Token token = readToken(parser);
        if (token.type == Token::OpenParen) {
            if (!qid.nestedName.isEmpty()) {
                // Should not be preceded by nested name specifier
                parser->error(
                    false, {ParseError::NestedNameNotAllowedHere, token, {}, qid.getFirstToken()});
                // Don't mute errors
            }

            parser->stopMutingErrors();

            if ((dcorFlags & DeclaratorFlags::AllowAbstract) != 0) {
                // If abstract declarators are allowed, try to parse a function parameter list
                // first.
                pushBackToken(parser, token);
                RestorePoint rp{parser};
                // FIXME: When a restore point is active, handleUnexpectedToken() should always
                // return false. Otherwise, parseParameterList could end up consuming way too many
                // tokens, and it might even incorrectly "pre-tokenize" '>>' as a right-shift
                // operator instead of as two CloseAngles...
                grammar::DeclaratorProduction* savedProd = dcor.prod;
                prodToModify = &dcor.prod;
                grammar::DeclaratorProduction* fnProd = parseParameterList(parser, &prodToModify);
                if (!rp.errorOccurred()) {
                    // Success. Parse optional trailing return type. If any parse errors occur while
                    // doing so, we won't backtrack.
                    PLY_ASSERT(fnProd);
                    rp.cancel();
                    parseOptionalTrailingReturnType(parser, fnProd);
                    // Break out of the loop and continue with the second phase.
                    break;
                }

                // It didn't parse as a function parameter list.
                // Roll back any productions that were created:
                while (dcor.prod != savedProd) {
                    PLY_ASSERT(dcor.prod);
                    grammar::DeclaratorProduction* child = dcor.prod->target.release();
                    dcor.prod = child;
                }
                rp.backtrack();
                rp.cancel();
                token = readToken(parser);
                prodToModify = nullptr;
            }

            // Parse it as a nested declarator.
            grammar::Declarator target;
            parseDeclarator(parser, target, dcor.prod.release(), dcorFlags);
            dcor.prod = new grammar::DeclaratorProduction;
            auto parenthesized = dcor.prod->type.parenthesized().switchTo();
            parenthesized->openParen = token;
            dcor.prod->target = std::move(target.prod);
            PLY_ASSERT(dcor.qid.isEmpty());
            dcor.qid = std::move(target.qid);

            if (!closeScope(parser, &parenthesized->closeParen, token))
                return;
            break;
        }

        if (!qid.nestedName.isEmpty()) {
            if (token.type != Token::Star) {
                // Should not be preceded by nested name specifier
                parser->error(
                    false, {ParseError::NestedNameNotAllowedHere, token, {}, qid.getFirstToken()});
            }
        }

        if (token.type == Token::Star || token.type == Token::SingleAmpersand ||
            token.type == Token::DoubleAmpersand) {
            parser->stopMutingErrors();

            auto* prod = new grammar::DeclaratorProduction;
            auto ptrTo = prod->type.pointerTo().switchTo();
            ptrTo->nestedName = std::move(qid.nestedName);
            ptrTo->punc = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
            allowQualifier = (token.type == Token::Star);
        } else if (token.type == Token::Ellipsis) {
            // FIXME: Make a Production rule for this

            parser->stopMutingErrors();
        } else if (token.type == Token::Identifier) {
            PLY_ASSERT(qid.nestedName.isEmpty());
            PLY_ASSERT(token.identifier == "const" || token.identifier == "volatile" ||
                       token.identifier == "inline" || token.identifier == "static" ||
                       token.identifier == "friend");
            if (!allowQualifier) {
                // Qualifier not allowed here
                parser->error(false, {ParseError::QualifierNotAllowedHere, token});
                // Handle it anyway...
            }

            parser->stopMutingErrors();

            auto* prod = new grammar::DeclaratorProduction;
            auto qualifier = prod->type.qualifier().switchTo();
            qualifier->keyword = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
        } else {
            // End of first phase of parsing a declarator.
            PLY_ASSERT(qid.nestedName.isEmpty());
            if ((dcorFlags & DeclaratorFlags::AllowAbstract) == 0) {
                // Note that we still allow "empty" declarators (in other words, abstract
                // declarators with no DeclaratorProductions) even when AllowAbstract is not
                // specified, so that class definitions like:
                //      struct Foo {};
                // do not log an error.
                //
                // With this in mind, if a declarator name was required but
                // none was given, log an error *only if* some DeclaratorProductions have been
                // created.
                //
                // FIXME: Log an error (or warning?) if it's an empty declarators that *doesn't*
                // define a new class/struct/union, such as:
                //      int;
                if (dcor.prod) {
                    parser->error(true, {ParseError::Expected, token, ExpectedToken::QualifiedID});
                } else {
                    // No DeclaratorProductions have been created yet. We'll log an error if any are
                    // created in the second phase.
                    expectingQualifiedID = true;
                }
            }
            pushBackToken(parser, token);
            break;
        }
    }

    // This is the second phase of parsing a declarator. It parses only trailing function
    // parameter lists and array subscripts. A subchain of DeclaratorProductions is built in the
    // same order that these are encountered, so that they're effectively read left-to-right.
    // For example,
    //      []()
    // becomes "array of functions" in the subchain. This subchain is inserted at the head of
    // dcor.prod, the current DeclaratorProduction chain being built.
    //
    // Note that this phase can take place inside a nested declarator, which means that the
    // caller may continue inserting DeclaratorProductions at the head of the chain after we
    // return.
    //
    // FIXME: make sure this approach works correctly for things like (*x())()

    if (!prodToModify) {
        prodToModify = &dcor.prod;
    }
    for (;;) {
        Token token = readToken(parser);
        auto checkExpectingQualifiedID = [&]() {
            parser->stopMutingErrors();
            if (expectingQualifiedID) {
                parser->error(true, {ParseError::Expected, token, ExpectedToken::QualifiedID});
                expectingQualifiedID = false;
            }
        };

        if (token.type == Token::OpenSquare) {
            checkExpectingQualifiedID();

            auto* prod = new grammar::DeclaratorProduction;
            auto arrayOf = prod->type.arrayOf().switchTo();
            arrayOf->openSquare = token;
            prod->target = std::move(*prodToModify);
            *prodToModify = prod;
            prodToModify = &prod->target;

            parseExpression(parser, true);

            if (!closeScope(parser, &arrayOf->closeSquare, token))
                return;
        } else if (token.type == Token::OpenParen) {
            checkExpectingQualifiedID();

            pushBackToken(parser, token);
            grammar::DeclaratorProduction* fnProd = parseParameterList(parser, &prodToModify);
            if (fnProd) {
                parseOptionalTrailingReturnType(parser, fnProd);
            }
        } else {
            pushBackToken(parser, token);
            break;
        }
    }
}

void skipMemberInitializerList(Parser* parser) {
    // Make sure that if { is encountered (even with unexpected placement), we return to caller.
    PLY_SET_IN_SCOPE(parser->outerAcceptFlags, parser->outerAcceptFlags | Parser::AcceptOpenCurly);
    // FIXME: Add a scope to declare that we are parsing a member initializer list, and report this
    // scope in any logged errors (?)

    for (;;) {
        grammar::QualifiedID qid = parseQualifiedID(parser, ParseQualifiedMode::AllowIncomplete);
        if (qid.isComplete()) {
            Token openBraceToken = readToken(parser);
            if (openBraceToken.type == Token::OpenParen) {
                skipAnyScope(parser, nullptr, openBraceToken);
            } else if (openBraceToken.type == Token::OpenCurly) {
                skipAnyScope(parser, nullptr, openBraceToken);
            } else {
                // expected ( or {
                // FIXME: should report that it was expected after qualified id
                parser->error(
                    true, {ParseError::Expected, openBraceToken, ExpectedToken::OpenCurlyOrParen});
                pushBackToken(parser, openBraceToken);
                continue;
            }

            Token nextToken = readToken(parser);
            if (nextToken.type == Token::OpenCurly) {
                // End of member initializer list.
                parser->stopMutingErrors();
                pushBackToken(parser, nextToken);
                break;
            } else if (nextToken.type == Token::Comma) {
                parser->stopMutingErrors();
            } else {
                parser->error(true,
                              {ParseError::ExpectedFunctionBodyAfterMemberInitList, nextToken});
                pushBackToken(parser, nextToken);
                break;
            }
        } else {
            Token token = readToken(parser);
            parser->error(true, {ParseError::Expected, token, ExpectedToken::BaseOrMember});
            if (qid.isEmpty()) {
                if (!handleUnexpectedToken(parser, nullptr, token))
                    break;
            } else {
                pushBackToken(parser, token);
            }
        }
    }
}

PLY_NO_INLINE void parseOptionalFunctionBody(Parser* parser, grammar::Initializer& result,
                                             const grammar::Declaration::Simple& simple) {
    result.none().switchTo();
    Token token = readToken(parser);
    if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switchTo();
        assign->equalSign = token;
        parseExpression(parser); // FIXME: Fill in varInit
        return;
    }
    if (token.type == Token::SingleColon) {
        auto fnBody = result.functionBody().switchTo();
        fnBody->colon = token;
        // FIXME: populate MemberInitializerWithComma
        skipMemberInitializerList(parser);
        token = readToken(parser);
    }
    if (token.type == Token::OpenCurly) {
        parser->visor->doEnter(TypedPtr::bind(&simple));
        auto fnBody = result.functionBody();
        if (!fnBody) {
            fnBody.switchTo();
        }
        fnBody->openCurly = token;
        skipAnyScope(parser, &fnBody->closeCurly, token);
        parser->visor->doExit(TypedPtr::bind(&simple));
    } else {
        pushBackToken(parser, token);
    }
}

void parseOptionalTypeIDInitializer(Parser* parser, grammar::Initializer& result) {
    result.none().switchTo();
    Token token = readToken(parser);
    if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switchTo();
        assign->equalSign = token;
        token = readToken(parser);
        if (token.identifier == "0") {
            // FIXME: Support <typename A::B = 0> correctly!
            // It will require changes to parseSpecifiersAndDeclarators
        } else {
            pushBackToken(parser, token);
            ParseActivity pa{parser};
            grammar::Declaration::Simple simple;
            parseSpecifiersAndDeclarators(parser, simple, SpecDcorMode::TypeID);
            if (!pa.errorOccurred()) {
                auto typeID = assign->type.typeID().switchTo();
                typeID->declSpecifierSeq = std::move(simple.declSpecifierSeq);
                PLY_ASSERT(simple.initDeclarators.numItems() == 1);
                PLY_ASSERT(simple.initDeclarators[0].dcor.qid.isEmpty());
                typeID->abstractDcor = std::move(simple.initDeclarators[0].dcor.prod);
            }
        }
    } else {
        pushBackToken(parser, token);
    }
}

PLY_NO_INLINE void parseOptionalVariableInitializer(Parser* parser, grammar::Initializer& result,
                                                    bool allowBracedInit) {
    result.none().switchTo();
    Token token = readToken(parser);
    if (token.type == Token::OpenCurly) {
        // It's a variable initializer
        result.assignment().switchTo();
        pushBackToken(parser, token);
        parseExpression(parser); // FIXME: Fill in varInit
    } else if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switchTo();
        assign->equalSign = token;
        Tuple<Token, Token> expPair = parseExpression(parser);
        assign->type.expression()->start = expPair.first;
        assign->type.expression()->end = expPair.second;
    } else if (token.type == Token::SingleColon) {
        auto bitField = result.bitField().switchTo();
        bitField->colon = token;
        Tuple<Token, Token> expPair = parseExpression(parser);
        bitField->expressionStart = expPair.first;
        bitField->expressionEnd = expPair.second;
    } else {
        pushBackToken(parser, token);
    }
}

// Only called from parseSpecifiersAndDeclarators:
void parseInitDeclarators(Parser* parser, grammar::Declaration::Simple& simple,
                          const SpecDcorMode& mode) {
    if (mode.mode == SpecDcorMode::GlobalOrMember) {
        // A list of zero or more named declarators is accepted here.
        for (;;) {
            grammar::Declarator dcor;
            parseDeclarator(parser, dcor, nullptr, DeclaratorFlags::AllowNamed);
            if (dcor.qid.isEmpty())
                break; // Error was already logged
            grammar::InitDeclaratorWithComma& initDcor = simple.initDeclarators.append();
            initDcor.dcor = std::move(dcor);
            if (initDcor.dcor.isFunction()) {
                parseOptionalFunctionBody(parser, initDcor.init, simple);
                if (initDcor.init.functionBody()) {
                    if (simple.initDeclarators.numItems() > 1) {
                        // Note: Mixing function definitions and declarations could be a
                        // higher-level error instead of a parse error.
                        // FIXME: A reference to both declarators should be part of the error
                        // message. For now, we'll just use the open parenthesis token.
                        parser->error(false,
                                      {ParseError::CantMixFunctionDefAndDecl,
                                       initDcor.dcor.prod->type.function()->params.openPunc});
                    }
                }
                // FIXME: Eventually it might be nice to *not* break early here, so that we can
                // look ahead to the next token and issue a special error message if it's a comma
                // (CantMixFunctionDefAndDecl, seen below). However, if we do that now, comments
                // at declaration scope will be skipped by readToken(), and the ParseSupervisor
                // won't get a complete list of those comments. This could evenutally be fixed by
                // implementing a peekToken() / consumeToken() API, so that peeking at the next
                // token won't skip over comments at declaration scope.
                // [Update 2020-06-07: atDeclarationScope is now a member of Parser. This opens the
                // possibility of a new approach: Just set parser->atDeclarationScope = true while
                // checking ahead for a comma.]
                break; // Stop parsing declarators immediately after the function body!
            } else {
                parseOptionalVariableInitializer(parser, initDcor.init, true);
            }
            Token sepToken = readToken(parser);
            if (sepToken.type == Token::Comma) {
                if (initDcor.dcor.isFunction()) {
                    // FIXME: It's not very clear from this error message that the comma is the
                    // token that triggered an error. In any case, we don't hit this codepath yet,
                    // as explained by the above comment.
                    PLY_ASSERT(0); // codepath never gets hit at the moment
                    parser->error(false, {ParseError::CantMixFunctionDefAndDecl,
                                          initDcor.dcor.prod->type.function()->params.openPunc});
                }
                initDcor.comma = sepToken;
            } else {
                pushBackToken(parser, sepToken);
                break;
            }
        }
    } else {
        // If it's a Param or TemplateParam, there should be one (possibly abstract) declarator,
        // possibly with initializer. If it's a TypeID or ConversionTypeID, there should be one
        // abstract declarator.
        grammar::InitDeclaratorWithComma& initDcor = simple.initDeclarators.append();
        u32 dcorFlags = DeclaratorFlags::AllowNamed | DeclaratorFlags::AllowAbstract;
        if (mode.isAnyTypeID()) {
            dcorFlags = DeclaratorFlags::AllowAbstract;
        }
        parseDeclarator(parser, initDcor.dcor, nullptr, dcorFlags);
        if (mode.isAnyParam()) {
            // Note: If this is a template parameter, it must be a non-type parameter. Type
            // parameters are parsed in parseOptionalTypeIDInitializer.
            parseOptionalVariableInitializer(parser, initDcor.init, false);
        }
    }
}

Array<grammar::BaseSpecifierWithComma> parseBaseSpecifierList(Parser* parser) {
    Array<grammar::BaseSpecifierWithComma> baseSpecifierList;
    for (;;) {
        grammar::BaseSpecifierWithComma baseSpec;

        // Optional access specifier
        Token token = readToken(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "public" || token.identifier == "private" ||
                token.identifier == "protected") {
                parser->stopMutingErrors();
                baseSpec.accessSpec = token;
                token = readToken(parser);
            }
        }
        pushBackToken(parser, token);

        // Qualified ID
        baseSpec.baseQid = parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
        if (baseSpec.baseQid.unqual.empty())
            break;
        parser->stopMutingErrors();
        grammar::BaseSpecifierWithComma& addedBS = baseSpecifierList.append(std::move(baseSpec));

        // Comma or {
        Token puncToken = readToken(parser);
        if (puncToken.type == Token::OpenCurly) {
            pushBackToken(parser, puncToken);
            break;
        } else if (puncToken.type == Token::Comma) {
            addedBS.comma = token;
        } else {
            parser->error(true, {ParseError::Expected, puncToken, ExpectedToken::CommaOrOpenCurly});
            break;
        }
    }
    return baseSpecifierList;
}

bool looksLikeCtorDtor(StringView enclosingClassName, const grammar::QualifiedID& qid) {
    if (enclosingClassName.isEmpty()) {
        if (qid.nestedName.numItems() < 1)
            return false;
        StringView ctorDtorName = qid.unqual.getCtorDtorName();
        if (ctorDtorName.isEmpty())
            return false;
        const grammar::NestedNameComponent& tail = qid.nestedName.back();
        auto ident = tail.type.identifierOrTemplated();
        if (!ident)
            return false;
        PLY_ASSERT(ident->name.isValid());
        return ctorDtorName == ident->name.identifier;
    } else {
        if (qid.nestedName.numItems() > 0)
            return false;
        StringView ctorDtorName = qid.unqual.getCtorDtorName();
        return ctorDtorName == enclosingClassName;
    }
}

//-------------------------------------------------------------------------------------
// parseSpecifiersAndDeclarators
//-------------------------------------------------------------------------------------
void parseSpecifiersAndDeclarators(Parser* parser, grammar::Declaration::Simple& simple,
                                   const SpecDcorMode& mode) {
    // First, read decl specifier sequence.
    s32 typeSpecifierIndex = -1;
    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "extern") {
                parser->stopMutingErrors();
                Token literal = readToken(parser);
                if (literal.type == Token::StringLiteral) {
                    simple.declSpecifierSeq.append(new grammar::DeclSpecifier{
                        grammar::DeclSpecifier::LangLinkage{token, literal}});
                } else {
                    simple.declSpecifierSeq.append(
                        new grammar::DeclSpecifier{grammar::DeclSpecifier::Keyword{token}});
                    pushBackToken(parser, literal);
                }
            } else if (token.identifier == "inline" || token.identifier == "const" ||
                       token.identifier == "volatile" || token.identifier == "static" ||
                       token.identifier == "friend" || token.identifier == "virtual" ||
                       token.identifier == "constexpr" || token.identifier == "thread_local" ||
                       token.identifier == "unsigned" || token.identifier == "mutable" ||
                       token.identifier == "explicit") {
                parser->stopMutingErrors();
                simple.declSpecifierSeq.append(
                    new grammar::DeclSpecifier{grammar::DeclSpecifier::Keyword{token}});
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       token.identifier == "alignas") {
                parser->stopMutingErrors();
                // FIXME: Implement DeclSpecifier::AlignAs
                // Note: alignas is technically part of the attribute-specifier-seq in the
                // grammar, which means it can only appear before the decl-specifier-seq. But
                // for now, let's just accept it here:
                Token openParen = readToken(parser);
                if (openParen.type != Token::OpenParen) {
                    parser->error(
                        true, {ParseError::Expected, openParen, ExpectedToken::OpenParen, token});
                    continue;
                }
                // FIXME: Accept integral constant expression here too
                grammar::Declaration::Simple simple;
                parseSpecifiersAndDeclarators(parser, simple, SpecDcorMode::TypeID);
                Token closeParen;
                if (!closeScope(parser, &closeParen, openParen))
                    return;
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       token.identifier == "typedef") {
                parser->stopMutingErrors();
                // FIXME: Store this token in the parse tree
            } else if ((mode.mode != SpecDcorMode::TemplateParam) &&
                       (token.identifier == "struct" || token.identifier == "class" ||
                        token.identifier == "union")) {
                parser->stopMutingErrors();
                // FIXME: for TemplateParams, "class" should be treated like "typename".
                // Otherwise, it seems C++20 may actually support structs as non-type template
                // parameters, so we should revisit this eventually.
                if (typeSpecifierIndex >= 0) {
                    // Already got type specifier
                    // Should this be a higher-level error?
                    // Note: We mute errors here, but we probably don't want to mute lower-level
                    // (syntactic) errors; we mainly just want to mute subsequent
                    // TooManyTypeSpecifiers for the same declaration. Could be another reason to
                    // log this error at some kind of higher level.
                    parser->error(true, {ParseError::TooManyTypeSpecifiers, token});
                }
                grammar::DeclSpecifier::Record record;
                record.classKey = token;

                record.qid = parseQualifiedID(parser, ParseQualifiedMode::RequireCompleteOrEmpty);

                token = readToken(parser);
                if (token.type == Token::SingleColon) {
                    record.colon = token;
                    record.baseSpecifierList = parseBaseSpecifierList(parser);
                    token = readToken(parser);
                }

                if (token.type == Token::OpenCurly) {
                    record.openCurly = token;
                    parser->visor->doEnter(TypedPtr::bind(&record));
                    parseDeclarationList(parser, &record.closeCurly, record.qid.getClassName());
                    parser->visor->doExit(TypedPtr::bind(&record));
                } else {
                    pushBackToken(parser, token);
                }
                typeSpecifierIndex = simple.declSpecifierSeq.numItems();
                simple.declSpecifierSeq.append(new grammar::DeclSpecifier{std::move(record)});
            } else if ((mode.mode != SpecDcorMode::TemplateParam) && (token.identifier == "enum")) {
                parser->stopMutingErrors();
                if (typeSpecifierIndex >= 0) {
                    // Already got type specifier
                    // Should this be a higher-level error?
                    // Note: We mute errors here, but we probably don't want to mute lower-level
                    // (syntactic) errors; we mainly just want to mute subsequent
                    // TooManyTypeSpecifiers for the same declaration. Could be another reason to
                    // log this error at some kind of higher level.
                    parser->error(true, {ParseError::TooManyTypeSpecifiers, token});
                }
                grammar::DeclSpecifier::Enum_ en;
                en.enumKey = token;
                Token token2 = readToken(parser);
                if ((token2.type == Token::Identifier) && (token2.identifier == "class")) {
                    en.classKey = token2;
                } else {
                    pushBackToken(parser, token2);
                }

                en.qid = parseQualifiedID(parser, ParseQualifiedMode::RequireCompleteOrEmpty);

                Token sepToken = readToken(parser);
                if (sepToken.type == Token::SingleColon) {
                    if (en.qid.isEmpty()) {
                        parser->error(false, {ParseError::ScopedEnumRequiresName, sepToken});
                    }
                    en.basePunc = sepToken;
                    en.base = parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
                } else {
                    pushBackToken(parser, sepToken);
                }

                Token token3 = readToken(parser);
                if (token3.type == Token::OpenCurly) {
                    en.openCurly = token3;
                    parser->visor->doEnter(TypedPtr::bind(&en));
                    parseEnumBody(parser, &en);
                    parser->visor->doExit(TypedPtr::bind(&en));
                } else {
                    pushBackToken(parser, token3);
                }

                typeSpecifierIndex = simple.declSpecifierSeq.numItems();
                simple.declSpecifierSeq.append(new grammar::DeclSpecifier{std::move(en)});
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       (token.identifier == "operator") && (typeSpecifierIndex < 0)) {
                parser->stopMutingErrors();
                // It's a conversion function
                grammar::InitDeclaratorWithComma& initDcor = simple.initDeclarators.append();
                auto convFunc = initDcor.dcor.qid.unqual.conversionFunc().switchTo();
                convFunc->keyword = token;
                parseConversionTypeID(parser, convFunc.get());
                // Ensure there's an open parenthesis
                Token openParen = readToken(parser);
                pushBackToken(parser, openParen);
                if (openParen.type == Token::OpenParen) {
                    initDcor.dcor.prod = new grammar::DeclaratorProduction;
                    auto func = initDcor.dcor.prod->type.function().switchTo();
                    parseParameterDeclarationList(parser, func->params, false);
                    func->qualifiers = parseFunctionQualifierSeq(parser);
                    parseOptionalFunctionBody(parser, initDcor.init, simple);
                } else {
                    parser->error(true,
                                  {ParseError::Expected, openParen, ExpectedToken::OpenParen});
                }
                return;
            } else {
                parser->stopMutingErrors();
                if (typeSpecifierIndex >= 0) {
                    // We already got a type specifier, so this must be the declarator part.
                    pushBackToken(parser, token);
                    parseInitDeclarators(parser, simple, mode);
                    return;
                }

                Token typename_;
                grammar::QualifiedID qid;
                if (token.identifier == "typename") {
                    typename_ = token;
                    Token ellipsis;
                    Token t2 = readToken(parser);
                    if (t2.type == Token::Ellipsis) {
                        ellipsis = t2;
                    } else {
                        pushBackToken(parser, t2);
                    }
                    qid = parseQualifiedID(parser, ParseQualifiedMode::RequireCompleteOrEmpty);
                    if (mode.mode == SpecDcorMode::TemplateParam &&
                        simple.declSpecifierSeq.numItems() == 0 && qid.nestedName.numItems() == 0) {
                        // Parse it as a type parameter
                        grammar::DeclSpecifier* declSpec =
                            simple.declSpecifierSeq.append(new grammar::DeclSpecifier);
                        auto typeParam = declSpec->typeParam().switchTo();
                        typeParam->keyword = token;
                        typeParam->ellipsis = ellipsis;
                        if (auto ident = qid.unqual.identifier()) {
                            typeParam->identifier = ident->name;
                        } else if (!qid.unqual.empty()) {
                            parser->error(true, {ParseError::Expected, qid.unqual.getFirstToken(),
                                                 ExpectedToken::Identifier});
                            return;
                        }
                        grammar::InitDeclaratorWithComma& initDcor =
                            simple.initDeclarators.append();
                        initDcor.dcor.qid = std::move(qid);
                        parseOptionalTypeIDInitializer(parser, initDcor.init);
                        return;
                    }
                    if (ellipsis.isValid()) {
                        parser->error(true,
                                      {ParseError::Expected, ellipsis, ExpectedToken::QualifiedID});
                    }
                } else {
                    pushBackToken(parser, token);
                    qid = parseQualifiedID(parser, ParseQualifiedMode::RequireComplete);
                    PLY_ASSERT(!qid.isEmpty()); // Shouldn't happen because token was an Identifier
                }

                if ((mode.mode == SpecDcorMode::GlobalOrMember) && !typename_.isValid() &&
                    looksLikeCtorDtor(mode.enclosingClassName, qid)) {
                    // Try (optimistically) to parse it as a constructor.
                    // We need a restore point in order to recover from Foo(bar())
                    RestorePoint rp{parser};
                    grammar::Declarator ctorDcor;
                    Owned<grammar::DeclaratorProduction>* prodToModify = &ctorDcor.prod;
                    parseParameterList(parser, &prodToModify);
                    if (!rp.errorOccurred()) {
                        // It's a constructor
                        PLY_ASSERT(ctorDcor.prod && ctorDcor.prod->type.function());
                        rp.cancel();
                        grammar::InitDeclaratorWithComma& initDcor =
                            simple.initDeclarators.append();
                        initDcor.dcor = std::move(ctorDcor);
                        PLY_ASSERT(initDcor.dcor.qid.isEmpty());
                        initDcor.dcor.qid = std::move(qid);
                        parseOptionalFunctionBody(parser, initDcor.init, simple);
                        return;
                    }
                    // It failed to parse as a constructor. Treat this token as part of a
                    // simple type specifier instead.
                    rp.backtrack();
                }

                // In C++, all declarations must be explicitly typed; there is no "default
                // int". Therefore, this must be a simple type specifier.
                if (typename_.isValid() && qid.nestedName.isEmpty()) {
                    parser->error(true, {ParseError::Expected, qid.getFirstToken(),
                                         ExpectedToken::NestedNamePrefix});
                }
                // FIXME: If mode == SpecDcorMode::Param, we should check at this point that qid
                // actually refers to a type (if possible!). Consider for example (inside class
                // 'Foo'):
                //      Foo(baz())
                // If 'baz' refers to a type, it's a constructor. Otherwise, it's a function 'baz'
                // returning Foo. If it's not possible to determine in this pass, we obviously have
                // to guess (leaning towards it being a constructor), but the parse tree should
                // store the fact that we guessed somewhere.
                typeSpecifierIndex = simple.declSpecifierSeq.numItems();
                grammar::DeclSpecifier* declSpec =
                    simple.declSpecifierSeq.append(new grammar::DeclSpecifier);
                auto typeID = declSpec->typeID().switchTo();
                typeID->typename_ = typename_;
                typeID->qid = std::move(qid);
                /*
                token = readToken(parser);
                if (token.type == Token::Ellipsis) {
                    // Template parameter packs are detected here
                    // FIXME: Improve the parser here!
                    PLY_ASSERT(mode.mode == SpecDcorMode::TemplateParam);
                } else {
                    pushBackToken(parser, token);
                }
                */
            }
        } else {
            // Not an identifier.
            // This must be the declarator part. (eg. may start with * or &)
            // Do we have a type specifier yet?
            if (typeSpecifierIndex < 0) {
                switch (mode.mode) {
                    case SpecDcorMode::GlobalOrMember:
                        // If parsing a global or member, don't log an error if no type specifier
                        // was encountered yet, because the declarator may name a destructor. Leave
                        // it to higher-level code to verify there are type specifiers when needed.
                        break;
                    case SpecDcorMode::Param:
                        parser->error(true,
                                      {ParseError::Expected, token, ExpectedToken::ParameterType});
                        break;
                    case SpecDcorMode::TemplateParam:
                        parser->error(true, {ParseError::Expected, token,
                                             ExpectedToken::TemplateParameterDecl});
                        break;
                    case SpecDcorMode::TypeID:
                    case SpecDcorMode::ConversionTypeID:
                        parser->error(true,
                                      {ParseError::Expected, token, ExpectedToken::TypeSpecifier});
                        break;
                    default:
                        PLY_ASSERT(0);
                        break;
                }
            }
            pushBackToken(parser, token);
            parseInitDeclarators(parser, simple, mode);
            return;
        }
    } // for loop
}

} // namespace cpp
} // namespace ply
