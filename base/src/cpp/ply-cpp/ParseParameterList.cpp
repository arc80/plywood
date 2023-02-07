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

struct ParseParams {
    Token::Type open_punc = Token::OpenParen;
    Token::Type close_punc = Token::CloseParen;
    SpecDcorMode spec_dcor_mode = SpecDcorMode::Param;

    static ParseParams Func;
    static ParseParams Template;
};

ParseParams ParseParams::Func = {};
ParseParams ParseParams::Template = {
    Token::OpenAngle,
    Token::CloseAngle,
    SpecDcorMode::TemplateParam,
};

void parse_parameter_declaration_list(Parser* parser,
                                      grammar::ParamDeclarationList& params,
                                      bool for_template) {
    const ParseParams* pp = for_template ? &ParseParams::Template : &ParseParams::Func;

    // Get open punctuator
    // Caller is responsible for ensuring the expected punctuator is next!
    Token token = read_token(parser);
    // FIXME: Maybe we should log an error here instead of asserting. This could let us
    // remove some of the checks performed in some callers, potentially cleaning up
    // code.
    PLY_ASSERT(token.type == pp->open_punc); // Guaranteed by caller
    params.open_punc = token;
    parser->stop_muting_errors();

    token = read_token(parser);
    if (token.type == pp->close_punc) {
        // Empty parameter declaration list
        params.close_punc = token;
        return;
    } else {
        push_back_token(parser, token);
    }

    SetAcceptFlagsInScope accept_scope{parser, pp->open_punc};

    for (;;) {
        // A parameter declaration is expected here.
        grammar::ParamDeclarationWithComma* pdc = nullptr;
        bool any_tokens_consumed = false;

        Token expected_loc = read_token(parser);
        if (expected_loc.type == Token::Ellipsis && !for_template) {
            // FIXME: Check somewhere that this is the last parameter
            pdc = &params.params.append();
            grammar::DeclSpecifier* decl_spec = new grammar::DeclSpecifier;
            auto ellipsis = decl_spec->ellipsis().switch_to();
            ellipsis->ellipsis_token = expected_loc;
            pdc->decl_specifier_seq.append(decl_spec);
            any_tokens_consumed = true;
        } else if (for_template && expected_loc.identifier == "template") {
            // template template parameter
            // FIXME: Extend the grammar and store the result in params
            grammar::Declaration::Template_ tmpl;
            tmpl.keyword = expected_loc;
            Token token2 = read_token(parser);
            if (token2.type != Token::OpenAngle) {
                // expected <
                parser->error(true,
                              {ParseError::Expected, token2, ExpectedToken::OpenAngle});
                push_back_token(parser, token2);
                goto end_of_param;
            }
            push_back_token(parser, token2);
            ParseActivity pa{parser};
            {
                PLY_SET_IN_SCOPE(parser->pp->tokenize_close_angles_only, true);
                parse_parameter_declaration_list(parser, tmpl.params, true);
            }
            if (pa.error_occurred())
                goto end_of_param;
            Token class_token = read_token(parser);
            if (class_token.identifier != "class") { // C++17 also accepts 'typename'
                parser->error(true, {ParseError::Expected, class_token,
                                     ExpectedToken::OpenAngle});
                push_back_token(parser, class_token);
                goto end_of_param;
            }
            // read stuff after that
            grammar::Declaration::Simple simple;
            parse_specifiers_and_declarators(parser, simple, SpecDcorMode::TypeID);
            if (!pa.error_occurred()) {
                // We successfully parsed a template template parameter declaration.
                parser->stop_muting_errors();
            }
        } else {
            push_back_token(parser, expected_loc);
            grammar::Declaration::Simple simple;
            ParseActivity pa{parser};
            parse_specifiers_and_declarators(parser, simple, pp->spec_dcor_mode);
            if (!pa.error_occurred()) {
                // We successfully parsed a parameter declaration.
                parser->stop_muting_errors();
            }
            if (!simple.init_declarators.is_empty()) {
                // If there are no parse errors, parse_specifiers_and_declarators is
                // guaranteed to return exactly one init_declarator when spec_dcor_mode
                // == TemplateParam or Param, even if it's empty, as is the case for an
                // abstract declarator.
                PLY_ASSERT(simple.init_declarators.num_items() == 1);
                pdc = &params.params.append();
                pdc->decl_specifier_seq = std::move(simple.decl_specifier_seq);
                pdc->dcor = std::move(simple.init_declarators[0].dcor);
                pdc->init = std::move(simple.init_declarators[0].init);
            }
            any_tokens_consumed = pa.any_tokens_consumed();
        }
    end_of_param:;

        token = read_token(parser);
        if (token.type == pp->close_punc) {
            // End of parameter declaration list
            params.close_punc = token;
            break;
        } else if (token.type == Token::Comma) {
            // Comma
            if (pdc) {
                pdc->comma = token;
            }
        } else {
            // Unexpected token
            parser->error(true, {ParseError::Expected, token,
                                 for_template ? ExpectedToken::CommaOrCloseAngle
                                              : ExpectedToken::CommaOrCloseParen});
            if (any_tokens_consumed) {
                if (!handle_unexpected_token(parser, nullptr, token))
                    break;
            } else {
                if (!ok_to_stay_in_scope(parser, token))
                    break;
                push_back_token(parser, token);
            }
        }
    }
}

grammar::FunctionQualifierSeq parse_function_qualifier_seq(Parser* parser) {
    grammar::FunctionQualifierSeq qualifiers;

    // Read trailing qualifiers
    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::Identifier &&
            (token.identifier == "const" || token.identifier == "override")) {
            qualifiers.tokens.append(token);
        } else if (token.type == Token::SingleAmpersand ||
                   token.type == Token::DoubleAmpersand) {
            qualifiers.tokens.append(token);
        } else {
            push_back_token(parser, token);
            break;
        }
    }

    return qualifiers;
}

grammar::DeclaratorProduction*
parse_parameter_list(Parser* parser,
                     Owned<grammar::DeclaratorProduction>** prod_to_modify) {
    Token open_paren = read_token(parser);
    if (open_paren.type != Token::OpenParen) {
        // Currently, we only hit this case when optimistically trying to parse a
        // constructor
        PLY_ASSERT(parser->restore_point_enabled); // Just a sanity check
        parser->error(true,
                      {ParseError::Expected, open_paren, ExpectedToken::OpenParen});
        push_back_token(parser, open_paren);
        return nullptr;
    }

    parser->stop_muting_errors();

    auto* prod = new grammar::DeclaratorProduction;
    auto func = prod->type.function().switch_to();
    prod->target = std::move(**prod_to_modify);
    **prod_to_modify = prod;
    *prod_to_modify = &prod->target;

    push_back_token(parser, open_paren);
    parse_parameter_declaration_list(parser, func->params, false);
    func->qualifiers = parse_function_qualifier_seq(parser);
    return prod;
}

} // namespace cpp
} // namespace ply
