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

void ParseError::dump(StringWriter& sw) const {
    sw.format("({}, {}): error: {}\n", location.line, location.column, message);
    for (u32 i = 0; i < context.numItems(); i++) {
        const Scope& scope = context.back(-(s32) i - 1);
        sw.format("({}, {}) ", scope.location.line, scope.location.column);
        switch (scope.type) {
            case Scope::Object:
                sw << "while reading object started here";
                break;

            case Scope::Property:
                sw << "while reading property " << scope.name << " started here";
                break;

            case Scope::Duplicate:
                sw << "existing property was defined here";
                break;

            case Scope::Array:
                sw << "while reading item " << scope.index
                   << " of the array started here (index is zero-based)";
                break;
        }
        sw << '\n';
    }
}

void Parser::error(Location location, HybridString&& message) {
    if (this->errorCallback) {
        ParseError err{location, std::move(message), context};
        this->errorCallback.call(err);
    }
    this->anyError_ = true;
}

void Parser::advanceChar() {
    switch (nextUnit) {
        case '\r':
        case -1:
            break;

        case '\t':
            location.column = ((location.column + tabSize - 1) / tabSize) * tabSize + 1;
            break;

        case '\n':
            location.line++;
            location.column = 1;
            break;

        default:
            location.column++;
            break;
    }

    if (readOfs + 1 < srcView.numBytes) {
        readOfs++;
        nextUnit = srcView.bytes[readOfs];
    } else {
        nextUnit = -1;
    }
}

Parser::Token Parser::readPlainToken(Token::Type type) {
    Token result = {type, location, {}};
    advanceChar();
    return result;
}

bool Parser::readEscapedHex(OutStream* outs, Location escapeLoc) {
    PLY_ASSERT(0); // FIXME
    return false;
}

Parser::Token Parser::readQuotedString() {
    PLY_ASSERT(nextUnit == '"' || nextUnit == '\'');
    Token token = {Token::Type::Text, location, {}};
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
                    error(location, "Unexpected end of file in string literal");
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
                        error(location, "Unexpected end of line in string literal");
                        return {};
                    }
                    break;
                }

                case '\\': {
                    // Escape sequence
                    Location escapeLoc = location;
                    advanceChar();
                    s32 code = nextUnit;
                    advanceChar();
                    switch (code) {
                        case -1: {
                            error(location, "Unexpected end of file in string literal");
                            return {};
                        }

                        case '\r':
                        case '\n': {
                            error(location, "Unexpected end of line in string literal");
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
                            if (!readEscapedHex(&outs, escapeLoc))
                                return {}; // FIXME: Would be better to continue reading the
                                           // rest of the string
                            break;
                        }

                        default: {
                            error(escapeLoc, String::format("Unrecognized escape sequence \"\\{}\"",
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
    Token token = {Token::Text, location, {}};
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
                Location newLineLoc = location;
                advanceChar();
                if (tokenizeNewLine)
                    return {Token::NewLine, newLineLoc, {}};
                break;
            }

            case -1:
                return {Token::EndOfFile, location, {}};
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
                    return {Token::Junk, location, {}};
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

HybridString Parser::toString(const Node& node) {
    switch (node.type.id) {
        case Node::Type::ID::Object:
            return "object";
        case Node::Type::ID::Array:
            return "array";
        case Node::Type::ID::Text:
            return String::format("text \"{}\"", fmt::EscapedString{node.text(), 20});
        default:
            PLY_ASSERT(0);
            return "???";
    }
}

Node Parser::readObject(const Token& startToken) {
    PLY_ASSERT(startToken.type == Token::OpenCurly);
    ScopeHandler objectScope{*this, ParseError::Scope::object(startToken.location)};
    Node node = Node::createObject(startToken.location);
    struct PropLocationTraits {
        using Key = StringView;
        struct Item {
            HybridString name;
            Location location;
        };
        static Key comparand(const Item& item) {
            return item.name.view();
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
                error(firstToken.location,
                      String::format("Expected a comma, semicolon or newline "
                                     "separator between properties \"{}\" and \"{}\"",
                                     fmt::EscapedString{prevProperty.text, 20},
                                     fmt::EscapedString{firstToken.text, 20}));
                return {};
            }
        } else if (prevProperty.isValid()) {
            error(firstToken.location,
                  String::format("Unexpected {} after property \"{}\"", toString(firstToken),
                                 fmt::EscapedString{prevProperty.text, 20}));
            return {};
        } else {
            error(firstToken.location,
                  String::format("Expected property, got {}", toString(firstToken)));
            return {};
        }

        auto propLocationCursor = propLocations.insertOrFind(firstToken.text);
        if (propLocationCursor.wasFound()) {
            ScopeHandler duplicateScope{*this,
                                        ParseError::Scope::duplicate(propLocationCursor->location)};
            error(firstToken.location, String::format("Duplicate property \"{}\"",
                                                      fmt::EscapedString{firstToken.text, 20}));
            return {};
        }

        Token colon = readToken();
        if (colon.type != Token::Colon && colon.type != Token::Equals) {
            error(colon.location,
                  String::format("Expected \":\" or \"=\" after \"{}\", got {}",
                                 fmt::EscapedString{firstToken.text, 20}, toString(colon)));
            return {};
        }

        {
            // Read value of property
            ScopeHandler propertyScope{
                *this, ParseError::Scope::property(firstToken.location, firstToken.text)};
            Node value = readExpression(readToken(), &colon);
            if (!value.isValid())
                return value;
            Node::Object::Item& objItem = node.object().add(std::move(firstToken.text));
            objItem.value = std::move(value);
            propLocationCursor->name = objItem.name.view();
            propLocationCursor->location = firstToken.location;
        }

        prevProperty = std::move(firstToken);
    }
    return {};
}

Node Parser::readArray(const Token& startToken) {
    PLY_ASSERT(startToken.type == Token::OpenSquare);
    ScopeHandler arrayScope{*this, ParseError::Scope::array(startToken.location, 0)};
    Node arrayNode = Node::createArray(startToken.location);
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
                Node value = readExpression(std::move(token), sepToken);
                if (!value.isValid())
                    return value;
                arrayNode.array().append(std::move(value));
                arrayScope.get().index++;
                sepToken = nullptr;
                break;
            }
        }
    }
}

Node Parser::readExpression(Token&& firstToken, const Token* afterToken) {
    switch (firstToken.type) {
        case Token::OpenCurly:
            return readObject(firstToken);

        case Token::OpenSquare:
            return readArray(firstToken);

        case Token::Text:
            return Node::createText(std::move(firstToken.text), firstToken.location);

        case Token::Invalid:
            return {};

        default: {
            StringWriter sw;
            sw << "Unexpected " << toString(firstToken);
            if (afterToken) {
                sw << " after " << toString(*afterToken);
            }
            error(firstToken.location, sw.moveToString());
            return {};
        }
    }
}

Node Parser::parse(ConstBufferView srcView_) {
    srcView = srcView_;
    nextUnit = srcView.numBytes > 0 ? srcView[0] : -1;

    Token rootToken = readToken();
    Node root = readExpression(std::move(rootToken));
    if (!root.isValid())
        return {};

    Token nextToken = readToken();
    if (nextToken.type != Token::EndOfFile) {
        error(nextToken.location,
              String::format("Unexpected {} after {}", toString(nextToken), toString(root)));
        return {};
    }

    return root;
}

} // namespace pylon
