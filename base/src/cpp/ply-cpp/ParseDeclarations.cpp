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

Token parse_required_semicolon(Parser* parser) {
    Token semi_token = read_token(parser);
    if (semi_token.type == Token::Semicolon) {
        parser->stop_muting_errors();
        return semi_token;
    } else {
        // expected ;
        parser->error(true,
                      {ParseError::Expected, semi_token, ExpectedToken::Semicolon});
        push_back_token(parser, semi_token);
        return {};
    }
}

bool is_type_declaration(const grammar::Declaration::Simple& simple) {
    for (const grammar::DeclSpecifier* decl_spec : simple.decl_specifier_seq) {
        switch (decl_spec->id) {
            case grammar::DeclSpecifier::ID::Record:
            case grammar::DeclSpecifier::ID::Enum_:
                return true;
            default:
                break;
        }
    }
    return false;
}

void parse_enum_body(Parser* parser, grammar::DeclSpecifier::Enum_* en) {
    parser->stop_muting_errors();
    SetAcceptFlagsInScope accept_scope{parser, Token::OpenCurly};

    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::CloseCurly) {
            // Done
            parser->stop_muting_errors();
            en->close_curly = token;
            break;
        } else if (token.type == Token::Identifier) {
            parser->stop_muting_errors();

            // Create enumerator
            grammar::InitEnumeratorWithComma* init_enor =
                en->enumerators.append(new grammar::InitEnumeratorWithComma);
            init_enor->identifier = token;
            parse_optional_variable_initializer(parser, init_enor->init, false);
            Token token2 = read_token(parser);
            bool done = false;
            if (token2.type == Token::Comma) {
                parser->stop_muting_errors();
                init_enor->comma = token2;
            } else if (token2.type == Token::CloseCurly) {
                // Done
                parser->stop_muting_errors();
                en->close_curly = token2;
                done = true;
            } else {
                // expected , or } after enum member
                if (token2.type == Token::Identifier) {
                    parser->error(
                        true,
                        {ParseError::MissingCommaAfterEnumerator, token2, {}, token});
                }
                // Other tokens will generate an error on next loop iteration
                push_back_token(parser, token2);
            }
            parser->visor->on_got_enumerator(init_enor);
            if (done)
                break;
        } else {
            // expected enumerator or }
            parser->error(true, {ParseError::Expected, token,
                                 ExpectedToken::EnumeratorOrCloseCurly});
            if (!handle_unexpected_token(parser, nullptr, token))
                return;
        }
    }
}

void parse_simple_declaration(Parser* parser, StringView enclosing_class_name) {
    Token start_loc = read_token(parser);
    push_back_token(parser, start_loc);
    ParseActivity pa{parser};
    grammar::Declaration::Simple simple;
    parse_specifiers_and_declarators(
        parser, simple, {SpecDcorMode::GlobalOrMember, enclosing_class_name});
    if (!pa.error_occurred()) {
        parser->stop_muting_errors();
        if (simple.init_declarators.is_empty() && !is_type_declaration(simple)) {
            // FIXME: It feels like this should be a higher-level error (or just a
            // warning actually!), since it's more of a semantic than a syntactic error.
            // Similarly, it should not be possible to define a class inside a function
            // parameter!
            parser->error(false, {ParseError::MissingDeclaration, start_loc});
        }
    }

    bool requires_trailing_semicolon = false;
    if (simple.init_declarators.num_items() > 0) {
        const grammar::InitDeclaratorWithComma& init_dcor =
            simple.init_declarators.back();
        requires_trailing_semicolon = !init_dcor.init.function_body();
    }

    if (requires_trailing_semicolon) {
        simple.semicolon = parse_required_semicolon(parser);
    }

    parser->visor->got_declaration(std::move(simple));
}

// Returns false if no input was read.
bool parse_declaration(Parser* parser, StringView enclosing_class_name) {
    Token token = read_token(parser);

    PLY_SET_IN_SCOPE(parser->at_declaration_scope, false);
    if (token.type == Token::Identifier) {
        if (token.identifier == "extern") {
            // Possible linkage specification
            parser->stop_muting_errors();
            push_back_token(parser, token);
            RestorePoint rp{parser};
            token = read_token(parser);
            grammar::Declaration::Linkage linkage;
            linkage.extern_ = token;

            token = read_token(parser);
            if (token.type != Token::StringLiteral) {
                rp.backtrack();
                parse_simple_declaration(parser, enclosing_class_name);
            } else {
                linkage.literal = token;

                token = read_token(parser);
                if (token.type == Token::OpenCurly) {
                    // It's a linkage specification block, such as
                    //      extern "C" {
                    //          ...
                    //      }
                    rp.cancel();
                    linkage.open_curly = token;
                    parser->visor->do_enter(AnyObject::bind(&linkage));
                    parse_declaration_list(parser, &linkage.close_curly, {});
                    parser->visor->do_exit(AnyObject::bind(&linkage));
                    parser->visor->got_declaration(std::move(linkage));
                } else {
                    // It's a linkage specifier for the current declaration, such as
                    //      extern "C" void foo();
                    //      ^^^^^^^^^^
                    // FIXME: Make Declaration type for this
                    rp.backtrack();
                    parse_simple_declaration(parser, enclosing_class_name);
                }
            }
        } else if (token.identifier == "public" || token.identifier == "private" ||
                   token.identifier == "protected") {
            // Access specifier
            parser->stop_muting_errors();
            Token punc_token = read_token(parser);
            if (punc_token.type == Token::SingleColon) {
                parser->visor->got_declaration(
                    grammar::Declaration::AccessSpecifier{token, punc_token});
            } else {
                // expected :
                parser->error(true, {ParseError::Expected, punc_token,
                                     ExpectedToken::Colon, token});
                push_back_token(parser, punc_token);
            }
        } else if (token.identifier == "static_assert") {
            // static_assert
            parser->stop_muting_errors();
            Token punc_token = read_token(parser);
            if (punc_token.type != Token::OpenParen) {
                // expected (
                parser->error(true, {ParseError::Expected, punc_token,
                                     ExpectedToken::OpenParen, token});
                push_back_token(parser, punc_token);
            } else {
                Token close_token;
                bool continue_normally =
                    skip_any_scope(parser, &close_token, punc_token);
                if (continue_normally) {
                    grammar::Declaration::StaticAssert sa;
                    sa.keyword = token;
                    sa.arg_list.open_paren = punc_token;
                    sa.arg_list.close_paren = close_token;
                    sa.semicolon = parse_required_semicolon(parser);
                    parser->visor->got_declaration(grammar::Declaration{std::move(sa)});
                }
            }
        } else if (token.identifier == "namespace") {
            // namespace
            parser->stop_muting_errors();
            grammar::Declaration::Namespace_ ns;
            ns.keyword = token;

            Token token = read_token(parser);
            if (token.type == Token::Identifier) {
                // FIXME: Ensure it's not a reserved word
                push_back_token(parser, token);
                ns.qid =
                    parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
                token = read_token(parser);
            }

            if (token.type == Token::OpenCurly) {
                ns.open_curly = token;
                parser->visor->do_enter(AnyObject::bind(&ns));
                parse_declaration_list(parser, &ns.close_curly, {});
                parser->visor->do_exit(AnyObject::bind(&ns));
            } else {
                // expected {
                parser->error(true,
                              {ParseError::Expected, token, ExpectedToken::OpenCurly});
                push_back_token(parser, token);
            }
            parser->visor->got_declaration(std::move(ns));
        } else if (token.identifier == "template") {
            // template
            parser->stop_muting_errors();
            grammar::Declaration::Template_ tmpl;
            tmpl.keyword = token;
            Token token2 = read_token(parser);
            if (token2.type == Token::OpenAngle) {
                push_back_token(parser, token2);
                {
                    PLY_SET_IN_SCOPE(parser->pp->tokenize_close_angles_only, true);
                    parse_parameter_declaration_list(parser, tmpl.params, true);
                }
            } else {
                push_back_token(parser, token2);
            }
            parser->visor->do_enter(AnyObject::bind(&tmpl));
            parse_declaration(parser, enclosing_class_name);
            parser->visor->do_exit(AnyObject::bind(&tmpl));
            parser->visor->got_declaration(grammar::Declaration{std::move(tmpl)});
        } else if (token.identifier == "using") {
            // using directive or type alias
            parser->stop_muting_errors();
            Token token2 = read_token(parser);
            if (token2.type == Token::Identifier && token2.identifier == "namespace") {
                grammar::Declaration::UsingDirective using_dir;
                using_dir.using_ = token;
                using_dir.namespace_ = token2;

                using_dir.qid =
                    parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
                using_dir.semicolon = parse_required_semicolon(parser);

                parser->visor->got_declaration(
                    grammar::Declaration{std::move(using_dir)});
            } else {
                grammar::Declaration::Alias alias;
                alias.using_ = token;
                alias.name = token2;

                Token equal_token = read_token(parser);
                if (equal_token.type != Token::SingleEqual) {
                    // expected =
                    parser->error(true, {ParseError::Expected, equal_token,
                                         ExpectedToken::Equal, token2});
                    push_back_token(parser, equal_token);
                } else {
                    alias.equals = equal_token;

                    grammar::Declaration::Simple simple;
                    parse_specifiers_and_declarators(parser, simple,
                                                     SpecDcorMode::TypeID);
                    alias.decl_specifier_seq = std::move(simple.decl_specifier_seq);
                    PLY_ASSERT(simple.init_declarators.num_items() ==
                               1); // because SpecDcorMode::TypeID
                    alias.dcor = std::move(simple.init_declarators[0].dcor);
                    alias.semicolon = parse_required_semicolon(parser);

                    parser->visor->got_declaration(
                        grammar::Declaration{std::move(alias)});
                }
            }
        } else {
            push_back_token(parser, token);
            parse_simple_declaration(parser, enclosing_class_name);
        }
    } else if (token.type == Token::Semicolon) {
        grammar::Declaration::Empty empty;
        empty.semicolon = token;
        parser->visor->got_declaration(grammar::Declaration{std::move(empty)});
    } else if (token.type == Token::Tilde) {
        push_back_token(parser, token);
        parse_simple_declaration(parser, enclosing_class_name);
    } else {
        // Can't handle this token.
        push_back_token(parser, token);
        return false;
    }
    return true;
}

void parse_declaration_list(Parser* parser, Token* out_close_curly,
                            StringView enclosing_class_name) {
    // Always handle close curly at this scope, even if it's file scope:
    SetAcceptFlagsInScope accept_scope{parser, Token::OpenCurly};
    PLY_SET_IN_SCOPE(parser->at_declaration_scope, true);

    for (;;) {
        Token token = read_token(parser);
        if (token.type == (out_close_curly ? Token::CloseCurly : Token::EndOfFile)) {
            if (out_close_curly) {
                *out_close_curly = token;
            }
            break;
        }
        push_back_token(parser, token);
        if (!parse_declaration(parser, enclosing_class_name)) {
            token = read_token(parser);
            parser->error(true,
                          {ParseError::Expected, token, ExpectedToken::Declaration});
            if (token.type !=
                Token::CloseCurly) { // consume close curlies after logging the error
                if (!handle_unexpected_token(parser, nullptr, token)) {
                    PLY_ASSERT(token.type == Token::EndOfFile); // can only be EOF here
                    break;
                }
            }
        }
    }
}

grammar::TranslationUnit parse_translation_unit(Parser* parser) {
    grammar::TranslationUnit tu;
    parser->visor->do_enter(AnyObject::bind(&tu));
    parse_declaration_list(parser, nullptr, {});
    Token eof_tok = read_token(parser);
    PLY_ASSERT(eof_tok.type == Token::EndOfFile); // EOF is the only possible token here
    PLY_UNUSED(eof_tok);
    parser->visor->do_exit(AnyObject::bind(&tu));
    return tu;
}

} // namespace cpp
} // namespace ply
