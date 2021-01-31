/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon/Core.h>
#include <pylon/Parse.h>
#include <ply-runtime/io/StdIO.h>

namespace pylon {

bool isAlnumUnit(u32 c) {
    return (c == '_') || (c == '$') || (c == '-') || (c == '.') || (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c >= 128);
}

void Parser::dumpError(const ParseError& error, OutStream& outs) const {
    FileLocation errorLoc = this->fileLocMap.getFileLocation(error.fileOfs);
    outs.format("({}, {}): error: {}\n", errorLoc.lineNumber, errorLoc.columnNumber, error.message);
    for (u32 i = 0; i < error.context.numItems(); i++) {
        const ParseError::Scope& scope = error.context.back(-(s32) i - 1);
        FileLocation contextLoc = this->fileLocMap.getFileLocation(scope.fileOfs);
        outs.format("({}, {}) ", contextLoc.lineNumber, contextLoc.columnNumber);
        switch (scope.type) {
            case ParseError::Scope::Object:
                outs << "while reading object started here";
                break;

            case ParseError::Scope::Property:
                outs << "while reading property " << scope.name << " started here";
                break;

            case ParseError::Scope::Duplicate:
                outs << "existing property was defined here";
                break;

            case ParseError::Scope::Array:
                outs << "while reading item " << scope.index
                   << " of the array started here (index is zero-based)";
                break;
        }
        outs << '\n';
    }
}

void Parser::error(u32 fileOfs, HybridString&& message) {
    if (this->errorCallback) {
        ParseError err{fileOfs, std::move(message), context};
        this->errorCallback.call(err);
    }
    this->anyError_ = true;
}

void Parser::advanceChar() {
    if (readOfs + 1 < srcView.numBytes) {
        readOfs++;
        nextUnit = srcView.bytes[readOfs];
    } else {
        nextUnit = -1;
    }
}

Parser::Token Parser::readPlainToken(Token::Type type) {
    Token result = {type, this->readOfs, {}};
    advanceChar();
    return result;
}

bool Parser::readEscapedHex(OutStream* outs, u32 escapeFileOfs) {
    PLY_ASSERT(0); // FIXME
    return false;
}

Parser::Token Parser::readQuotedString() {
    PLY_ASSERT(nextUnit == '"' || nextUnit == '\'');
    Token token = {Token::Type::Text, this->readOfs, {}};
    MemOutStream outs;
    NativeEndianWriter wr{&outs};
    s32 endByte = nextUnit;
    u32 quoteRun = 1;
    bool multiline = false;
    advanceChar();

    for (;;) {
        if (nextUnit == endByte) {
            advanceChar();
            if (quoteRun == 0) {
                if (multiline) {
                    quoteRun++;
                } else {
                    break; // end of string
                }
            } else {
                quoteRun++;
                if (quoteRun == 3) {
                    if (multiline) {
                        break; // end of string
                    } else {
                        multiline = true;
                        quoteRun = 0;
                    }
                }
            }
        } else {
            if (quoteRun > 0) {
                if (multiline) {
                    for (u32 i = 0; i < quoteRun; i++) {
                        wr.write((u8) endByte);
                    }
                } else if (quoteRun == 2) {
                    break; // empty string
                }
                quoteRun = 0;
            }

            switch (nextUnit) {
                case -1: {
                    error(this->readOfs, "Unexpected end of file in string literal");
                    return {};
                }

                case '\r':
                case '\n': {
                    if (multiline) {
                        if (nextUnit == '\n') {
                            wr.write((u8) nextUnit);
                        }
                        advanceChar();
                    } else {
                        error(this->readOfs, "Unexpected end of line in string literal");
                        return {};
                    }
                    break;
                }

                case '\\': {
                    // Escape sequence
                    u32 escapeFileOfs = this->readOfs;
                    advanceChar();
                    s32 code = nextUnit;
                    advanceChar();
                    switch (code) {
                        case -1: {
                            error(this->readOfs, "Unexpected end of file in string literal");
                            return {};
                        }

                        case '\r':
                        case '\n': {
                            error(this->readOfs, "Unexpected end of line in string literal");
                            return {};
                        }

                        case '\\':
                        case '\'':
                        case '"': {
                            wr.write((u8) code);
                            break;
                        }

                        case 'r': {
                            wr.write((u8) '\r');
                            break;
                        }

                        case 'n': {
                            wr.write((u8) '\n');
                            break;
                        }

                        case 't': {
                            wr.write((u8) '\t');
                            break;
                        }

                        case 'x': {
                            if (!readEscapedHex(&outs, escapeFileOfs))
                                return {}; // FIXME: Would be better to continue reading the
                                           // rest of the string
                            break;
                        }

                        default: {
                            error(escapeFileOfs, String::format("Unrecognized escape sequence \"\\{}\"",
                                                            (char) code));
                            return {}; // FIXME: Would be better to continue reading the rest
                                       // of the string
                        }
                    }
                    break;
                }

                default: {
                    wr.write((u8) nextUnit);
                    advanceChar();
                    break;
                }
            }
        }
    }

    token.text = outs.moveToString();
    return token;
}

Parser::Token Parser::readLiteral() {
    PLY_ASSERT(isAlnumUnit(nextUnit));
    Token token = {Token::Text, this->readOfs, {}};
    u32 startOfs = readOfs;

    while (isAlnumUnit(nextUnit)) {
        advanceChar();
    }

    token.text = StringView{(char*) srcView.bytes + startOfs, readOfs - startOfs};
    return token;
}

Parser::Token Parser::readToken(bool tokenizeNewLine) {
    if (pushBackToken.isValid()) {
        Token token = std::move(pushBackToken);
        pushBackToken = {};
        return token;
    }

    for (;;) {
        switch (nextUnit) {
            case ' ':
            case '\t':
            case '\r':
                advanceChar();
                break;

            case '\n': {
                u32 newLineOfs = this->readOfs;
                advanceChar();
                if (tokenizeNewLine)
                    return {Token::NewLine, newLineOfs, {}};
                break;
            }

            case -1:
                return {Token::EndOfFile, this->readOfs, {}};
            case '{':
                return readPlainToken(Token::OpenCurly);
            case '}':
                return readPlainToken(Token::CloseCurly);
            case '[':
                return readPlainToken(Token::OpenSquare);
            case ']':
                return readPlainToken(Token::CloseSquare);
            case ':':
                return readPlainToken(Token::Colon);
            case '=':
                return readPlainToken(Token::Equals);
            case ',':
                return readPlainToken(Token::Comma);
            case ';':
                return readPlainToken(Token::Semicolon);

            case '"':
            case '\'':
                return readQuotedString();

            default:
                if (isAlnumUnit(nextUnit))
                    return readLiteral();
                else
                    return {Token::Junk, this->readOfs, {}};
        }
    }
}

HybridString Parser::toString(const Token& token) {
    switch (token.type) {
        case Token::OpenCurly:
            return "\"{\"";
        case Token::CloseCurly:
            return "\"}\"";
        case Token::OpenSquare:
            return "\"[\"";
        case Token::CloseSquare:
            return "\"]\"";
        case Token::Colon:
            return "\":\"";
        case Token::Equals:
            return "\"=\"";
        case Token::Comma:
            return "\",\"";
        case Token::Semicolon:
            return "\";\"";
        case Token::Text:
            return String::format("text \"{}\"", fmt::EscapedString{token.text, 20});
        case Token::Junk:
            return String::format("junk \"{}\"", fmt::EscapedString{token.text, 20});
        case Token::NewLine:
            return "newline";
        case Token::EndOfFile:
            return "end of file";
        default:
            PLY_ASSERT(0);
            return "???";
    }
}

HybridString Parser::toString(const Node* node) {
    switch ((Node::Type) node->type) {
        case Node::Type::Object:
            return "object";
        case Node::Type::Array:
            return "array";
        case Node::Type::Text:
            return String::format("text \"{}\"", fmt::EscapedString{node->text(), 20});
        default:
            PLY_ASSERT(0);
            return "???";
    }
}

Owned<Node> Parser::readObject(const Token& startToken) {
    PLY_ASSERT(startToken.type == Token::OpenCurly);
    ScopeHandler objectScope{*this, ParseError::Scope::object(startToken.fileOfs)};
    Owned<Node> node = Node::createObject(startToken.fileOfs);
    struct PropLocationTraits {
        using Key = StringView;
        struct Item {
            HybridString name;
            u32 fileOfs;
        };
        static PLY_INLINE bool match(const Item& item, Key key) {
            return item.name.view() == key;
        }
    };
    HashMap<PropLocationTraits> propLocations;
    Token prevProperty = {};
    for (;;) {
        bool gotSeparator = false;
        Token firstToken = {};
        for (;;) {
            firstToken = readToken(true);
            switch (firstToken.type) {
                case Token::CloseCurly:
                    return node;

                case Token::Comma:
                case Token::Semicolon:
                case Token::NewLine:
                    gotSeparator = true;
                    break;

                default:
                    goto breakOuter;
            }
        }
    breakOuter:

        if (firstToken.type == Token::Text) {
            if (prevProperty.isValid() && !gotSeparator) {
                error(firstToken.fileOfs,
                      String::format("Expected a comma, semicolon or newline "
                                     "separator between properties \"{}\" and \"{}\"",
                                     fmt::EscapedString{prevProperty.text, 20},
                                     fmt::EscapedString{firstToken.text, 20}));
                return {};
            }
        } else if (prevProperty.isValid()) {
            error(firstToken.fileOfs,
                  String::format("Unexpected {} after property \"{}\"", toString(firstToken),
                                 fmt::EscapedString{prevProperty.text, 20}));
            return {};
        } else {
            error(firstToken.fileOfs,
                  String::format("Expected property, got {}", toString(firstToken)));
            return {};
        }

        auto propLocationCursor = propLocations.insertOrFind(firstToken.text);
        if (propLocationCursor.wasFound()) {
            ScopeHandler duplicateScope{*this,
                                        ParseError::Scope::duplicate(propLocationCursor->fileOfs)};
            error(firstToken.fileOfs, String::format("Duplicate property \"{}\"",
                                                      fmt::EscapedString{firstToken.text, 20}));
            return {};
        }

        Token colon = readToken();
        if (colon.type != Token::Colon && colon.type != Token::Equals) {
            error(colon.fileOfs,
                  String::format("Expected \":\" or \"=\" after \"{}\", got {}",
                                 fmt::EscapedString{firstToken.text, 20}, toString(colon)));
            return {};
        }

        {
            // Read value of property
            ScopeHandler propertyScope{
                *this, ParseError::Scope::property(firstToken.fileOfs, firstToken.text)};
            Owned<Node> value = readExpression(readToken(), &colon);
            if (!value->isValid())
                return value;
            propLocationCursor->name = firstToken.text.view();
            propLocationCursor->fileOfs = firstToken.fileOfs;
            node->set(std::move(firstToken.text), std::move(value));
        }

        prevProperty = std::move(firstToken);
    }
    return {};
}

Owned<Node> Parser::readArray(const Token& startToken) {
    PLY_ASSERT(startToken.type == Token::OpenSquare);
    ScopeHandler arrayScope{*this, ParseError::Scope::array(startToken.fileOfs, 0)};
    Owned<Node> arrayNode = Node::createArray(startToken.fileOfs);
    Token sepTokenHolder;
    Token* sepToken = nullptr;
    for (;;) {
        Token token = readToken(true);
        switch (token.type) {
            case Token::CloseSquare:
                return arrayNode;

            case Token::Comma:
            case Token::Semicolon:
            case Token::NewLine:
                sepTokenHolder = std::move(token);
                sepToken = &sepTokenHolder;
                break;

            default: {
                Owned<Node> value = readExpression(std::move(token), sepToken);
                if (!value->isValid())
                    return value;
                arrayNode->array().append(std::move(value));
                arrayScope.get().index++;
                sepToken = nullptr;
                break;
            }
        }
    }
}

Owned<Node> Parser::readExpression(Token&& firstToken, const Token* afterToken) {
    switch (firstToken.type) {
        case Token::OpenCurly:
            return readObject(firstToken);

        case Token::OpenSquare:
            return readArray(firstToken);

        case Token::Text:
            return Node::createText(std::move(firstToken.text), firstToken.fileOfs);

        case Token::Invalid:
            return {};

        default: {
            MemOutStream mout;
            mout << "Unexpected " << toString(firstToken);
            if (afterToken) {
                mout << " after " << toString(*afterToken);
            }
            error(firstToken.fileOfs, mout.moveToString());
            return {};
        }
    }
}

Parser::Result Parser::parse(StringView srcView_) {
    srcView = srcView_;
    nextUnit = srcView.numBytes > 0 ? srcView[0] : -1;

    this->fileLocMap = FileLocationMap::fromView(srcView_);

    Token rootToken = readToken();
    Owned<Node> root = readExpression(std::move(rootToken));
    if (!root->isValid())
        return {};

    Token nextToken = readToken();
    if (nextToken.type != Token::EndOfFile) {
        error(nextToken.fileOfs,
              String::format("Unexpected {} after {}", toString(nextToken), toString(root)));
        return {};
    }

    return {std::move(root), std::move(this->fileLocMap)};
}

} // namespace pylon
