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

PLY_INLINE bool error(Parser* parser, const ExpandedToken& errorToken, ErrorTokenAction tokenAction,
                      StringView message) {
    if (!parser->recovery.muteErrors) {
        MemOutStream msg;
        FileLocation fileLoc = parser->tkr->fileLocationMap.getFileLocation(errorToken.fileOffset);
        msg.format("({}, {}) error: {}\n", fileLoc.lineNumber, fileLoc.columnNumber, message);
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
    3, // Multiply
    3, // Divide
    3, // Modulo
    4, // Add
    4, // Subtract
    6, // LessThan
    6, // LessThanOrEqual
    6, // GreaterThan
    6, // GreaterThanOrEqual
    7, // DoubleEqual
};

Owned<Expression> Parser::parseExpression(bool required, u32 outerPrecendenceLevel) {
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
        default: {
            PLY_ASSERT(0);
            goto recover;
        }
    }

    for (;;) {
        token = this->tkr->readToken();
        auto extendBinaryOp = [&](MethodTable::BinaryOp op) {
            u32 opPrecedence = BinaryOpPrecedence[(u32) op];
            if (opPrecedence >= outerPrecendenceLevel) {
                this->tkr->rewindTo(token.tokenIdx);
                return false;
            }

            auto binaryOpExpr = Owned<crowbar::Expression>::create();
            auto binaryOp = binaryOpExpr->binaryOp().switchTo();
            binaryOp->op = op;
            binaryOp->left = std::move(expr);
            binaryOp->right = this->parseExpression(true, opPrecedence);
            expr = std::move(binaryOpExpr);
            return true;
        };
        switch (token.type) {
            case TokenType::LessThan: {
                if (!extendBinaryOp(MethodTable::BinaryOp::LessThan))
                    goto recover;
                break;
            }
            case TokenType::LessThanOrEqual: {
                if (!extendBinaryOp(MethodTable::BinaryOp::LessThanOrEqual))
                    goto recover;
                break;
            }
            case TokenType::GreaterThan: {
                if (!extendBinaryOp(MethodTable::BinaryOp::GreaterThan))
                    goto recover;
                break;
            }
            case TokenType::GreaterThanOrEqual: {
                if (!extendBinaryOp(MethodTable::BinaryOp::GreaterThanOrEqual))
                    goto recover;
                break;
            }
            case TokenType::DoubleEqual: {
                if (!extendBinaryOp(MethodTable::BinaryOp::DoubleEqual))
                    goto recover;
                break;
            }
            case TokenType::Plus: {
                if (!extendBinaryOp(MethodTable::BinaryOp::Add))
                    goto recover;
                break;
            }
            case TokenType::Minus: {
                if (!extendBinaryOp(MethodTable::BinaryOp::Subtract))
                    goto recover;
                break;
            }
            case TokenType::Asterisk: {
                if (!extendBinaryOp(MethodTable::BinaryOp::Multiply))
                    goto recover;
                break;
            }
            case TokenType::Slash: {
                if (!extendBinaryOp(MethodTable::BinaryOp::Divide))
                    goto recover;
                break;
            }
            case TokenType::Percent: {
                if (!extendBinaryOp(MethodTable::BinaryOp::Modulo))
                    goto recover;
                break;
            }
            case TokenType::OpenParen: {
                PLY_SET_IN_SCOPE(this->tkr->tokenizeNewLine, false);
                auto callExpr = Owned<crowbar::Expression>::create();
                auto call = callExpr->call().switchTo();
                call->callable = std::move(expr);

                // Parse argument list.
                token = this->tkr->readToken();
                if (token.type != TokenType::CloseParen) {
                    this->tkr->rewindTo(token.tokenIdx);
                    for (;;) {
                        call->args.append(this->parseExpression());
                        token = this->tkr->readToken();
                        if (token.type == TokenType::CloseParen)
                            break;
                        PLY_ASSERT(token.type == TokenType::Comma);
                    }
                }

                expr = std::move(callExpr);
                break;
            }
            default: {
                this->tkr->rewindTo(token.tokenIdx);
                goto recover;
            }
        }
    }

recover:;
    return expr;
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

    Owned<Expression> expr = this->parseExpression(false);
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
    PLY_SET_IN_SCOPE(this->tkr->tokenizeNewLine, true);
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
    PLY_SET_IN_SCOPE(this->tkr->tokenizeNewLine, false);
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
    PLY_SET_IN_SCOPE(parser->tkr->tokenizeNewLine, false);

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