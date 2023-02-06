/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Parser.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace biscuit {

PLY_NO_INLINE bool error_at_token(Parser* parser, const ExpandedToken& error_token,
                                  ErrorTokenAction token_action, StringView message) {
    if (!parser->recovery.mute_errors) {
        parser->error(
            String::format("{} error: {}\n",
                           parser->tkr->file_location_map.format_file_location(
                               error_token.file_offset),
                           message));
        parser->error_count++;
    }

    parser->recovery.mute_errors = true;
    if (token_action == ErrorTokenAction::HandleUnexpected) {
        ExpandedToken close_token;
        return handle_unexpected_token(parser, &close_token, error_token);
    } else if (token_action == ErrorTokenAction::PushBack) {
        parser->tkr->rewind_to(error_token.token_idx);
    }
    return false;
}

constexpr u32 BinaryOpPrecedence[] = {
    0,  // Invalid
    3,  // Multiply
    3,  // Divide
    3,  // Modulo
    4,  // Add
    4,  // Subtract
    6,  // LessThan
    6,  // LessThanOrEqual
    6,  // GreaterThan
    6,  // GreaterThanOrEqual
    7,  // DoubleEqual
    11, // LogicalAnd
    12, // LogicalOr
    0,  // Count
};

PLY_STATIC_ASSERT(BinaryOpPrecedence[(u32) MethodTable::BinaryOp::Count] == 0);

constexpr u32 UnaryOpPrecedence[] = {
    0, // Invalid
    2, // Negate
    2, // LogicalNot
    2, // BitComplement
    0, // ContainerStart
};

PLY_STATIC_ASSERT(UnaryOpPrecedence[(u32) MethodTable::UnaryOp::ContainerStart] == 0);

Owned<Expression> parse_argument_list(Parser* parser, Owned<Expression>&& callable,
                                      u32 open_paren_token_idx) {
    PLY_SET_IN_SCOPE(parser->recovery.outer_accept_flags,
                     parser->recovery.outer_accept_flags |
                         Parser::RecoveryState::AcceptCloseParen);
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenize_new_line, false);
    auto call_expr = Owned<Expression>::create();
    call_expr->token_idx = open_paren_token_idx;
    auto call = call_expr->call().switch_to();
    call->callable = std::move(callable);

    // Parse argument list.
    ExpandedToken token = parser->tkr->read_token();
    if (token.type == TokenType::CloseParen)
        return call_expr; // Empty argument list.

    parser->tkr->rewind_to(token.token_idx);
    for (;;) {
        Owned<Expression> arg = parser->parse_expression();
        if (arg) {
            call->args.append(std::move(arg));
        }
        token = parser->tkr->read_token();
        if (token.type == TokenType::CloseParen) {
            // Got end of argument list
            parser->recovery.mute_errors = false;
            return call_expr;
        } else if (token.type == TokenType::Comma) {
            // Got comma
            parser->recovery.mute_errors = false;
        } else {
            if (!error_at_token(
                    parser, token, ErrorTokenAction::HandleUnexpected,
                    String::format(
                        "expected ',' or ')' after function argument; got {}",
                        token.desc())))
                return call_expr;
        }
    }
}

MethodTable::BinaryOp token_to_binary_op(TokenType token_type) {
    switch (token_type) {
        case TokenType::LessThan:
            return MethodTable::BinaryOp::LessThan;
        case TokenType::LessThanOrEqual:
            return MethodTable::BinaryOp::LessThanOrEqual;
        case TokenType::GreaterThan:
            return MethodTable::BinaryOp::GreaterThan;
        case TokenType::GreaterThanOrEqual:
            return MethodTable::BinaryOp::GreaterThanOrEqual;
        case TokenType::DoubleEqual:
            return MethodTable::BinaryOp::DoubleEqual;
        case TokenType::Plus:
            return MethodTable::BinaryOp::Add;
        case TokenType::Minus:
            return MethodTable::BinaryOp::Subtract;
        case TokenType::Asterisk:
            return MethodTable::BinaryOp::Multiply;
        case TokenType::Slash:
            return MethodTable::BinaryOp::Divide;
        case TokenType::Percent:
            return MethodTable::BinaryOp::Modulo;
        case TokenType::DoubleVerticalBar:
            return MethodTable::BinaryOp::LogicalOr;
        case TokenType::DoubleAmpersand:
            return MethodTable::BinaryOp::LogicalAnd;
        default:
            return MethodTable::BinaryOp::Invalid;
    }
}

MethodTable::UnaryOp token_to_unary_op(TokenType token_type) {
    switch (token_type) {
        case TokenType::Minus:
            return MethodTable::UnaryOp::Negate;
        case TokenType::Bang:
            return MethodTable::UnaryOp::LogicalNot;
        case TokenType::Tilde:
            return MethodTable::UnaryOp::BitComplement;
        default:
            return MethodTable::UnaryOp::Invalid;
    }
}

void parse_interpolated_string(Parser* parser, Expression* expr, bool is_multiline) {
    auto& pieces = expr->interpolated_string().switch_to()->pieces;

    PLY_SET_IN_SCOPE(parser->tkr->behavior.inside_string, true);
    PLY_SET_IN_SCOPE(parser->tkr->behavior.is_multiline_string, is_multiline);
    for (;;) {
        ExpandedToken token = parser->tkr->read_token();
        switch (token.type) {
            case TokenType::StringLiteral: {
                pieces.append(token.text);
                break;
            }
            case TokenType::BeginStringEmbed: {
                PLY_SET_IN_SCOPE(parser->recovery.outer_accept_flags,
                                 parser->recovery.outer_accept_flags |
                                     Parser::RecoveryState::AcceptCloseCurly);
                PLY_SET_IN_SCOPE(parser->tkr->behavior.inside_string, false);
                PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenize_new_line, false);
                if (pieces.is_empty() || pieces.back().embed) {
                    pieces.append();
                }
                pieces.back().embed = parser->parse_expression();
                ExpandedToken close_token = parser->tkr->read_token();
                if (close_token.type != TokenType::CloseCurly) {
                    error_at_token(
                        parser, close_token, ErrorTokenAction::HandleUnexpected,
                        String::format(
                            "expected '}' to close embedded expression at {}; got {}",
                            parser->tkr->file_location_map.format_file_location(
                                token.file_offset, false),
                            close_token.desc()));
                    skip_any_scope(parser, nullptr, TokenType::OpenCurly);
                }
                parser->recovery.mute_errors = false; // embed is now closed
                break;
            }
            case TokenType::EndString: {
                return;
            }
            default: {
                PLY_ASSERT(0); // Shouldn't get here
                break;
            }
        }
    }
}

Owned<Expression> Parser::parse_expression(u32 outer_precendence_level,
                                           bool as_statement) {
    Owned<Expression> expr;

    ExpandedToken token = this->tkr->read_token();
    if (token.type == TokenType::OpenParen) {
        PLY_SET_IN_SCOPE(this->recovery.outer_accept_flags,
                         this->recovery.outer_accept_flags |
                             Parser::RecoveryState::AcceptCloseParen);
        PLY_SET_IN_SCOPE(this->tkr->behavior.tokenize_new_line, false);
        expr = this->parse_expression();
        ExpandedToken closing_token = this->tkr->read_token();
        if (closing_token.type == TokenType::CloseParen) {
            this->recovery.mute_errors = false;
        } else {
            error_at_token(
                this, closing_token, ErrorTokenAction::PushBack,
                String::format("expected ')' to match the '(' at {}; got {}",
                               this->tkr->file_location_map.format_file_location(
                                   token.file_offset, false),
                               closing_token.desc()));
        }
    } else {
        expr = Owned<Expression>::create();
        expr->token_idx = token.token_idx;
        switch (token.type) {
            case TokenType::Identifier: {
                expr->name_lookup().switch_to()->name = token.label;
                break;
            }
            case TokenType::NumericLiteral: {
                expr->integer_literal().switch_to()->value = token.text.to<u32>();
                break;
            }
            case TokenType::BeginString: {
                parse_interpolated_string(this, expr, false);
                break;
            }
            case TokenType::BeginMultilineString: {
                parse_interpolated_string(this, expr, true);
                break;
            }
            default: {
                MethodTable::UnaryOp op = token_to_unary_op(token.type);
                if (op == MethodTable::UnaryOp::Invalid) {
                    this->tkr->rewind_to(token.token_idx);
                    if (!as_statement) { // Statement errors are issued by caller
                        error_at_token(this, token, ErrorTokenAction::DoNothing,
                                       String::format("expected an expression; got {}",
                                                      token.desc()));
                    }
                    return {};
                }
                auto unary_op = expr->unary_op().switch_to();
                unary_op->op = op;
                unary_op->expr = this->parse_expression();
                break;
            }
        }
    }
    this->recovery.mute_errors = false; // Got a valid expression

    // Try to extend the expression by consuming tokens to the right (eg. binary
    // operators and function call arguments).
    for (;;) {
        token = this->tkr->read_token();

        if (token.type == TokenType::OpenParen) {
            expr = parse_argument_list(this, std::move(expr), token.token_idx);
            continue;
        } else if (token.type == TokenType::Dot) {
            token = this->tkr->read_token();
            if (token.type != TokenType::Identifier) {
                error_at_token(this, token, ErrorTokenAction::PushBack,
                               String::format("expected identifier after '.'; got {}",
                                              token.desc()));
                continue;
            }
            auto prop_lookup_expr = Owned<Expression>::create();
            prop_lookup_expr->token_idx = token.token_idx;
            auto prop_lookup = prop_lookup_expr->property_lookup().switch_to();
            prop_lookup->obj = std::move(expr);
            prop_lookup->property_name = token.label;
            expr = std::move(prop_lookup_expr);
            continue;
        }

        MethodTable::BinaryOp op = token_to_binary_op(token.type);
        if (op != MethodTable::BinaryOp::Invalid) {
            u32 op_precedence = BinaryOpPrecedence[(u32) op];
            if (op_precedence >= outer_precendence_level) {
                this->tkr->rewind_to(token.token_idx);
                return expr;
            }

            Owned<Expression> rhs = this->parse_expression(op_precedence);
            if (!rhs)
                return expr; // an error occurred

            auto binary_op_expr = Owned<Expression>::create();
            binary_op_expr->token_idx = token.token_idx;
            auto binary_op = binary_op_expr->binary_op().switch_to();
            binary_op->op = op;
            binary_op->left = std::move(expr);
            binary_op->right = std::move(rhs);
            expr = std::move(binary_op_expr);
            continue;
        }

        // Can't extend this expression any further.
        this->tkr->rewind_to(token.token_idx);
        return expr;
    }
}

bool parse_parameter_list(Parser* parser, Statement::FunctionDefinition* function_def) {
    ExpandedToken token = parser->tkr->read_token();
    if (token.type != TokenType::OpenParen) {
        error_at_token(parser, token, ErrorTokenAction::PushBack,
                       String::format("expected '(' after function name '{}'; got {}",
                                      g_labelStorage.view(function_def->name),
                                      token.desc()));
        return false;
    }

    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenize_new_line, false);
    ExpandedToken param_token = parser->tkr->read_token();
    if (param_token.type == TokenType::CloseParen) {
        parser->recovery.mute_errors = false;
        return true;
    }

    PLY_SET_IN_SCOPE(parser->recovery.outer_accept_flags,
                     parser->recovery.outer_accept_flags |
                         Parser::RecoveryState::AcceptCloseParen);
    for (;; param_token = parser->tkr->read_token()) {
        if (param_token.type != TokenType::Identifier) {
            if (error_at_token(parser, param_token, ErrorTokenAction::HandleUnexpected,
                               String::format("expected function parameter; got {}",
                                              param_token.desc())))
                return false;
        }
        function_def->parameter_names.append(param_token.label);
        ExpandedToken token = parser->tkr->read_token();
        if (token.type == TokenType::CloseParen) {
            parser->recovery.mute_errors = false;
            return true;
        }
        if (token.type != TokenType::Comma) {
            if (error_at_token(
                    parser, token, ErrorTokenAction::HandleUnexpected,
                    String::format("expected ',' or ')' after parameter {}; got {}",
                                   param_token.desc(), token.desc())))
                return false;
        }
    }
}

void handle_function(Parser* parser, Owned<Statement>&& stmt, const ExpandedToken&) {
    if (!parse_parameter_list(parser, stmt->function_definition().get()))
        return;

    // Parse function body.
    biscuit::Parser::Filter filter;
    filter.keyword_handler = [](const KeywordParams&) {
        return KeywordResult::Illegal;
    };
    filter.allow_instructions = true;
    PLY_SET_IN_SCOPE(parser->filter, filter);
    stmt->function_definition()->body =
        parse_statement_block(parser, {"function", "parameter list"});
}

void parse_function_definition(Parser* parser, const ExpandedToken& fn_token,
                               StatementBlock* stmt_block) {
    auto stmt = Owned<Statement>::create();
    auto* function_def = stmt->function_definition().switch_to().get();
    function_def->tkr = parser->tkr;
    PLY_SET_IN_SCOPE(parser->outer_scope, stmt);

    // We got the 'fn' keyword.
    parser->recovery.mute_errors = false;

    // Parse function name.
    ExpandedToken name_token = parser->tkr->read_token();
    if (name_token.type != TokenType::Identifier) {
        error_at_token(parser, name_token, ErrorTokenAction::HandleUnexpected,
                       String::format("expected function name after 'fn'; got {}",
                                      name_token.desc()));
        return;
    }
    function_def->name = name_token.label;

    parser->function_handler(std::move(stmt), name_token);
}

void Parser::parse_statement(StatementBlock* stmt_block) {
    Label fn_key = g_labelStorage.insert("fn");
    Label if_key = g_labelStorage.insert("if");
    Label while_key = g_labelStorage.insert("while");
    Label else_key = g_labelStorage.insert("else");
    Label return_key = g_labelStorage.insert("return");

    ExpandedToken token = this->tkr->read_token();
    auto stmt = Owned<Statement>::create();
    stmt->token_idx = token.token_idx;

    // Look for keywords that begin a statement: fn, flow control and custom.
    AnyOwnedObject attributes;
    if (this->keywords.find(token.label)) {
        if ((token.label == fn_key) && (this->filter.allow_functions)) {
            parse_function_definition(this, token, stmt_block);
            return;
        } else if ((token.label == if_key) && (this->filter.allow_instructions)) {
            auto cond = stmt->if_().switch_to();
            cond->condition = this->parse_expression();
            cond->true_block =
                parse_statement_block(this, {"if-statement", "if-condition", true});
            PLY_SET_IN_SCOPE(this->tkr->behavior.tokenize_new_line, false);
            token = this->tkr->read_token();
            if (token.label == else_key) {
                cond->false_block =
                    parse_statement_block(this, {"else-block", "'else'", true});
            } else {
                this->tkr->rewind_to(token.token_idx);
            }
            stmt_block->statements.append(std::move(stmt));
            return;
        } else if ((token.label == while_key) && (this->filter.allow_instructions)) {
            auto cond = stmt->while_().switch_to();
            cond->condition = this->parse_expression();
            PLY_SET_IN_SCOPE(this->tkr->behavior.tokenize_new_line, false);
            cond->block = parse_statement_block(this, {"while-loop", "'while'", true});
            stmt_block->statements.append(std::move(stmt));
            return;
        } else if ((token.label == return_key) && (this->filter.allow_instructions)) {
            auto return_ = stmt->return_().switch_to();
            return_->expr = this->parse_expression();
            stmt_block->statements.append(std::move(stmt));
            return;
        }

        do {
            KeywordParams kp;
            kp.kw_token = token;
            kp.stmt_block = stmt_block;
            kp.attributes = &attributes;
            KeywordResult kr = this->filter.keyword_handler(kp);
            if (kr == KeywordResult::Illegal) {
                if (error_at_token(
                        this, token, ErrorTokenAction::HandleUnexpected,
                        String::format("keyword '{}' cannot be used here", token.text)))
                    return;
            } else if (kr == KeywordResult::Attribute) {
                // Get next token:
                token = this->tkr->read_token();
            } else if (kr == KeywordResult::Block) {
                token = this->tkr->read_token();
                if ((token.type == TokenType::NewLine) ||
                    (token.type == TokenType::Semicolon) ||
                    (token.type == TokenType::EndOfFile)) {
                    // This token marks the end of the statement.
                } else {
                    this->tkr->rewind_to(token.token_idx);
                    if (token.type != TokenType::CloseCurly) {
                        error_at_token(
                            this, token, ErrorTokenAction::DoNothing,
                            String::format(
                                "expected newline or ';' after block; got {}",
                                token.desc()));
                    }
                }
                return;
            } else {
                PLY_ASSERT(kr == KeywordResult::Error);
                return;
            }
        } while (this->keywords.find(token.label));
    }

    if (!this->filter.allow_instructions) {
        error_at_token(this, token, ErrorTokenAction::HandleUnexpected,
                       String::format("unexpected {}", token.desc()));
        return;
    }

    this->tkr->rewind_to(token.token_idx);

    // Try to parse an expression.
    Owned<Expression> expr = this->parse_expression(Limits<u32>::Max, true);
    token = this->tkr->read_token();
    if (expr) {
        StringView statement_type;
        if (token.type == TokenType::Equal) {
            statement_type = "assignment";
            auto assignment = stmt->assignment().switch_to();
            assignment->left = std::move(expr);
            assignment->right = this->parse_expression();
            assignment->attributes = std::move(attributes);
            token = this->tkr->read_token();
        } else {
            statement_type = "expression";
            auto evaluate = stmt->evaluate().switch_to();
            evaluate->expr = std::move(expr);
            evaluate->attributes = std::move(attributes);
        }

        // Check whether expression attributes are permitted for the given statement
        // type:
        this->filter.validate_attributes(stmt);

        // End of evaluate or assignment statement.
        if ((token.type == TokenType::NewLine) ||
            (token.type == TokenType::Semicolon)) {
            // This token marks the end of the statement.
        } else {
            this->tkr->rewind_to(token.token_idx);
            if (token.type != TokenType::CloseCurly) {
                error_at_token(this, token, ErrorTokenAction::DoNothing,
                               String::format("unexpected {} after {}", token.desc(),
                                              statement_type));
            }
        }
        stmt_block->statements.append(std::move(stmt));
        return;
    } else {
        if (attributes) {
            // FIXME: Report the exact attribute in this message
            if (error_at_token(
                    this, token, ErrorTokenAction::HandleUnexpected,
                    String::format("expected an expression after attribute; got {}",
                                   token.desc())))
                return;
        } else {
            if (error_at_token(this, token, ErrorTokenAction::HandleUnexpected,
                               String::format("unexpected {}", token.desc())))
                return;
        }
    }
}

Owned<StatementBlock> parse_statement_block_inner(Parser* parser,
                                                  const StatementBlockProperties& props,
                                                  bool file_scope) {
    auto block = Owned<StatementBlock>::create();

    for (;;) {
        ExpandedToken token = parser->tkr->read_token();
        switch (token.type) {
            case TokenType::NewLine:
            case TokenType::Semicolon: {
                break;
            }
            case TokenType::CloseCurly: {
                if (!file_scope)
                    return block;
                error_at_token(parser, token, ErrorTokenAction::HandleUnexpected,
                               "unexpected '}' at file scope");
                break;
            }
            case TokenType::EndOfFile: {
                if (!file_scope) {
                    error_at_token(parser, token, ErrorTokenAction::PushBack,
                                   String::format("unexpected end-of-file inside {}",
                                                  props.block_type));
                }
                return block;
            }
            default: {
                parser->tkr->rewind_to(token.token_idx);
                parser->parse_statement(block);
                break;
            }
        }
    }
}

Owned<StatementBlock> parse_statement_block(Parser* parser,
                                            const StatementBlockProperties& props) {
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenize_new_line, false);
    ExpandedToken token = parser->tkr->read_token();
    parser->tkr->behavior.tokenize_new_line = true;

    if (token.type == TokenType::OpenCurly) {
        return parse_statement_block_inner(parser, props);
    } else if (props.curly_braces_optional_if_control_flow) {
        parser->tkr->rewind_to(token.token_idx);
        auto block = Owned<StatementBlock>::create();
        parser->parse_statement(block);
        bool is_legal = false;
        if (block->statements.num_items() == 1) {
            switch (block->statements[0]->id) {
                case Statement::ID::Return_:
                case Statement::ID::If_:
                    is_legal = true;
                    break;
                default:
                    break;
            }
        }
        if (!is_legal) {
            error_at_token(
                parser, token, ErrorTokenAction::DoNothing,
                String::format(
                    "body of {} must be enclosed in curly braces unless it's a "
                    "break, continue or return statement",
                    props.block_type));
            parser->recovery.mute_errors = false;
        }
        return block;
    } else {
        error_at_token(parser, token, ErrorTokenAction::PushBack,
                       String::format("expected '{{' after {}; got {}",
                                      props.after_item_text, token.desc()));
        return {};
    }
}

Parser::Parser() {
    this->keywords.assign(g_labelStorage.insert("fn"), true);
    this->keywords.assign(g_labelStorage.insert("if"), true);
    this->keywords.assign(g_labelStorage.insert("while"), true);
    this->keywords.assign(g_labelStorage.insert("else"), true);
    this->keywords.assign(g_labelStorage.insert("return"), true);
    this->function_handler = {handle_function, this};
}

} // namespace biscuit
} // namespace ply
