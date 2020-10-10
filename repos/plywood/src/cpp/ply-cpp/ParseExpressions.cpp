/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>

namespace ply {
namespace cpp {

void consumeSpecifier(Parser* parser) {
    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::OpenAngle) {
            // Template type
            // FIXME: Does < always indicate a template type here?
            // FIXME: This needs to handle "Tmpl<(2 > 1)>" and ""Tmpl<(2 >> 1)>"
            PLY_SET_IN_SCOPE(parser->pp->tokenizeCloseAnglesOnly, true);
            Token closeToken;
            skipAnyScope(parser, &closeToken, token);
            token = readToken(parser);
        }
        if (token.type == Token::DoubleColon) {
            Token specToken = readToken(parser);
            if (specToken.type != Token::Identifier) {
                // expected identifier after ::
                parser->error(true, {ParseError::Expected, specToken, ExpectedToken::Identifier});
                pushBackToken(parser, specToken);
                return;
            }
        } else {
            pushBackToken(parser, token);
            return;
        }
    }
}

void parseCaptureList(Parser* parser) {
    Token token = readToken(parser);
    if (token.type != Token::CloseSquare) {
        // FIXME: accept an actual capture list instead of just an empty list
        parser->error(true, {ParseError::Expected, token, ExpectedToken::CloseSquare});
    }
}

// FIXME: This needs work.
// It's enough to parse the initializers used by Plywood, but there are definitely lots of
// expressions it doesn't handle.
Tuple<Token, Token> parseExpression(Parser* parser, bool optional) {
    Token startToken = readToken(parser);
    Token endToken;
    switch (startToken.type) {
        case Token::Identifier: {
            // FIXME: This should use parseQualifiedID instead
            consumeSpecifier(parser);
            Token token2 = readToken(parser);
            if (token2.type == Token::OpenParen) {
                // Function arguments
                SetAcceptFlagsInScope acceptScope{parser, Token::OpenParen};
                for (;;) {
                    Token token3 = readToken(parser);
                    if (token3.type == Token::CloseParen) {
                        endToken = token3;
                        break; // end of arguments
                    } else {
                        pushBackToken(parser, token3);
                        parseExpression(parser);
                        Token token4 = readToken(parser);
                        if (token4.type == Token::Comma) {
                        } else if (token4.type == Token::CloseParen) {
                            endToken = token4;
                            break; // end of arguments
                        } else {
                            // expected , or ) after argument
                            parser->error(true, {ParseError::Expected, token4,
                                                 ExpectedToken::CommaOrCloseParen});
                            if (!handleUnexpectedToken(parser, nullptr, token4))
                                break;
                        }
                    }
                }
            } else if (token2.type == Token::OpenCurly) {
                // It's a braced initializer (list).
                // FIXME: Not sure, but maybe this case should use a "low priority" curly (???)
                // Because if ';' is encountered, we should perhaps end the outer declaration.
                // And if an outer ) is matched, it should maybe cancel the initializer.
                // However, if we do that, it will be inconsisent with the behavior of
                // skipAnyScope(). Does that matter?
                SetAcceptFlagsInScope acceptScope{parser, Token::OpenCurly};
                for (;;) {
                    Token token3 = readToken(parser);
                    if (token3.type == Token::CloseCurly) {
                        endToken = token3;
                        break; // end of arguments
                    } else {
                        pushBackToken(parser, token3);
                        parseExpression(parser);
                        Token token4 = readToken(parser);
                        if (token4.type == Token::Comma) {
                        } else if (token4.type == Token::CloseCurly) {
                            endToken = token4;
                            break; // end of arguments
                        } else {
                            // expected , or } after argument
                            parser->error(true, {ParseError::Expected, token4,
                                                 ExpectedToken::CommaOrCloseCurly});
                            if (!handleUnexpectedToken(parser, nullptr, token4))
                                break;
                        }
                    }
                }
            } else {
                // Can't consume any more of expression
                endToken = startToken;
                pushBackToken(parser, token2);
            }
            break;
        }

        case Token::NumericLiteral: {
            // Consume it
            endToken = startToken;
            break;
        }

        case Token::StringLiteral: {
            endToken = startToken;
            for (;;) {
                // concatenate multiple string literals
                Token token = readToken(parser);
                if (token.type != Token::StringLiteral) {
                    pushBackToken(parser, token);
                    break;
                }
                endToken = token;
            }
            break;
        }

        case Token::OpenParen: {
            SetAcceptFlagsInScope acceptScope{parser, Token::OpenParen};
            parseExpression(parser);
            Token token2 = readToken(parser);
            if (token2.type == Token::CloseParen) {
                // Treat as a C-style cast.
                // FIXME: This should only be done if the inner expression identifies a type!
                // Otherwise, it's just a parenthesized expression:
                endToken = parseExpression(parser, true).second;
            } else {
                // expected ) after expression
                pushBackToken(parser, token2);
                Token closeParen;
                closeScope(parser, &closeParen, startToken); // This will log an error
                endToken = closeParen;
            }
            break;
        }

        case Token::OpenCurly: {
            for (;;) {
                Token token2 = readToken(parser);
                if (token2.type == Token::CloseCurly) {
                    endToken = token2;
                    break;
                } else {
                    pushBackToken(parser, token2);
                    parseExpression(parser);
                    Token token4 = readToken(parser);
                    if (token4.type == Token::Comma) {
                    } else if (token4.type == Token::CloseCurly) {
                        endToken = token4;
                        break; // end of braced initializer
                    } else {
                        // expected , or } after expression
                        parser->error(
                            true, {ParseError::Expected, token4, ExpectedToken::CommaOrCloseCurly});
                        if (!handleUnexpectedToken(parser, nullptr, token4))
                            break;
                    }
                }
            }
            break;
        }

        case Token::Bang:
        case Token::SingleAmpersand:
        case Token::SingleMinus: {
            endToken = parseExpression(parser).second;
            break;
        }

        case Token::OpenSquare: {
            // lambda expression
            parseCaptureList(parser);
            Token openParen = readToken(parser);
            pushBackToken(parser, openParen);
            if (openParen.type == Token::OpenParen) {
                grammar::ParamDeclarationList unusedParams;
                parseParameterDeclarationList(parser, unusedParams, false);
            } else {
                // FIXME: Could be cool to state, in this error message, that the '(' is needed for
                // the parameter list of a lambda expression:
                parser->error(true, {ParseError::Expected, openParen, ExpectedToken::OpenParen});
            }
            Token token2 = readToken(parser);
            if (token2.type == Token::Arrow) {
                grammar::Declaration::Simple simple;
                parseSpecifiersAndDeclarators(parser, simple, SpecDcorMode::TypeID);
                token2 = readToken(parser);
            }
            if (token2.type != Token::OpenCurly) {
                // FIXME: Could be cool to state, in this error message, that the '{' is needed for
                // the body of a lambda expression:
                parser->error(true, {ParseError::Expected, token2, ExpectedToken::OpenCurly});
                pushBackToken(parser, token2);
            } else {
                Token closeToken;
                skipAnyScope(parser, &closeToken, token2);
                endToken = closeToken;
            }
            break;
        }

        default: {
            if (optional) {
                pushBackToken(parser, startToken);
            } else {
                PLY_ASSERT(0);
            }
            return {};
        }
    }

    Token token = readToken(parser);
    switch (token.type) {
        case Token::CloseAngle: {
            if (parser->pp->tokenizeCloseAnglesOnly) {
                pushBackToken(parser, token);
                break;
            } else {
                endToken = parseExpression(parser).second;
            }
        };

        case Token::SingleVerticalBar:
        case Token::DoubleEqual:
        case Token::NotEqual:
        case Token::OpenAngle:
        case Token::LessThanOrEqual:
        case Token::GreaterThanOrEqual:
        case Token::LeftShift:
        case Token::RightShift:
        case Token::SinglePlus:
        case Token::SingleMinus:
        case Token::Percent:
        case Token::Arrow:
        case Token::Star:
        case Token::Dot:
        case Token::ForwardSlash: {
            endToken = parseExpression(parser).second;
            break;
        }

        case Token::QuestionMark: {
            parseExpression(parser);
            token = readToken(parser);
            if (token.type != Token::SingleColon) {
                // expected : after expression
                // FIXME: It would be cool the mention, in the error message, that the colon is
                // needed to match the '?' that was encountered earlier
                parser->error(true, {ParseError::Expected, token, ExpectedToken::Colon});
                pushBackToken(parser, token);
            } else {
                endToken = parseExpression(parser).second;
            }
            break;
        };

        default: {
            pushBackToken(parser, token);
            break;
        }
    }
    return {startToken, endToken};
}

} // namespace cpp
} // namespace ply
