/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

PLY_NO_INLINE bool error(Parser* parser, const ExpandedToken& errorToken,
                         ErrorTokenAction tokenAction, StringView message) {
    if (!parser->recovery.muteErrors) {
        parser->errorOut->format(
            "{} error: {}\n",
            parser->tkr->fileLocationMap.formatFileLocation(errorToken.fileOffset), message);
        parser->errorCount++;
    }

    parser->recovery.muteErrors = true;
    if (tokenAction == ErrorTokenAction::HandleUnexpected) {
        ExpandedToken closeToken;
        return handleUnexpectedToken(parser, &closeToken, errorToken);
    } else if (tokenAction == ErrorTokenAction::PushBack) {
        parser->tkr->rewindTo(errorToken.tokenIdx);
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

Owned<Expression> parseArgumentList(Parser* parser, Owned<Expression>&& callable,
                                    u32 openParenTokenIdx) {
    PLY_SET_IN_SCOPE(parser->recovery.outerAcceptFlags,
                     parser->recovery.outerAcceptFlags | Parser::RecoveryState::AcceptCloseParen);
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);
    auto callExpr = Owned<Expression>::create();
    callExpr->tokenIdx = openParenTokenIdx;
    auto call = callExpr->call().switchTo();
    call->callable = std::move(callable);

    // Parse argument list.
    ExpandedToken token = parser->tkr->readToken();
    if (token.type == TokenType::CloseParen)
        return callExpr; // Empty argument list.

    parser->tkr->rewindTo(token.tokenIdx);
    for (;;) {
        Owned<Expression> arg = parser->parseExpression();
        if (arg) {
            call->args.append(std::move(arg));
        }
        token = parser->tkr->readToken();
        if (token.type == TokenType::CloseParen) {
            // Got end of argument list
            parser->recovery.muteErrors = false;
            return callExpr;
        } else if (token.type == TokenType::Comma) {
            // Got comma
            parser->recovery.muteErrors = false;
        } else {
            if (!error(parser, token, ErrorTokenAction::HandleUnexpected,
                       String::format("expected ',' or ')' after argument; got {}", token.desc())))
                return callExpr;
        }
    }
}

MethodTable::BinaryOp tokenToBinaryOp(TokenType tokenType) {
    switch (tokenType) {
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

MethodTable::UnaryOp tokenToUnaryOp(TokenType tokenType) {
    switch (tokenType) {
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

void parseInterpolatedString(Parser* parser, Expression* expr) {
    auto& pieces = expr->interpolatedString().switchTo()->pieces;

    PLY_SET_IN_SCOPE(parser->tkr->behavior.insideString, true);
    for (;;) {
        ExpandedToken token = parser->tkr->readToken();
        switch (token.type) {
            case TokenType::StringLiteral: {
                pieces.append(token.text);
                break;
            }
            case TokenType::BeginStringEmbed: {
                PLY_SET_IN_SCOPE(parser->recovery.outerAcceptFlags,
                                 parser->recovery.outerAcceptFlags |
                                     Parser::RecoveryState::AcceptCloseCurly);
                PLY_SET_IN_SCOPE(parser->tkr->behavior.insideString, false);
                PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);
                if (pieces.isEmpty() || pieces.back().embed) {
                    pieces.append();
                }
                pieces.back().embed = parser->parseExpression();
                ExpandedToken closeToken = parser->tkr->readToken();
                if (closeToken.type != TokenType::CloseCurly) {
                    error(parser, closeToken, ErrorTokenAction::HandleUnexpected,
                          String::format("expected '}' to close embedded expression at {}; got {}",
                                         parser->tkr->fileLocationMap.formatFileLocation(
                                             token.fileOffset, false),
                                         closeToken.desc()));
                    skipAnyScope(parser, nullptr, TokenType::OpenCurly);
                }
                parser->recovery.muteErrors = false; // embed is now closed
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

Owned<Expression> Parser::parseExpression(u32 outerPrecendenceLevel, bool asStatement) {
    Owned<Expression> expr;

    ExpandedToken token = this->tkr->readToken();
    if (token.type == TokenType::OpenParen) {
        PLY_SET_IN_SCOPE(this->recovery.outerAcceptFlags,
                         this->recovery.outerAcceptFlags | Parser::RecoveryState::AcceptCloseParen);
        PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, false);
        expr = this->parseExpression();
        ExpandedToken closingToken = this->tkr->readToken();
        if (closingToken.type == TokenType::CloseParen) {
            this->recovery.muteErrors = false;
        } else {
            error(this, closingToken, ErrorTokenAction::PushBack,
                  String::format(
                      "expected ')' to match the '(' at {}; got {}",
                      this->tkr->fileLocationMap.formatFileLocation(token.fileOffset, false),
                      closingToken.desc()));
        }
    } else {
        expr = Owned<Expression>::create();
        expr->tokenIdx = token.tokenIdx;
        switch (token.type) {
            case TokenType::Identifier: {
                expr->nameLookup().switchTo()->name = token.label;
                break;
            }
            case TokenType::NumericLiteral: {
                expr->integerLiteral().switchTo()->value = token.text.to<u32>();
                break;
            }
            case TokenType::BeginString: {
                parseInterpolatedString(this, expr);
                break;
            }
            default: {
                MethodTable::UnaryOp op = tokenToUnaryOp(token.type);
                if (op == MethodTable::UnaryOp::Invalid) {
                    this->tkr->rewindTo(token.tokenIdx);
                    if (!asStatement) { // Statement errors are issued by caller
                        error(this, token, ErrorTokenAction::DoNothing,
                              String::format("expected an expression; got {}", token.desc()));
                    }
                    return {};
                }
                auto unaryOp = expr->unaryOp().switchTo();
                unaryOp->op = op;
                unaryOp->expr = this->parseExpression();
                break;
            }
        }
    }
    this->recovery.muteErrors = false; // Got a valid expression

    // Try to extend the expression by consuming tokens to the right (eg. binary operators and
    // function call arguments).
    for (;;) {
        token = this->tkr->readToken();

        if (token.type == TokenType::OpenParen) {
            expr = parseArgumentList(this, std::move(expr), token.tokenIdx);
            continue;
        } else if (token.type == TokenType::Dot) {
            token = this->tkr->readToken();
            if (token.type != TokenType::Identifier) {
                error(this, token, ErrorTokenAction::PushBack,
                      String::format("expected identifier after '.'; got {}", token.desc()));
                continue;
            }
            auto propLookupExpr = Owned<Expression>::create();
            propLookupExpr->tokenIdx = token.tokenIdx;
            auto propLookup = propLookupExpr->propertyLookup().switchTo();
            propLookup->obj = std::move(expr);
            propLookup->propertyName = token.label;
            expr = std::move(propLookupExpr);
            continue;
        }

        MethodTable::BinaryOp op = tokenToBinaryOp(token.type);
        if (op != MethodTable::BinaryOp::Invalid) {
            u32 opPrecedence = BinaryOpPrecedence[(u32) op];
            if (opPrecedence >= outerPrecendenceLevel) {
                this->tkr->rewindTo(token.tokenIdx);
                return expr;
            }

            Owned<Expression> rhs = this->parseExpression(opPrecedence);
            if (!rhs)
                return expr; // an error occurred

            auto binaryOpExpr = Owned<Expression>::create();
            binaryOpExpr->tokenIdx = token.tokenIdx;
            auto binaryOp = binaryOpExpr->binaryOp().switchTo();
            binaryOp->op = op;
            binaryOp->left = std::move(expr);
            binaryOp->right = std::move(rhs);
            expr = std::move(binaryOpExpr);
            continue;
        }

        // Can't extend this expression any further.
        this->tkr->rewindTo(token.tokenIdx);
        return expr;
    }
}

Owned<Expression> parseExpressionWithTraits(Parser* parser, AnyOwnedObject* expressionTraits) {
    Owned<Expression> expr;
    for (;;) {
        // Try to parse a custom expression trait.
        if (parser->hooks->tryParseExpressionTrait(expressionTraits))
            continue;

        // Try to parse an expression unless we've already obtained one.
        if (expr)
            return expr;
        expr = parser->parseExpression(Limits<u32>::Max, true);
        if (!expr)
            return {};
    }
}

void Parser::parseStatement(StatementBlock* stmtBlock) {
    Label ifKey = LabelMap::instance.insertOrFind("if");
    Label whileKey = LabelMap::instance.insertOrFind("while");
    Label elseKey = LabelMap::instance.insertOrFind("else");
    Label returnKey = LabelMap::instance.insertOrFind("return");

    // Try to parse a custom block.
    if (this->hooks->tryParseCustomBlock(stmtBlock))
        return;

    // Try to parse a flow control statement.
    ExpandedToken token = this->tkr->readToken();
    auto stmt = Owned<Statement>::create();
    stmt->tokenIdx = token.tokenIdx;
    if (token.label == ifKey) {
        auto cond = stmt->if_().switchTo();
        cond->condition = this->parseExpression();
        cond->trueBlock = parseStatementBlock(this, {"if-statement", "if-condition", true});
        PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, false);
        token = this->tkr->readToken();
        if (token.label == elseKey) {
            cond->falseBlock = parseStatementBlock(this, {"else-block", "'else'", true});
        } else {
            this->tkr->rewindTo(token.tokenIdx);
        }
        stmtBlock->statements.append(std::move(stmt));
        return;
    } else if (token.label == whileKey) {
        auto cond = stmt->while_().switchTo();
        cond->condition = this->parseExpression();
        PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, false);
        cond->block = parseStatementBlock(this, {"while-loop", "'while'", true});
        stmtBlock->statements.append(std::move(stmt));
        return;
    } else if (token.label == returnKey) {
        auto return_ = stmt->return_().switchTo();
        return_->expr = this->parseExpression();
        stmtBlock->statements.append(std::move(stmt));
        return;
    }
    this->tkr->rewindTo(token.tokenIdx);

    AnyOwnedObject expressionTraits;
    Owned<Expression> expr = parseExpressionWithTraits(this, &expressionTraits);
    token = this->tkr->readToken();
    if (expr) {
        StringView statementType;
        if (token.type == TokenType::Equal) {
            if (expressionTraits.data) {
                // FIXME: Improve the error message by mentioning the specific trait that was
                // used.
                error(this, token, ErrorTokenAction::DoNothing,
                      "assignment not allowed when expression traits are used");
            }
            statementType = "assignment";
            auto assignment = stmt->assignment().switchTo();
            assignment->left = std::move(expr);
            assignment->right = this->parseExpression();
            token = this->tkr->readToken();
        } else {
            statementType = "expression";
            auto evaluate = stmt->evaluate().switchTo();
            evaluate->expr = std::move(expr);
            evaluate->traits = std::move(expressionTraits);
        }
        if ((token.type == TokenType::NewLine) || (token.type == TokenType::Semicolon)) {
            // This token marks the end of the statement.
        } else {
            this->tkr->rewindTo(token.tokenIdx);
            if (token.type != TokenType::CloseCurly) {
                error(this, token, ErrorTokenAction::DoNothing,
                      String::format("unexpected {} after {}", token.desc(), statementType));
            }
        }
        stmtBlock->statements.append(std::move(stmt));
        return;
    } else {
        if (error(this, token, ErrorTokenAction::HandleUnexpected,
                  String::format("expected a statement; got {}", token.desc())))
            return;
    }
}

Owned<StatementBlock> parseStatementBlock(Parser* parser, const StatementBlockProperties& props) {
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);
    ExpandedToken token = parser->tkr->readToken();
    parser->tkr->behavior.tokenizeNewLine = true;

    if (token.type == TokenType::OpenCurly) {
        auto block = Owned<StatementBlock>::create();

        for (;;) {
            ExpandedToken token = parser->tkr->readToken();
            switch (token.type) {
                case TokenType::NewLine:
                case TokenType::Semicolon: {
                    break;
                }
                case TokenType::CloseCurly: {
                    return block;
                }
                case TokenType::EndOfFile: {
                    error(parser, token, ErrorTokenAction::PushBack,
                          String::format("unexpected end-of-file inside {}", props.blockType));
                    return block;
                }
                default: {
                    parser->tkr->rewindTo(token.tokenIdx);
                    parser->parseStatement(block);
                    break;
                }
            }
        }
    } else if (props.curlyBracesOptionalIfControlFlow) {
        parser->tkr->rewindTo(token.tokenIdx);
        auto block = Owned<StatementBlock>::create();
        parser->parseStatement(block);
        bool isLegal = false;
        if (block->statements.numItems() == 1) {
            switch (block->statements[0]->id) {
                case Statement::ID::Return_:
                case Statement::ID::If_:
                    isLegal = true;
                    break;
                default:
                    break;
            }
        }
        if (!isLegal) {
            error(parser, token, ErrorTokenAction::DoNothing,
                  String::format("body of {} must be enclosed in curly braces unless it's a "
                                 "break, continue or return statement",
                                 props.blockType));
            parser->recovery.muteErrors = false;
        }
        return block;
    } else {
        error(parser, token, ErrorTokenAction::PushBack,
              String::format("expected '{{' after {}; got {}", props.afterItemText, token.desc()));
        return {};
    }
}

void parseParameterList(Parser* parser, Statement::FunctionDefinition* functionDef) {
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);
    ExpandedToken paramToken = parser->tkr->readToken();
    if (paramToken.type == TokenType::CloseParen) {
        parser->tkr->rewindTo(paramToken.tokenIdx);
        return;
    }
    PLY_SET_IN_SCOPE(parser->recovery.outerAcceptFlags,
                     parser->recovery.outerAcceptFlags | Parser::RecoveryState::AcceptCloseParen);
    for (;; paramToken = parser->tkr->readToken()) {
        if (paramToken.type != TokenType::Identifier) {
            if (error(parser, paramToken, ErrorTokenAction::HandleUnexpected,
                      String::format("expected function parameter; got {}", paramToken.desc())))
                return;
        }
        functionDef->parameterNames.append(paramToken.label);
        ExpandedToken token = parser->tkr->readToken();
        if (token.type == TokenType::CloseParen) {
            parser->tkr->rewindTo(token.tokenIdx);
            return;
        }
        if (token.type != TokenType::Comma) {
            if (error(parser, token, ErrorTokenAction::HandleUnexpected,
                      String::format("expected ',' or ')' after parameter {}; got {}",
                                     paramToken.desc(), token.desc())))
                return;
        }
    }
}

void parseFunctionDefinition(Parser* parser, const ExpandedToken& fnToken,
                             StatementBlock* stmtBlock) {
    auto stmt = Owned<Statement>::create();
    auto functionDef = stmt->functionDefinition().switchTo().get();
    functionDef->tkr = parser->tkr;
    PLY_SET_IN_SCOPE(parser->context.func, functionDef);

    // We got the 'fn' keyword.
    parser->recovery.muteErrors = false;

    // Parse function name.
    ExpandedToken nameToken = parser->tkr->readToken();
    if (nameToken.type != TokenType::Identifier) {
        error(parser, nameToken, ErrorTokenAction::PushBack,
              String::format("expected function name after 'fn'; got {}", nameToken.desc()));
        return;
    }
    functionDef->name = nameToken.label;

    // Parse parameter list.
    ExpandedToken token = parser->tkr->readToken();
    if (token.type != TokenType::OpenParen) {
        error(parser, token, ErrorTokenAction::PushBack,
              String::format("expected '(' after function name {}; got {}", nameToken.desc(),
                             token.desc()));
        return;
    }
    parseParameterList(parser, functionDef);
    token = parser->tkr->readToken();
    if (token.type != TokenType::CloseParen)
        return; // Typically parser would be EOF, in which case an error was already logged.

    // We got the closing ')'.
    parser->recovery.muteErrors = false;

    // Parse function body.
    functionDef->body = parseStatementBlock(parser, {"function", "parameter list"});
    stmtBlock->statements.append(std::move(stmt));
}

Owned<StatementBlock> Parser::parseFile() {
    Label fnKey = LabelMap::instance.insertOrFind("fn");
    auto stmtBlock = Owned<StatementBlock>::create();

    for (;;) {
        // Hooks allow the host application to customize parse behavior:
        if (this->hooks->tryParseCustomBlock(stmtBlock))
            continue;

        // Input was not handled by hooks.
        ExpandedToken token = this->tkr->readToken();
        if (token.type == TokenType::EndOfFile)
            break;
        if (token.label == fnKey) {
            parseFunctionDefinition(this, token, stmtBlock);
        } else {
            error(this, token, ErrorTokenAction::HandleUnexpected,
                  String::format("unexpected {}", token.desc()));
            continue;
        }
    }

    return stmtBlock;
}

} // namespace crowbar
} // namespace ply
