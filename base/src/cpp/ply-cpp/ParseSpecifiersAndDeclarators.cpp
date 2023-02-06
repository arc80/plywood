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

struct DeclaratorFlags {
    static const u32 AllowNamed = 1;
    static const u32 AllowAbstract = 2;
};

// Consumes as much as it can; unrecognized tokens are returned to caller without
// logging an error:
PLY_NO_INLINE void parse_conversion_type_id2(Parser* parser, grammar::Declarator& dcor,
                                             grammar::DeclaratorProduction* nested) {
    dcor.prod = nested;
    bool allow_qualifier = false;

    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::Star || token.type == Token::SingleAmpersand ||
            token.type == Token::DoubleAmpersand) {
            auto* prod = new grammar::DeclaratorProduction;
            auto ptr_to = prod->type.pointer_to().switch_to();
            ptr_to->punc = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
            allow_qualifier = (token.type == Token::Star);
        } else if (token.type == Token::Ellipsis) {
            // FIXME: Make a Production rule for this
        } else if (token.type == Token::Identifier) {
            if (token.identifier == "const" || token.identifier == "volatile") {
                if (!allow_qualifier) {
                    // Qualifier not allowed here (eg. immediately after comma in
                    // declarator list). This is not a breaking error; just ignore it
                    // and continue from here.
                    parser->error(false, {ParseError::QualifierNotAllowedHere, token});
                }

                auto* prod = new grammar::DeclaratorProduction;
                auto qualifier = prod->type.qualifier().switch_to();
                qualifier->keyword = token;
                prod->target = std::move(dcor.prod);
                dcor.prod = prod;
            } else {
                push_back_token(parser, token);
                break;
            }
        } else {
            // Unrecognized tokens are returned to caller without raising an error here.
            push_back_token(parser, token);
            break;
        }
    }
}

// Consumes as much as it can; unrecognized tokens are returned to caller without
// logging an error:
PLY_NO_INLINE void
parse_conversion_type_id(Parser* parser, grammar::UnqualifiedID::ConversionFunc* conv) {
    // Note: This has some similarities to parse_specifiers_and_declarators (with an
    // optional abstract declarator using & * &&), but for now, merging them appears too
    // complex, especially since this function does not accept () or []. (OR...? We
    // could simply set a flag to prevent those functions from consuming '(' or ']'.)
    bool got_type_specifier = false;
    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "const" || token.identifier == "volatile") {
                conv->decl_specifier_seq.append(
                    new grammar::DeclSpecifier{grammar::DeclSpecifier::Keyword{token}});
            } else {
                push_back_token(parser, token);
                grammar::QualifiedID qid =
                    parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
                if (got_type_specifier) {
                    // We already got a type specifier.
                    // This is not a breaking error; just ignore it and continue from
                    // here.
                    parser->error(false, {ParseError::TooManyTypeSpecifiers,
                                          qid.get_first_token()});
                } else {
                    got_type_specifier = true;
                    PLY_ASSERT(!qid.is_empty()); // Shouldn't happen because token was
                                                 // an identifier
                    conv->decl_specifier_seq.append(new grammar::DeclSpecifier{
                        grammar::DeclSpecifier::TypeID{{}, std::move(qid)}});
                }
            }
        } else {
            // This must be the optional (limited) abstract declarator part:
            push_back_token(parser, token);
            grammar::Declarator dcor;
            parse_conversion_type_id2(parser, dcor, nullptr);
            PLY_ASSERT(dcor.qid.is_empty());
            conv->abstract_dcor = std::move(dcor.prod);
            break;
        }
    }
}

PLY_NO_INLINE bool close_scope(Parser* parser, Token* out_close_token,
                               const Token& open_token) {
    Token close_token = read_token(parser);
    if (close_token.type == open_token.type + 1) {
        *out_close_token = close_token;
    } else {
        ExpectedToken exp = ExpectedToken::CloseParen;
        if (open_token.type == Token::OpenSquare) {
            exp = ExpectedToken::CloseSquare;
        } else {
            PLY_ASSERT(open_token.type == Token::OpenParen);
        }
        parser->error(true, {ParseError::Expected, close_token, exp});
        push_back_token(parser, close_token);
        // Consume tokens up to the closing )
        if (!skip_any_scope(parser, nullptr, open_token)) {
            // We didn't get a closing ), but an outer scope will handle it
            PLY_ASSERT(parser->mute_errors);
            return false;
        }
        // Got closing )
        parser->stop_muting_errors();
    }
    return true;
}

void parse_optional_trailing_return_type(Parser* parser,
                                         grammar::DeclaratorProduction* fn_prod) {
    PLY_ASSERT(fn_prod);
    auto function = fn_prod->type.function();
    PLY_ASSERT(function);

    Token arrow_token = read_token(parser);
    if (arrow_token.type == Token::Arrow) {
        function->arrow = arrow_token;
        Token qid_start_token = read_token(parser);
        push_back_token(parser, qid_start_token);
        // FIXME: Should parse a TypeID here, not just a qualified ID:
        function->trailing_ret_type =
            parse_qualified_id(parser, ParseQualifiedMode::AllowIncomplete);
        if (function->trailing_ret_type.is_empty()) {
            parser->error(true, {ParseError::Expected, qid_start_token,
                                 ExpectedToken::TrailingReturnType, function->arrow});
        }
    } else {
        push_back_token(parser, arrow_token);
    }
}

//-------------------------------------------------------------------------------------
// parse_declarator
//
// When bad tokens are encountered, it consumes them until it encounters a token that an
// outer scope is expected to handle, as determined by parser->outer_accept_flags. In
// that case, it returns early. If the bad token is one of { ( or [, it calls
// skip_any_scope().
//
// The first bad token sets parser->mute_errors to true. mute_errors remains true until
// it reaches the next good token. mute_errors may remain true when we return; this can
// happen, for example, when } is encountered, causing us to return early.
//-------------------------------------------------------------------------------------
void parse_declarator(Parser* parser, grammar::Declarator& dcor,
                      grammar::DeclaratorProduction* nested, u32 dcor_flags) {
    dcor.prod = nested;
    bool allow_qualifier = false;
    Owned<grammar::DeclaratorProduction>* prod_to_modify = nullptr; // Used in phase two
    bool expecting_qualified_id = false;

    // This is the first phase of parsing a declarator. It handles everything up to
    // trailing function parameter lists and array subscripts.
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
    // If an open parenthesis is encountered during this phase, and the AllowAbstract
    // flags is set, it first tries to parse a function parameter list; otherwise, or if
    // that fails, it tries to parse a nested declarator. If it's a nested declarator,
    // nested DeclarationProductions are inserted at the head of the current
    // DeclarationProduction chain. In either case, no further
    // pointer/reference/cv-qualifiers are expected after the closing parenthesis, so we
    // break out of the loop and proceed to the second phase.
    //
    // When a qualified ID is encountered, it's considered the name of the declarator
    // (in other words, the declarator is not abstract), and we break out of the loop
    // and proceed to the second phase.

    for (;;) {
        // Try to tokenize a qualified ID.
        grammar::QualifiedID qid =
            parse_qualified_id(parser, ParseQualifiedMode::AllowIncomplete);
        if (!qid.unqual.empty()) {
            dcor.qid = std::move(qid);
            if ((dcor_flags & DeclaratorFlags::AllowNamed) == 0) {
                // Qualified ID is not allowed here
                // FIXME: Should rewind instead of consuming the qualified-id????
                // The caller may log a more informative error at this token! (check
                // test suite)
                parser->error(
                    false, {ParseError::TypeIDCannotHaveName, qid.get_first_token()});
                // Don't mute errors
            }
            break; // Got qualified-id
        }
        // qid.unqual is empty, but qid.nested_name might be a pointer prefix (as in a
        // pointer-to-member).

        Token token = read_token(parser);
        if (token.type == Token::OpenParen) {
            if (!qid.nested_name.is_empty()) {
                // Should not be preceded by nested name specifier
                parser->error(false, {ParseError::NestedNameNotAllowedHere,
                                      token,
                                      {},
                                      qid.get_first_token()});
                // Don't mute errors
            }

            parser->stop_muting_errors();

            if ((dcor_flags & DeclaratorFlags::AllowAbstract) != 0) {
                // If abstract declarators are allowed, try to parse a function
                // parameter list first.
                push_back_token(parser, token);
                RestorePoint rp{parser};
                // FIXME: When a restore point is active, handle_unexpected_token()
                // should always return false. Otherwise, parse_parameter_list could end
                // up consuming way too many tokens, and it might even incorrectly
                // "pre-tokenize" '>>' as a right-shift operator instead of as two
                // CloseAngles...
                grammar::DeclaratorProduction* saved_prod = dcor.prod;
                prod_to_modify = &dcor.prod;
                grammar::DeclaratorProduction* fn_prod =
                    parse_parameter_list(parser, &prod_to_modify);
                if (!rp.error_occurred()) {
                    // Success. Parse optional trailing return type. If any parse errors
                    // occur while doing so, we won't backtrack.
                    PLY_ASSERT(fn_prod);
                    rp.cancel();
                    parse_optional_trailing_return_type(parser, fn_prod);
                    // Break out of the loop and continue with the second phase.
                    break;
                }

                // It didn't parse as a function parameter list.
                // Roll back any productions that were created:
                while (dcor.prod != saved_prod) {
                    PLY_ASSERT(dcor.prod);
                    grammar::DeclaratorProduction* child = dcor.prod->target.release();
                    dcor.prod = child;
                }
                rp.backtrack();
                rp.cancel();
                token = read_token(parser);
                prod_to_modify = nullptr;
            }

            // Parse it as a nested declarator.
            grammar::Declarator target;
            parse_declarator(parser, target, dcor.prod.release(), dcor_flags);
            dcor.prod = new grammar::DeclaratorProduction;
            auto parenthesized = dcor.prod->type.parenthesized().switch_to();
            parenthesized->open_paren = token;
            dcor.prod->target = std::move(target.prod);
            PLY_ASSERT(dcor.qid.is_empty());
            dcor.qid = std::move(target.qid);

            if (!close_scope(parser, &parenthesized->close_paren, token))
                return;
            break;
        }

        if (!qid.nested_name.is_empty()) {
            if (token.type != Token::Star) {
                // Should not be preceded by nested name specifier
                parser->error(false, {ParseError::NestedNameNotAllowedHere,
                                      token,
                                      {},
                                      qid.get_first_token()});
            }
        }

        if (token.type == Token::Star || token.type == Token::SingleAmpersand ||
            token.type == Token::DoubleAmpersand) {
            parser->stop_muting_errors();

            auto* prod = new grammar::DeclaratorProduction;
            auto ptr_to = prod->type.pointer_to().switch_to();
            ptr_to->nested_name = std::move(qid.nested_name);
            ptr_to->punc = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
            allow_qualifier = (token.type == Token::Star);
        } else if (token.type == Token::Ellipsis) {
            // FIXME: Make a Production rule for this

            parser->stop_muting_errors();
        } else if (token.type == Token::Identifier) {
            PLY_ASSERT(qid.nested_name.is_empty());
            PLY_ASSERT(token.identifier == "const" || token.identifier == "volatile" ||
                       token.identifier == "inline" || token.identifier == "static" ||
                       token.identifier == "friend");
            if (!allow_qualifier) {
                // Qualifier not allowed here
                parser->error(false, {ParseError::QualifierNotAllowedHere, token});
                // Handle it anyway...
            }

            parser->stop_muting_errors();

            auto* prod = new grammar::DeclaratorProduction;
            auto qualifier = prod->type.qualifier().switch_to();
            qualifier->keyword = token;
            prod->target = std::move(dcor.prod);
            dcor.prod = prod;
        } else {
            // End of first phase of parsing a declarator.
            PLY_ASSERT(qid.nested_name.is_empty());
            if ((dcor_flags & DeclaratorFlags::AllowAbstract) == 0) {
                // Note that we still allow "empty" declarators (in other words,
                // abstract declarators with no DeclaratorProductions) even when
                // AllowAbstract is not specified, so that class definitions like:
                //      struct Foo {};
                // do not log an error.
                //
                // With this in mind, if a declarator name was required but
                // none was given, log an error *only if* some DeclaratorProductions
                // have been created.
                //
                // FIXME: Log an error (or warning?) if it's an empty declarators that
                // *doesn't* define a new class/struct/union, such as:
                //      int;
                if (dcor.prod) {
                    parser->error(true, {ParseError::Expected, token,
                                         ExpectedToken::QualifiedID});
                } else {
                    // No DeclaratorProductions have been created yet. We'll log an
                    // error if any are created in the second phase.
                    expecting_qualified_id = true;
                }
            }
            push_back_token(parser, token);
            break;
        }
    }

    // This is the second phase of parsing a declarator. It parses only trailing
    // function parameter lists and array subscripts. A subchain of
    // DeclaratorProductions is built in the same order that these are encountered, so
    // that they're effectively read left-to-right. For example,
    //      []()
    // becomes "array of functions" in the subchain. This subchain is inserted at the
    // head of dcor.prod, the current DeclaratorProduction chain being built.
    //
    // Note that this phase can take place inside a nested declarator, which means that
    // the caller may continue inserting DeclaratorProductions at the head of the chain
    // after we return.
    //
    // FIXME: make sure this approach works correctly for things like (*x())()

    if (!prod_to_modify) {
        prod_to_modify = &dcor.prod;
    }
    for (;;) {
        Token token = read_token(parser);
        auto check_expecting_qualified_id = [&]() {
            parser->stop_muting_errors();
            if (expecting_qualified_id) {
                parser->error(
                    true, {ParseError::Expected, token, ExpectedToken::QualifiedID});
                expecting_qualified_id = false;
            }
        };

        if (token.type == Token::OpenSquare) {
            check_expecting_qualified_id();

            auto* prod = new grammar::DeclaratorProduction;
            auto array_of = prod->type.array_of().switch_to();
            array_of->open_square = token;
            prod->target = std::move(*prod_to_modify);
            *prod_to_modify = prod;
            prod_to_modify = &prod->target;

            parse_expression(parser, true);

            if (!close_scope(parser, &array_of->close_square, token))
                return;
        } else if (token.type == Token::OpenParen) {
            check_expecting_qualified_id();

            push_back_token(parser, token);
            grammar::DeclaratorProduction* fn_prod =
                parse_parameter_list(parser, &prod_to_modify);
            if (fn_prod) {
                parse_optional_trailing_return_type(parser, fn_prod);
            }
        } else {
            push_back_token(parser, token);
            break;
        }
    }
}

void skip_member_initializer_list(Parser* parser) {
    // Make sure that if { is encountered (even with unexpected placement), we return to
    // caller.
    PLY_SET_IN_SCOPE(parser->outer_accept_flags,
                     parser->outer_accept_flags | Parser::AcceptOpenCurly);
    // FIXME: Add a scope to declare that we are parsing a member initializer list, and
    // report this scope in any logged errors (?)

    for (;;) {
        grammar::QualifiedID qid =
            parse_qualified_id(parser, ParseQualifiedMode::AllowIncomplete);
        if (qid.is_complete()) {
            Token open_brace_token = read_token(parser);
            if (open_brace_token.type == Token::OpenParen) {
                skip_any_scope(parser, nullptr, open_brace_token);
            } else if (open_brace_token.type == Token::OpenCurly) {
                skip_any_scope(parser, nullptr, open_brace_token);
            } else {
                // expected ( or {
                // FIXME: should report that it was expected after qualified id
                parser->error(true, {ParseError::Expected, open_brace_token,
                                     ExpectedToken::OpenCurlyOrParen});
                push_back_token(parser, open_brace_token);
                continue;
            }

            Token next_token = read_token(parser);
            if (next_token.type == Token::OpenCurly) {
                // End of member initializer list.
                parser->stop_muting_errors();
                push_back_token(parser, next_token);
                break;
            } else if (next_token.type == Token::Comma) {
                parser->stop_muting_errors();
            } else {
                parser->error(
                    true,
                    {ParseError::ExpectedFunctionBodyAfterMemberInitList, next_token});
                push_back_token(parser, next_token);
                break;
            }
        } else {
            Token token = read_token(parser);
            parser->error(true,
                          {ParseError::Expected, token, ExpectedToken::BaseOrMember});
            if (qid.is_empty()) {
                if (!handle_unexpected_token(parser, nullptr, token))
                    break;
            } else {
                push_back_token(parser, token);
            }
        }
    }
}

PLY_NO_INLINE void
parse_optional_function_body(Parser* parser, grammar::Initializer& result,
                             const grammar::Declaration::Simple& simple) {
    result.none().switch_to();
    Token token = read_token(parser);
    if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switch_to();
        assign->equal_sign = token;
        parse_expression(parser); // FIXME: Fill in var_init
        return;
    }
    if (token.type == Token::SingleColon) {
        auto fn_body = result.function_body().switch_to();
        fn_body->colon = token;
        // FIXME: populate MemberInitializerWithComma
        skip_member_initializer_list(parser);
        token = read_token(parser);
    }
    if (token.type == Token::OpenCurly) {
        parser->visor->do_enter(AnyObject::bind(&simple));
        auto fn_body = result.function_body();
        if (!fn_body) {
            fn_body.switch_to();
        }
        fn_body->open_curly = token;
        skip_any_scope(parser, &fn_body->close_curly, token);
        parser->visor->do_exit(AnyObject::bind(&simple));
    } else {
        push_back_token(parser, token);
    }
}

void parse_optional_type_idinitializer(Parser* parser, grammar::Initializer& result) {
    result.none().switch_to();
    Token token = read_token(parser);
    if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switch_to();
        assign->equal_sign = token;
        token = read_token(parser);
        if (token.identifier == "0") {
            // FIXME: Support <typename A::B = 0> correctly!
            // It will require changes to parse_specifiers_and_declarators
        } else {
            push_back_token(parser, token);
            ParseActivity pa{parser};
            grammar::Declaration::Simple simple;
            parse_specifiers_and_declarators(parser, simple, SpecDcorMode::TypeID);
            if (!pa.error_occurred()) {
                auto type_id = assign->type.type_id().switch_to();
                type_id->decl_specifier_seq = std::move(simple.decl_specifier_seq);
                PLY_ASSERT(simple.init_declarators.num_items() == 1);
                PLY_ASSERT(simple.init_declarators[0].dcor.qid.is_empty());
                type_id->abstract_dcor =
                    std::move(simple.init_declarators[0].dcor.prod);
            }
        }
    } else {
        push_back_token(parser, token);
    }
}

PLY_NO_INLINE void parse_optional_variable_initializer(Parser* parser,
                                                       grammar::Initializer& result,
                                                       bool allow_braced_init) {
    result.none().switch_to();
    Token token = read_token(parser);
    if (token.type == Token::OpenCurly) {
        // It's a variable initializer
        result.assignment().switch_to();
        push_back_token(parser, token);
        parse_expression(parser); // FIXME: Fill in var_init
    } else if (token.type == Token::SingleEqual) {
        auto assign = result.assignment().switch_to();
        assign->equal_sign = token;
        Tuple<Token, Token> exp_pair = parse_expression(parser);
        assign->type.expression()->start = exp_pair.first;
        assign->type.expression()->end = exp_pair.second;
    } else if (token.type == Token::SingleColon) {
        auto bit_field = result.bit_field().switch_to();
        bit_field->colon = token;
        Tuple<Token, Token> exp_pair = parse_expression(parser);
        bit_field->expression_start = exp_pair.first;
        bit_field->expression_end = exp_pair.second;
    } else {
        push_back_token(parser, token);
    }
}

// Only called from parse_specifiers_and_declarators:
void parse_init_declarators(Parser* parser, grammar::Declaration::Simple& simple,
                            const SpecDcorMode& mode) {
    if (mode.mode == SpecDcorMode::GlobalOrMember) {
        // A list of zero or more named declarators is accepted here.
        for (;;) {
            grammar::Declarator dcor;
            parse_declarator(parser, dcor, nullptr, DeclaratorFlags::AllowNamed);
            if (dcor.qid.is_empty())
                break; // Error was already logged
            grammar::InitDeclaratorWithComma& init_dcor =
                simple.init_declarators.append();
            init_dcor.dcor = std::move(dcor);
            if (init_dcor.dcor.is_function()) {
                parse_optional_function_body(parser, init_dcor.init, simple);
                if (init_dcor.init.function_body()) {
                    if (simple.init_declarators.num_items() > 1) {
                        // Note: Mixing function definitions and declarations could be a
                        // higher-level error instead of a parse error.
                        // FIXME: A reference to both declarators should be part of the
                        // error message. For now, we'll just use the open parenthesis
                        // token.
                        parser->error(
                            false,
                            {ParseError::CantMixFunctionDefAndDecl,
                             init_dcor.dcor.prod->type.function()->params.open_punc});
                    }
                }
                // FIXME: Eventually it might be nice to *not* break early here, so that
                // we can look ahead to the next token and issue a special error message
                // if it's a comma (CantMixFunctionDefAndDecl, seen below). However, if
                // we do that now, comments at declaration scope will be skipped by
                // read_token(), and the ParseSupervisor won't get a complete list of
                // those comments. This could evenutally be fixed by implementing a
                // peek_token() / consume_token() API, so that peeking at the next token
                // won't skip over comments at declaration scope. [Update 2020-06-07:
                // at_declaration_scope is now a member of Parser. This opens the
                // possibility of a new approach: Just set parser->at_declaration_scope
                // = true while checking ahead for a comma.]
                break; // Stop parsing declarators immediately after the function body!
            } else {
                parse_optional_variable_initializer(parser, init_dcor.init, true);
            }
            Token sep_token = read_token(parser);
            if (sep_token.type == Token::Comma) {
                if (init_dcor.dcor.is_function()) {
                    // FIXME: It's not very clear from this error message that the comma
                    // is the token that triggered an error. In any case, we don't hit
                    // this codepath yet, as explained by the above comment.
                    PLY_ASSERT(0); // codepath never gets hit at the moment
                    parser->error(
                        false,
                        {ParseError::CantMixFunctionDefAndDecl,
                         init_dcor.dcor.prod->type.function()->params.open_punc});
                }
                init_dcor.comma = sep_token;
            } else {
                push_back_token(parser, sep_token);
                break;
            }
        }
    } else {
        // If it's a Param or TemplateParam, there should be one (possibly abstract)
        // declarator, possibly with initializer. If it's a TypeID or ConversionTypeID,
        // there should be one abstract declarator.
        grammar::InitDeclaratorWithComma& init_dcor = simple.init_declarators.append();
        u32 dcor_flags = DeclaratorFlags::AllowNamed | DeclaratorFlags::AllowAbstract;
        if (mode.is_any_type_id()) {
            dcor_flags = DeclaratorFlags::AllowAbstract;
        }
        parse_declarator(parser, init_dcor.dcor, nullptr, dcor_flags);
        if (mode.is_any_param()) {
            // Note: If this is a template parameter, it must be a non-type parameter.
            // Type parameters are parsed in parse_optional_type_idinitializer.
            parse_optional_variable_initializer(parser, init_dcor.init, false);
        }
    }
}

Array<grammar::BaseSpecifierWithComma> parse_base_specifier_list(Parser* parser) {
    Array<grammar::BaseSpecifierWithComma> base_specifier_list;
    for (;;) {
        grammar::BaseSpecifierWithComma base_spec;

        // Optional access specifier
        Token token = read_token(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "public" || token.identifier == "private" ||
                token.identifier == "protected") {
                parser->stop_muting_errors();
                base_spec.access_spec = token;
                token = read_token(parser);
            }
        }
        push_back_token(parser, token);

        // Qualified ID
        base_spec.base_qid =
            parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
        if (base_spec.base_qid.unqual.empty())
            break;
        parser->stop_muting_errors();
        grammar::BaseSpecifierWithComma& added_bs =
            base_specifier_list.append(std::move(base_spec));

        // Comma or {
        Token punc_token = read_token(parser);
        if (punc_token.type == Token::OpenCurly) {
            push_back_token(parser, punc_token);
            break;
        } else if (punc_token.type == Token::Comma) {
            added_bs.comma = token;
        } else {
            parser->error(true, {ParseError::Expected, punc_token,
                                 ExpectedToken::CommaOrOpenCurly});
            break;
        }
    }
    return base_specifier_list;
}

bool looks_like_ctor_dtor(StringView enclosing_class_name,
                          const grammar::QualifiedID& qid) {
    if (enclosing_class_name.is_empty()) {
        if (qid.nested_name.num_items() < 1)
            return false;
        StringView ctor_dtor_name = qid.unqual.get_ctor_dtor_name();
        if (ctor_dtor_name.is_empty())
            return false;
        const grammar::NestedNameComponent& tail = qid.nested_name.back();
        auto ident = tail.type.identifier_or_templated();
        if (!ident)
            return false;
        PLY_ASSERT(ident->name.is_valid());
        return ctor_dtor_name == ident->name.identifier;
    } else {
        if (qid.nested_name.num_items() > 0)
            return false;
        StringView ctor_dtor_name = qid.unqual.get_ctor_dtor_name();
        return ctor_dtor_name == enclosing_class_name;
    }
}

//-------------------------------------------------------------------------------------
// parse_specifiers_and_declarators
//-------------------------------------------------------------------------------------
void parse_specifiers_and_declarators(Parser* parser,
                                      grammar::Declaration::Simple& simple,
                                      const SpecDcorMode& mode) {
    // First, read decl specifier sequence.
    s32 type_specifier_index = -1;
    for (;;) {
        Token token = read_token(parser);
        if (token.type == Token::Identifier) {
            if (token.identifier == "extern") {
                parser->stop_muting_errors();
                Token literal = read_token(parser);
                if (literal.type == Token::StringLiteral) {
                    simple.decl_specifier_seq.append(new grammar::DeclSpecifier{
                        grammar::DeclSpecifier::LangLinkage{token, literal}});
                } else {
                    simple.decl_specifier_seq.append(new grammar::DeclSpecifier{
                        grammar::DeclSpecifier::Keyword{token}});
                    push_back_token(parser, literal);
                }
            } else if (token.identifier == "inline" || token.identifier == "const" ||
                       token.identifier == "volatile" || token.identifier == "static" ||
                       token.identifier == "friend" || token.identifier == "virtual" ||
                       token.identifier == "constexpr" ||
                       token.identifier == "thread_local" ||
                       token.identifier == "unsigned" ||
                       token.identifier == "mutable" ||
                       token.identifier == "explicit") {
                parser->stop_muting_errors();
                simple.decl_specifier_seq.append(
                    new grammar::DeclSpecifier{grammar::DeclSpecifier::Keyword{token}});
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       token.identifier == "alignas") {
                parser->stop_muting_errors();
                // FIXME: Implement DeclSpecifier::AlignAs
                // Note: alignas is technically part of the attribute-specifier-seq in
                // the grammar, which means it can only appear before the
                // decl-specifier-seq. But for now, let's just accept it here:
                Token open_paren = read_token(parser);
                if (open_paren.type != Token::OpenParen) {
                    parser->error(true, {ParseError::Expected, open_paren,
                                         ExpectedToken::OpenParen, token});
                    continue;
                }
                // FIXME: Accept integral constant expression here too
                grammar::Declaration::Simple simple;
                parse_specifiers_and_declarators(parser, simple, SpecDcorMode::TypeID);
                Token close_paren;
                if (!close_scope(parser, &close_paren, open_paren))
                    return;
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       token.identifier == "typedef") {
                parser->stop_muting_errors();
                // FIXME: Store this token in the parse tree
            } else if ((mode.mode != SpecDcorMode::TemplateParam) &&
                       (token.identifier == "struct" || token.identifier == "class" ||
                        token.identifier == "union")) {
                parser->stop_muting_errors();
                // FIXME: for TemplateParams, "class" should be treated like "typename".
                // Otherwise, it seems C++20 may actually support structs as non-type
                // template parameters, so we should revisit this eventually.
                if (type_specifier_index >= 0) {
                    // Already got type specifier
                    // Should this be a higher-level error?
                    // Note: We mute errors here, but we probably don't want to mute
                    // lower-level (syntactic) errors; we mainly just want to mute
                    // subsequent TooManyTypeSpecifiers for the same declaration. Could
                    // be another reason to log this error at some kind of higher level.
                    parser->error(true, {ParseError::TooManyTypeSpecifiers, token});
                }
                grammar::DeclSpecifier::Record record;
                record.class_key = token;

                record.qid = parse_qualified_id(
                    parser, ParseQualifiedMode::RequireCompleteOrEmpty);

                // Read optional virt-specifier sequence
                {
                    Token final_tok;
                    for (;;) {
                        token = read_token(parser);
                        if (token.identifier == "final") {
                            if (final_tok.is_valid()) {
                                parser->error(
                                    true, {ParseError::DuplicateVirtSpecifier, token});
                            } else {
                                final_tok = token;
                                record.virt_specifiers.append(token);
                            }
                        } else {
                            break;
                        }
                    }
                }

                if (token.type == Token::SingleColon) {
                    record.colon = token;
                    record.base_specifier_list = parse_base_specifier_list(parser);
                    token = read_token(parser);
                }

                if (token.type == Token::OpenCurly) {
                    record.open_curly = token;
                    parser->visor->do_enter(AnyObject::bind(&record));
                    parse_declaration_list(parser, &record.close_curly,
                                           record.qid.get_class_name());
                    parser->visor->do_exit(AnyObject::bind(&record));
                } else {
                    push_back_token(parser, token);
                }
                type_specifier_index = simple.decl_specifier_seq.num_items();
                simple.decl_specifier_seq.append(
                    new grammar::DeclSpecifier{std::move(record)});
            } else if ((mode.mode != SpecDcorMode::TemplateParam) &&
                       (token.identifier == "enum")) {
                parser->stop_muting_errors();
                if (type_specifier_index >= 0) {
                    // Already got type specifier
                    // Should this be a higher-level error?
                    // Note: We mute errors here, but we probably don't want to mute
                    // lower-level (syntactic) errors; we mainly just want to mute
                    // subsequent TooManyTypeSpecifiers for the same declaration. Could
                    // be another reason to log this error at some kind of higher level.
                    parser->error(true, {ParseError::TooManyTypeSpecifiers, token});
                }
                grammar::DeclSpecifier::Enum_ en;
                en.enum_key = token;
                Token token2 = read_token(parser);
                if ((token2.type == Token::Identifier) &&
                    (token2.identifier == "class")) {
                    en.class_key = token2;
                } else {
                    push_back_token(parser, token2);
                }

                en.qid = parse_qualified_id(parser,
                                            ParseQualifiedMode::RequireCompleteOrEmpty);

                Token sep_token = read_token(parser);
                if (sep_token.type == Token::SingleColon) {
                    if (en.qid.is_empty()) {
                        parser->error(false,
                                      {ParseError::ScopedEnumRequiresName, sep_token});
                    }
                    en.base_punc = sep_token;
                    en.base =
                        parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
                } else {
                    push_back_token(parser, sep_token);
                }

                Token token3 = read_token(parser);
                if (token3.type == Token::OpenCurly) {
                    en.open_curly = token3;
                    parser->visor->do_enter(AnyObject::bind(&en));
                    parse_enum_body(parser, &en);
                    parser->visor->do_exit(AnyObject::bind(&en));
                } else {
                    push_back_token(parser, token3);
                }

                type_specifier_index = simple.decl_specifier_seq.num_items();
                simple.decl_specifier_seq.append(
                    new grammar::DeclSpecifier{std::move(en)});
            } else if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                       (token.identifier == "operator") && (type_specifier_index < 0)) {
                parser->stop_muting_errors();
                // It's a conversion function
                grammar::InitDeclaratorWithComma& init_dcor =
                    simple.init_declarators.append();
                auto conv_func =
                    init_dcor.dcor.qid.unqual.conversion_func().switch_to();
                conv_func->keyword = token;
                parse_conversion_type_id(parser, conv_func.get());
                // Ensure there's an open parenthesis
                Token open_paren = read_token(parser);
                push_back_token(parser, open_paren);
                if (open_paren.type == Token::OpenParen) {
                    init_dcor.dcor.prod = new grammar::DeclaratorProduction;
                    auto func = init_dcor.dcor.prod->type.function().switch_to();
                    parse_parameter_declaration_list(parser, func->params, false);
                    func->qualifiers = parse_function_qualifier_seq(parser);
                    parse_optional_function_body(parser, init_dcor.init, simple);
                } else {
                    parser->error(true, {ParseError::Expected, open_paren,
                                         ExpectedToken::OpenParen});
                }
                return;
            } else {
                parser->stop_muting_errors();
                if (type_specifier_index >= 0) {
                    // We already got a type specifier, so this must be the declarator
                    // part.
                    push_back_token(parser, token);
                    parse_init_declarators(parser, simple, mode);
                    return;
                }

                Token typename_;
                grammar::QualifiedID qid;
                if (token.identifier == "typename") {
                    typename_ = token;
                    Token ellipsis;
                    Token t2 = read_token(parser);
                    if (t2.type == Token::Ellipsis) {
                        ellipsis = t2;
                    } else {
                        push_back_token(parser, t2);
                    }
                    qid = parse_qualified_id(
                        parser, ParseQualifiedMode::RequireCompleteOrEmpty);
                    if (mode.mode == SpecDcorMode::TemplateParam &&
                        simple.decl_specifier_seq.num_items() == 0 &&
                        qid.nested_name.num_items() == 0) {
                        // Parse it as a type parameter
                        grammar::DeclSpecifier* decl_spec =
                            simple.decl_specifier_seq.append(
                                new grammar::DeclSpecifier);
                        auto type_param = decl_spec->type_param().switch_to();
                        type_param->keyword = token;
                        type_param->ellipsis = ellipsis;
                        if (auto ident = qid.unqual.identifier()) {
                            type_param->identifier = ident->name;
                        } else if (!qid.unqual.empty()) {
                            parser->error(true, {ParseError::Expected,
                                                 qid.unqual.get_first_token(),
                                                 ExpectedToken::Identifier});
                            return;
                        }
                        grammar::InitDeclaratorWithComma& init_dcor =
                            simple.init_declarators.append();
                        init_dcor.dcor.qid = std::move(qid);
                        parse_optional_type_idinitializer(parser, init_dcor.init);
                        return;
                    }
                    if (ellipsis.is_valid()) {
                        parser->error(true, {ParseError::Expected, ellipsis,
                                             ExpectedToken::QualifiedID});
                    }
                } else {
                    push_back_token(parser, token);
                    qid =
                        parse_qualified_id(parser, ParseQualifiedMode::RequireComplete);
                    PLY_ASSERT(!qid.is_empty()); // Shouldn't happen because token was
                                                 // an Identifier
                }

                if ((mode.mode == SpecDcorMode::GlobalOrMember) &&
                    !typename_.is_valid() &&
                    looks_like_ctor_dtor(mode.enclosing_class_name, qid)) {
                    // Try (optimistically) to parse it as a constructor.
                    // We need a restore point in order to recover from Foo(bar())
                    RestorePoint rp{parser};
                    grammar::Declarator ctor_dcor;
                    Owned<grammar::DeclaratorProduction>* prod_to_modify =
                        &ctor_dcor.prod;
                    parse_parameter_list(parser, &prod_to_modify);
                    if (!rp.error_occurred()) {
                        // It's a constructor
                        PLY_ASSERT(ctor_dcor.prod && ctor_dcor.prod->type.function());
                        rp.cancel();
                        grammar::InitDeclaratorWithComma& init_dcor =
                            simple.init_declarators.append();
                        init_dcor.dcor = std::move(ctor_dcor);
                        PLY_ASSERT(init_dcor.dcor.qid.is_empty());
                        init_dcor.dcor.qid = std::move(qid);
                        parse_optional_function_body(parser, init_dcor.init, simple);
                        return;
                    }
                    // It failed to parse as a constructor. Treat this token as part of
                    // a simple type specifier instead.
                    rp.backtrack();
                }

                // In C++, all declarations must be explicitly typed; there is no
                // "default int". Therefore, this must be a simple type specifier.
                if (typename_.is_valid() && qid.nested_name.is_empty()) {
                    parser->error(true, {ParseError::Expected, qid.get_first_token(),
                                         ExpectedToken::NestedNamePrefix});
                }
                // FIXME: If mode == SpecDcorMode::Param, we should check at this point
                // that qid actually refers to a type (if possible!). Consider for
                // example (inside class 'Foo'):
                //      Foo(baz())
                // If 'baz' refers to a type, it's a constructor. Otherwise, it's a
                // function 'baz' returning Foo. If it's not possible to determine in
                // this pass, we obviously have to guess (leaning towards it being a
                // constructor), but the parse tree should store the fact that we
                // guessed somewhere.
                type_specifier_index = simple.decl_specifier_seq.num_items();
                grammar::DeclSpecifier* decl_spec =
                    simple.decl_specifier_seq.append(new grammar::DeclSpecifier);
                auto type_id = decl_spec->type_id().switch_to();
                type_id->typename_ = typename_;
                type_id->qid = std::move(qid);
                /*
                token = read_token(parser);
                if (token.type == Token::Ellipsis) {
                    // Template parameter packs are detected here
                    // FIXME: Improve the parser here!
                    PLY_ASSERT(mode.mode == SpecDcorMode::TemplateParam);
                } else {
                    push_back_token(parser, token);
                }
                */
            }
        } else {
            // Not an identifier.
            // This must be the declarator part. (eg. may start with * or &)
            // Do we have a type specifier yet?
            if (type_specifier_index < 0) {
                switch (mode.mode) {
                    case SpecDcorMode::GlobalOrMember:
                        // If parsing a global or member, don't log an error if no type
                        // specifier was encountered yet, because the declarator may
                        // name a destructor. Leave it to higher-level code to verify
                        // there are type specifiers when needed.
                        break;
                    case SpecDcorMode::Param:
                        parser->error(true, {ParseError::Expected, token,
                                             ExpectedToken::ParameterType});
                        break;
                    case SpecDcorMode::TemplateParam:
                        parser->error(true, {ParseError::Expected, token,
                                             ExpectedToken::TemplateParameterDecl});
                        break;
                    case SpecDcorMode::TypeID:
                    case SpecDcorMode::ConversionTypeID:
                        parser->error(true, {ParseError::Expected, token,
                                             ExpectedToken::TypeSpecifier});
                        break;
                    default:
                        PLY_ASSERT(0);
                        break;
                }
            }
            push_back_token(parser, token);
            parse_init_declarators(parser, simple, mode);
            return;
        }
    } // for loop
}

} // namespace cpp
} // namespace ply
