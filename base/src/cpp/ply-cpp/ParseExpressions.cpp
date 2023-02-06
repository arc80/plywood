/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>

namespace ply {
namespace cpp {

void consume_specifier(Parser* parser) {
    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::OpenAngle) {
            // Template type
            // FIXME: Does < always indicate a template type here?
            // FIXME: This needs to handle "Tmpl<(2 > 1)>" and ""Tmpl<(2 >> 1)>"
            PLY_SET_IN_SCOPE(parser->pp->tokenize_close_angles_only, true);
            Token close_token;
            skip_any_scope(parser, &close_token, token);
            token = read_token(parser);
        }
        if (token.type == Token::DoubleColon) {
            Token spec_token = read_token(parser);
            if (spec_token.type != Token::Identifier) {
                // expected identifier after ::
                parser->error(true, {ParseError::Expected, spec_token,
                                     ExpectedToken::Identifier});
                push_back_token(parser, spec_token);
                return;
            }
        } else {
            push_back_token(parser, token);
            return;
        }
    }
}

void parse_capture_list(Parser* parser) {
    Token token = read_token(parser);
    if (token.type != Token::CloseSquare) {
        // FIXME: accept an actual capture list instead of just an empty list
        parser->error(true, {ParseError::Expected, token, ExpectedToken::CloseSquare});
    }
}

// FIXME: This needs work.
// It's enough to parse the initializers used by Plywood, but there are definitely lots
// of expressions it doesn't handle.
Tuple<Token, Token> parse_expression(Parser* parser, bool optional) {
    Token start_token = read_token(parser);
    Token end_token;
    switch (start_token.type) {
        case Token::Identifier: {
            // FIXME: This should use parse_qualified_id instead
            consume_specifier(parser);
            Token token2 = read_token(parser);
            if (token2.type == Token::OpenParen) {
                // Function arguments
                SetAcceptFlagsInScope accept_scope{parser, Token::OpenParen};
                for (;;) {
                    Token token3 = read_token(parser);
                    if (token3.type == Token::CloseParen) {
                        end_token = token3;
                        break; // end of arguments
                    } else {
                        push_back_token(parser, token3);
                        parse_expression(parser);
                        Token token4 = read_token(parser);
                        if (token4.type == Token::Comma) {
                        } else if (token4.type == Token::CloseParen) {
                            end_token = token4;
                            break; // end of arguments
                        } else {
                            // expected , or ) after argument
                            parser->error(true, {ParseError::Expected, token4,
                                                 ExpectedToken::CommaOrCloseParen});
                            if (!handle_unexpected_token(parser, nullptr, token4))
                                break;
                        }
                    }
                }
            } else if (token2.type == Token::OpenCurly) {
                // It's a braced initializer (list).
                // FIXME: Not sure, but maybe this case should use a "low priority"
                // curly (???) Because if ';' is encountered, we should perhaps end the
                // outer declaration. And if an outer ) is matched, it should maybe
                // cancel the initializer. However, if we do that, it will be
                // inconsisent with the behavior of skip_any_scope(). Does that matter?
                SetAcceptFlagsInScope accept_scope{parser, Token::OpenCurly};
                for (;;) {
                    Token token3 = read_token(parser);
                    if (token3.type == Token::CloseCurly) {
                        end_token = token3;
                        break; // end of arguments
                    } else {
                        push_back_token(parser, token3);
                        parse_expression(parser);
                        Token token4 = read_token(parser);
                        if (token4.type == Token::Comma) {
                        } else if (token4.type == Token::CloseCurly) {
                            end_token = token4;
                            break; // end of arguments
                        } else {
                            // expected , or } after argument
                            parser->error(true, {ParseError::Expected, token4,
                                                 ExpectedToken::CommaOrCloseCurly});
                            if (!handle_unexpected_token(parser, nullptr, token4))
                                break;
                        }
                    }
                }
            } else {
                // Can't consume any more of expression
                end_token = start_token;
                push_back_token(parser, token2);
            }
            break;
        }

        case Token::NumericLiteral: {
            // Consume it
            end_token = start_token;
            break;
        }

        case Token::StringLiteral: {
            end_token = start_token;
            for (;;) {
                // concatenate multiple string literals
                Token token = read_token(parser);
                if (token.type != Token::StringLiteral) {
                    push_back_token(parser, token);
                    break;
                }
                end_token = token;
            }
            break;
        }

        case Token::OpenParen: {
            SetAcceptFlagsInScope accept_scope{parser, Token::OpenParen};
            parse_expression(parser);
            Token token2 = read_token(parser);
            if (token2.type == Token::CloseParen) {
                // Treat as a C-style cast.
                // FIXME: This should only be done if the inner expression identifies a
                // type! Otherwise, it's just a parenthesized expression:
                end_token = parse_expression(parser, true).second;
            } else {
                // expected ) after expression
                push_back_token(parser, token2);
                Token close_paren;
                close_scope(parser, &close_paren,
                            start_token); // This will log an error
                end_token = close_paren;
            }
            break;
        }

        case Token::OpenCurly: {
            for (;;) {
                Token token2 = read_token(parser);
                if (token2.type == Token::CloseCurly) {
                    end_token = token2;
                    break;
                } else {
                    push_back_token(parser, token2);
                    parse_expression(parser);
                    Token token4 = read_token(parser);
                    if (token4.type == Token::Comma) {
                    } else if (token4.type == Token::CloseCurly) {
                        end_token = token4;
                        break; // end of braced initializer
                    } else {
                        // expected , or } after expression
                        parser->error(true, {ParseError::Expected, token4,
                                             ExpectedToken::CommaOrCloseCurly});
                        if (!handle_unexpected_token(parser, nullptr, token4))
                            break;
                    }
                }
            }
            break;
        }

        case Token::Bang:
        case Token::SingleAmpersand:
        case Token::SingleMinus: {
            end_token = parse_expression(parser).second;
            break;
        }

        case Token::OpenSquare: {
            // lambda expression
            parse_capture_list(parser);
            Token open_paren = read_token(parser);
            push_back_token(parser, open_paren);
            if (open_paren.type == Token::OpenParen) {
                grammar::ParamDeclarationList unused_params;
                parse_parameter_declaration_list(parser, unused_params, false);
            } else {
                // FIXME: Could be cool to state, in this error message, that the '(' is
                // needed for the parameter list of a lambda expression:
                parser->error(
                    true, {ParseError::Expected, open_paren, ExpectedToken::OpenParen});
            }
            Token token2 = read_token(parser);
            if (token2.type == Token::Arrow) {
                grammar::Declaration::Simple simple;
                parse_specifiers_and_declarators(parser, simple, SpecDcorMode::TypeID);
                token2 = read_token(parser);
            }
            if (token2.type != Token::OpenCurly) {
                // FIXME: Could be cool to state, in this error message, that the '{' is
                // needed for the body of a lambda expression:
                parser->error(true,
                              {ParseError::Expected, token2, ExpectedToken::OpenCurly});
                push_back_token(parser, token2);
            } else {
                Token close_token;
                skip_any_scope(parser, &close_token, token2);
                end_token = close_token;
            }
            break;
        }

        default: {
            if (optional) {
                push_back_token(parser, start_token);
            } else {
                PLY_ASSERT(0);
            }
            return {};
        }
    }

    Token token = read_token(parser);
    switch (token.type) {
        case Token::CloseAngle: {
            if (parser->pp->tokenize_close_angles_only) {
                push_back_token(parser, token);
                break;
            } else {
                end_token = parse_expression(parser).second;
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
            end_token = parse_expression(parser).second;
            break;
        }

        case Token::QuestionMark: {
            parse_expression(parser);
            token = read_token(parser);
            if (token.type != Token::SingleColon) {
                // expected : after expression
                // FIXME: It would be cool the mention, in the error message, that the
                // colon is needed to match the '?' that was encountered earlier
                parser->error(true,
                              {ParseError::Expected, token, ExpectedToken::Colon});
                push_back_token(parser, token);
            } else {
                end_token = parse_expression(parser).second;
            }
            break;
        };

        default: {
            push_back_token(parser, token);
            break;
        }
    }
    return {start_token, end_token};
}

} // namespace cpp
} // namespace ply
