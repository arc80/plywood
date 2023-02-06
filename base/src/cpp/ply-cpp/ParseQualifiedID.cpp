/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/RestorePoint.h>

namespace ply {
namespace cpp {

// Consumes as much as it can; unrecognized tokens are returned to caller without
// logging an error
Array<grammar::NestedNameComponent> parse_nested_name_specifier(Parser* parser) {
    // FIXME: Support leading ::
    Array<grammar::NestedNameComponent> nested_name;
    for (;;) {
        grammar::NestedNameComponent* comp = nullptr;

        Token token = read_token(parser);
        if (token.type != Token::Identifier) {
            push_back_token(parser, token);
            break;
        }
        if (token.identifier == "operator" || token.identifier == "const" ||
            token.identifier == "volatile" || token.identifier == "inline" ||
            token.identifier == "static" || token.identifier == "friend") {
            push_back_token(parser, token);
            break;
        }

        if (token.identifier == "decltype") {
            comp = &nested_name.append();
            auto decl_type = comp->type.decl_type().switch_to();
            decl_type->keyword = token;
            Token punc_token = read_token(parser);
            if (punc_token.type == Token::OpenParen) {
                decl_type->open_paren = punc_token;
                skip_any_scope(parser, &decl_type->close_paren, punc_token);
            } else {
                // expected (
                parser->error(true, {ParseError::Expected, punc_token,
                                     ExpectedToken::OpenParen, token});
                push_back_token(parser, punc_token);
            }
        } else {
            comp = &nested_name.append();
            auto ident = comp->type.identifier_or_templated().switch_to();
            ident->name = token;
            Token punc_token = read_token(parser);
            if (punc_token.type == Token::OpenAngle) {
                // FIXME: We should only parse < as the start of a template-argument
                // list if we know that the preceding name refers to a template function
                // or type. For now, we assume it always does. If we ever start parsing
                // function bodies, we won't be able to assume this.
                if (parser->pass_number <= 1) {
                    ident->open_angled = punc_token;

                    // Parse template-argument-list
                    SetAcceptFlagsInScope accept_scope{parser, Token::OpenAngle};
                    PLY_SET_IN_SCOPE(parser->pp->tokenize_close_angles_only, true);

                    for (;;) {
                        // FIXME: Parse constant expressions here instead of only
                        // allowing type IDs

                        // Try to parse a type ID
                        grammar::TemplateArgumentWithComma& template_arg =
                            ident->args.append();
                        Token start_token = read_token(parser);
                        template_arg.type.unknown()->start_token = start_token;
                        push_back_token(parser, start_token);
                        RestorePoint rp{parser};
                        grammar::Declaration::Simple simple;
                        parse_specifiers_and_declarators(parser, simple,
                                                         SpecDcorMode::TypeID);
                        if (!rp.error_occurred()) {
                            // Successfully parsed a type ID
                            auto type_id = template_arg.type.type_id().switch_to();
                            type_id->decl_specifier_seq =
                                std::move(simple.decl_specifier_seq);
                            PLY_ASSERT(simple.init_declarators.num_items() == 1);
                            PLY_ASSERT(simple.init_declarators[0].dcor.qid.is_empty());
                            type_id->abstract_dcor =
                                std::move(simple.init_declarators[0].dcor.prod);
                        } else {
                            rp.backtrack();
                            rp.cancel();
                        }

                        auto unknown = template_arg.type.unknown();
                        for (;;) {
                            Token sep_token = read_token(parser);
                            if (sep_token.type == Token::CloseAngle) {
                                // End of template-argument-list
                                ident->close_angled = sep_token;
                                parser->stop_muting_errors();
                                goto break_args;
                            } else if (sep_token.type == Token::Comma) {
                                // Comma
                                template_arg.comma = sep_token;
                                parser->stop_muting_errors();
                                break;
                            } else {
                                // Unexpected token
                                if (!unknown) {
                                    unknown.switch_to();
                                }
                                unknown->end_token = sep_token;
                                if (!handle_unexpected_token(
                                        parser, &unknown->end_token, sep_token))
                                    goto break_outer;
                            }
                        }
                    }
                break_args:;
                } else {
                    PLY_FORCE_CRASH(); // FIXME: implement this
                }
            } else {
                push_back_token(parser, punc_token);
            }
        }

        PLY_ASSERT(comp);
        Token sep_token = read_token(parser);
        if (sep_token.type == Token::DoubleColon) {
            comp->sep = sep_token;
        } else {
            push_back_token(parser, sep_token);
            break;
        }
    }

break_outer:
    return nested_name;
}

// Consumes as much as it can; unrecognized tokens are returned to caller without
// logging an error
grammar::QualifiedID parse_qualified_id(Parser* parser, ParseQualifiedMode mode) {
    grammar::QualifiedID qid;
    qid.nested_name = parse_nested_name_specifier(parser);
    if (qid.nested_name.num_items() > 0) {
        grammar::NestedNameComponent& tail = qid.nested_name.back();
        if (!tail.sep.is_valid()) {
            if (auto ident = tail.type.identifier_or_templated()) {
                if (ident->open_angled.is_valid()) {
                    auto tmpl = qid.unqual.template_id().switch_to();
                    tmpl->name = ident->name;
                    tmpl->open_angled = ident->open_angled;
                    tmpl->close_angled = ident->close_angled;
                    tmpl->args = std::move(ident->args);
                } else {
                    auto id = qid.unqual.identifier().switch_to();
                    id->name = ident->name;
                }
            } else if (auto decl_type = tail.type.decl_type()) {
                auto dst = qid.unqual.decl_type().switch_to();
                dst->keyword = decl_type->keyword;
                dst->open_paren = decl_type->open_paren;
                dst->close_paren = decl_type->close_paren;
            }
            qid.nested_name.pop();
        }
    }
    if (qid.unqual.empty()) {
        Token token = read_token(parser);
        if (token.type == Token::Tilde) {
            Token token2 = read_token(parser);
            if (token2.type != Token::Identifier) {
                // Expected class name after ~
                parser->error(true, {ParseError::Expected, token2,
                                     ExpectedToken::DestructorClassName, token});
                push_back_token(parser, token2);
            } else {
                auto dtor = qid.unqual.destructor().switch_to();
                PLY_ASSERT(token2.identifier != "decltype"); // FIXME: Support this
                dtor->tilde = token;
                dtor->name = token2;
            }
        } else if (token.type == Token::Identifier) {
            if (token.identifier == "operator") {
                auto op_func = qid.unqual.operator_func().switch_to();
                op_func->keyword = token;
                Token op_token = read_token(parser);
                switch (op_token.type) {
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
                        op_func->punc = op_token;
                        if (op_token.type == Token::OpenParen) {
                            Token op_token2 = read_token(parser);
                            if (op_token2.type == Token::CloseParen) {
                                op_func->punc2 = op_token2;
                            } else {
                                // Expected ) after (
                                parser->error(true,
                                              {ParseError::Expected, op_token2,
                                               ExpectedToken::CloseParen, op_token});
                                push_back_token(parser, op_token2);
                            }
                        } else if (op_token.type == Token::OpenSquare) {
                            Token op_token2 = read_token(parser);
                            if (op_token2.type == Token::CloseSquare) {
                                op_func->punc2 = op_token2;
                            } else {
                                parser->error(true,
                                              {ParseError::Expected, op_token2,
                                               ExpectedToken::CloseSquare, op_token});
                                push_back_token(parser, op_token2);
                            }
                        }
                        break;
                    }

                    default: {
                        // Expected operator token
                        parser->error(true, {ParseError::Expected, op_token,
                                             ExpectedToken::OperatorToken, token});
                        parser->mute_errors = true;
                        push_back_token(parser, op_token);
                        break;
                    };
                }
            } else {
                push_back_token(parser, token);
            }
        } else {
            push_back_token(parser, token);
        }
    }
    if (((mode == ParseQualifiedMode::RequireComplete) && !qid.is_complete()) ||
        ((mode == ParseQualifiedMode::RequireCompleteOrEmpty) &&
         qid.is_nested_name_only())) {
        // FIXME: Improve these error messages
        Token token = read_token(parser);
        parser->error(true, {ParseError::Expected, token, ExpectedToken::QualifiedID});
        push_back_token(parser, token);
    }
    return qid;
}

} // namespace cpp
} // namespace ply
