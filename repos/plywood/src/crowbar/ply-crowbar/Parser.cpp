/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

enum class ErrorTokenAction { DoNothing, PushBack, HandleUnexpected };

PLY_NO_INLINE String getLocation(Parser* parser, const ExpandedToken& token) {
    FileLocation fileLoc = parser->tkr->fileLocationMap.getFileLocation(token.fileOffset);
    return String::format("({}, {})", fileLoc.lineNumber, fileLoc.columnNumber);
}

PLY_NO_INLINE bool error(Parser* parser, const ExpandedToken& errorToken,
                         ErrorTokenAction tokenAction, StringView message) {
    if (!parser->recovery.muteErrors) {
        MemOutStream msg;
        msg.format("{} error: {}\n", getLocation(parser, errorToken), message);
        parser->hooks->onError(msg.moveToString());
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

u32 BinaryOpPrecedence[] = {
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
};

Owned<Expression> parseArgumentList(Parser* parser, Owned<Expression>&& callable) {
    PLY_SET_IN_SCOPE(parser->recovery.outerAcceptFlags,
                     parser->recovery.outerAcceptFlags | Parser::RecoveryState::AcceptCloseParen);
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);
    auto callExpr = Owned<Expression>::create();
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

Owned<Expression> parseInterpolatedString(Parser* parser) {
    auto expr = Owned<crowbar::Expression>::create();
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
                                         getLocation(parser, token), closeToken.desc()));
                    skipAnyScope(parser, nullptr, TokenType::OpenCurly);
                }
                parser->recovery.muteErrors = false; // embed is now closed
                break;
            }
            case TokenType::EndString: {
                return expr;
            }
            default: {
                PLY_ASSERT(0); // Shouldn't get here
                break;
            }
        }
    }
}

Owned<Expression> Parser::parseExpression(u32 outerPrecendenceLevel) {
    Owned<Expression> expr;

    ExpandedToken token = this->tkr->readToken();
    switch (token.type) {
        case TokenType::Identifier: {
            expr = Owned<crowbar::Expression>::create();
            expr->nameLookup().switchTo()->name = token.stringKey;
            break;
        }
        case TokenType::NumericLiteral: {
            expr = Owned<crowbar::Expression>::create();
            expr->integerLiteral().switchTo()->value = token.text.to<u32>();
            break;
        }
        case TokenType::BeginString: {
            expr = parseInterpolatedString(this);
            break;
        }
        case TokenType::OpenParen: {
            PLY_SET_IN_SCOPE(this->recovery.outerAcceptFlags,
                             this->recovery.outerAcceptFlags |
                                 Parser::RecoveryState::AcceptCloseParen);
            PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, false);
            expr = this->parseExpression();
            ExpandedToken closingToken = this->tkr->readToken();
            if (closingToken.type != TokenType::CloseParen) {
                error(this, closingToken, ErrorTokenAction::PushBack,
                      String::format("expected ')' to match the '(' at {}; got {}",
                                     getLocation(this, token), closingToken.desc()));
                return {};
            }
            this->recovery.muteErrors = false;
            break;
        }
        default: {
            error(this, token, ErrorTokenAction::PushBack,
                  String::format("expected an expression; got {}", token.desc()));
            return {};
        }
    }
    this->recovery.muteErrors = false; // Got a valid expression

    // Try to extend the expression by consuming tokens to the right (eg. binary operators and
    // function call arguments).
    for (;;) {
        token = this->tkr->readToken();

        if (token.type == TokenType::OpenParen) {
            expr = parseArgumentList(this, std::move(expr));
            continue;
        } else if (token.type == TokenType::Dot) {
            token = this->tkr->readToken();
            if (token.type != TokenType::Identifier) {
                error(this, token, ErrorTokenAction::PushBack,
                      String::format("expected identifier after '.'; got {}",
                                     token.desc()));
                continue;
            }
            auto propLookupExpr = Owned<crowbar::Expression>::create();
            auto propLookup = propLookupExpr->propertyLookup().switchTo();
            propLookup->obj = std::move(expr);
            propLookup->propertyName = token.stringKey;
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

            auto binaryOpExpr = Owned<crowbar::Expression>::create();
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

Owned<Statement> Parser::parseStatement() {
    u32 ifKey = this->tkr->internedStrings->findOrInsertKey("if");
    u32 whileKey = this->tkr->internedStrings->findOrInsertKey("while");
    u32 elseKey = this->tkr->internedStrings->findOrInsertKey("else");
    u32 returnKey = this->tkr->internedStrings->findOrInsertKey("return");
    auto stmt = Owned<crowbar::Statement>::create();

    ExpandedToken token = this->tkr->readToken();
    if (token.stringKey == ifKey) {
        auto cond = stmt->if_().switchTo();
        cond->condition = this->parseExpression();
        cond->trueBlock = this->parseNestedBlock("if-statement");
        token = this->tkr->readToken();
        if (token.stringKey == elseKey) {
            cond->falseBlock = this->parseNestedBlock("else-block");
        } else {
            this->tkr->rewindTo(token.tokenIdx);
        }
        return stmt;
    } else if (token.stringKey == whileKey) {
        auto cond = stmt->while_().switchTo();
        cond->condition = this->parseExpression();
        cond->block = this->parseNestedBlock("while-loop");
        return stmt;
    } else if (token.stringKey == returnKey) {
        auto return_ = stmt->return_().switchTo();
        return_->expr = this->parseExpression();
        return stmt;
    } else {
        this->tkr->rewindTo(token.tokenIdx);
    }

    Owned<Expression> expr = this->parseExpression();
    token = this->tkr->readToken();
    switch (token.type) {
        case TokenType::Equal: {
            auto assignment = stmt->assignment().switchTo();
            assignment->left = std::move(expr);
            assignment->right = this->parseExpression();
            return stmt;
        }
        default: {
            auto evaluate = stmt->evaluate().switchTo();
            evaluate->expr = std::move(expr);
            return stmt;
        }
    }

    return {};
}

Owned<StatementBlock> Parser::parseStatementBlock() {
    PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, true);
    auto block = Owned<StatementBlock>::create();

    for (;;) {
        ExpandedToken token = this->tkr->readToken();
        switch (token.type) {
            case TokenType::NewLine:
            case TokenType::Semicolon: {
                break;
            }
            case TokenType::CloseCurly:
            case TokenType::EndOfFile: {
                this->tkr->rewindTo(token.tokenIdx);
                goto recover;
            }
            default: {
                this->tkr->rewindTo(token.tokenIdx);
                Owned<Statement> stmt = this->parseStatement();
                block->statements.append(std::move(stmt));
                break;
            }
        }
    }

recover:
    return block;
}

Owned<StatementBlock> Parser::parseNestedBlock(StringView forStatementType) {
    PLY_SET_IN_SCOPE(this->tkr->behavior.tokenizeNewLine, false);
    ExpandedToken token = this->tkr->readToken();
    if (token.type == TokenType::OpenCurly) {
        Owned<StatementBlock> block = this->parseStatementBlock();
        token = this->tkr->readToken();
        if (token.type != TokenType::CloseCurly) {
            if (!error(this, token, ErrorTokenAction::HandleUnexpected,
                       String::format("unexpected {} inside {}", token.desc(), forStatementType)))
                return {};
        }
        return block;
    } else {
        this->tkr->rewindTo(token.tokenIdx);
        Owned<Statement> statement = this->parseStatement();
        switch (statement->id) {
            case Statement::ID::Return_:
                break;
            default: {
                error(this, token, ErrorTokenAction::DoNothing,
                      String::format("body of {} must be enclosed in curly braces unless it's a "
                                     "break, continue or return statement",
                                     forStatementType));
                this->recovery.muteErrors = false;
                break;
            }
        }
        auto block = Owned<StatementBlock>::create();
        block->statements.append(std::move(statement));
        return block;
    }
}

void parseParameterList(Parser* parser, FunctionDefinition* functionDef) {
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
        functionDef->parameterNames.append(paramToken.stringKey);
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

Owned<FunctionDefinition> parseFunctionDefinition(Parser* parser, const ExpandedToken& fnToken) {
    auto functionDef = Owned<FunctionDefinition>::create();
    PLY_SET_IN_SCOPE(parser->tkr->behavior.tokenizeNewLine, false);

    // We got the 'fn' keyword.
    parser->recovery.muteErrors = false;

    // Parse function name.
    ExpandedToken nameToken = parser->tkr->readToken();
    if (nameToken.type != TokenType::Identifier) {
        error(parser, nameToken, ErrorTokenAction::PushBack,
              String::format("expected function name after 'fn'; got {}", nameToken.desc()));
        return {};
    }
    functionDef->name = nameToken.stringKey;

    // Parse parameter list.
    ExpandedToken token = parser->tkr->readToken();
    if (token.type != TokenType::OpenParen) {
        error(parser, token, ErrorTokenAction::PushBack,
              String::format("expected '(' after function name {}; got {}", nameToken.desc(),
                             token.desc()));
        return {};
    }
    parseParameterList(parser, functionDef);
    token = parser->tkr->readToken();
    if (token.type != TokenType::CloseParen)
        return {}; // Typically parser would be EOF, in which case an error was already logged.

    // We got the closing ')'.
    parser->recovery.muteErrors = false;

    // Parse function body.
    token = parser->tkr->readToken();
    if (token.type != TokenType::OpenCurly) {
        error(parser, token, ErrorTokenAction::PushBack,
              String::format("expected '{{' after parameter list; got {}", token.desc()));
        return {};
    }
    functionDef->body = parser->parseStatementBlock();
    token = parser->tkr->readToken();
    if (token.type != TokenType::CloseCurly) {
        error(parser, token, ErrorTokenAction::PushBack,
              String::format("unexpected {} inside function", token.desc()));
        return {};
    }

    return functionDef;
}

Owned<File> Parser::parseFile() {
    u32 fnKey = this->tkr->internedStrings->findOrInsertKey("fn");
    auto file = Owned<File>::create();

    for (;;) {
        ExpandedToken token = this->tkr->readToken();
        if (token.type == TokenType::EndOfFile)
            break;

        if (token.stringKey == fnKey) {
            Owned<FunctionDefinition> functionDef = parseFunctionDefinition(this, token);
            if (functionDef) {
                file->functions.append(std::move(functionDef));
            }
        } else {
            error(this, token, ErrorTokenAction::HandleUnexpected,
                  String::format("unexpected {}", token.desc()));
            continue;
        }
    }

    return file;
}

} // namespace crowbar
} // namespace ply