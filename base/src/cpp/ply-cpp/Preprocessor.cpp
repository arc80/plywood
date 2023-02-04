/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

PLY_NO_INLINE void addPPDef(Preprocessor* pp, StringView identifier,
                            StringView expansion, bool takesArgs) {
    PPVisitedFiles* vf = pp->visitedFiles;

    u32 expIdx = vf->macroExpansions.numItems();
    PPVisitedFiles::MacroExpansion& exp = vf->macroExpansions.append();
    exp.setString(expansion);
    exp.takesArgs = takesArgs;

    pp->macros.assign(identifier, expIdx);
}

PLY_INLINE LinearLocation getLinearLocation(const Preprocessor* pp,
                                            const char* curByte) {
    const Preprocessor::StackItem& item = pp->stack.back();
    PLY_ASSERT(curByte >= item.in.start_byte);
    PLY_ASSERT(curByte <= item.in.end_byte);
    return pp->linearLocAtEndOfStackTop - (item.in.end_byte - curByte);
}

void Preprocessor::Error::writeMessage(OutStream& out,
                                       const PPVisitedFiles* visitedFiles) const {
    out.format("{}: error: ",
               expandFileLocation(visitedFiles, this->linearLoc).toString());
    switch (this->type) {
        case Preprocessor::Error::InvalidDirective: {
            out << "invalid preprocessing directive\n";
            break;
        }
        case Preprocessor::Error::EOFInMacro: {
            out << "unexpected end-of-file in preprocessor macro parameter list\n";
            out.format("{}: note: parameter list started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::EOFInComment: {
            out << "unexpected end-of-file in C-style comment\n";
            out.format("{}: note: comment started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::EOFInStringLiteral: {
            out << "unexpected end-of-file in string literal\n";
            out.format("{}: note: string literal started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::DirectiveNotAtStartOfLine: {
            out << "preprocessing directives must begin at start of line\n";
            break;
        }
        case Preprocessor::Error::EOFInRawStringDelimiter: {
            out << "unexpected end-of-file in raw string delimiter\n";
            out.format("{}: note: delimiter started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::InvalidCharInRawStringDelimiter: {
            out << "invalid character in raw string delimiter\n";
            out.format("{}: note: delimiter started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::InvalidStringLiteralPrefix: {
            out << "invalid string literal prefix\n";
            break;
        }
        case Preprocessor::Error::GarbageCharacters: {
            out << "garbage characters encountered\n";
            break;
        }
        default: {
            out << "error message not implemented!\n";
            break;
        }
    }
}

PLY_NO_INLINE void PPVisitedFiles::MacroExpansion::destruct() {
    if (this->fromFile) {
        subst::destructByMember(&this->range);
    } else {
        subst::destructByMember(&this->str);
    }
}

PLY_NO_INLINE StringView PPVisitedFiles::getContents(u32 includeChainIndex) const {
    const IncludeChain& chain = this->includeChains[includeChainIndex];
    if (chain.isMacroExpansion) {
        const MacroExpansion& exp = this->macroExpansions[chain.fileOrExpIdx];
        if (exp.fromFile) {
            const SourceFile& file = this->sourceFiles[exp.fromFile];
            return file.contents.view().subStr(exp.range.startOfs,
                                               exp.range.endOfs - exp.range.startOfs);
        } else {
            return exp.str;
        }
    } else {
        const SourceFile& file = this->sourceFiles[chain.fileOrExpIdx];
        return file.contents;
    }
}

void readNumericLiteral(ViewInStream& in) {
    if (in.ensure_readable() && (*in.cur_byte == '0')) {
        in.cur_byte++;
        if (in.ensure_readable() && (*in.cur_byte == 'x')) {
            in.cur_byte++;
            in.parse<u64>(fmt::Radix{16});
            goto suffix;
        }
    }
    in.parse<double>();
suffix:
    if (in.ensure_readable() && (*in.cur_byte == 'f')) {
        in.cur_byte++;
    } else {
        if (in.ensure_readable() && (*in.cur_byte == 'U')) {
            in.cur_byte++;
        }
        if (in.ensure_readable() && (*in.cur_byte == 'L')) {
            in.cur_byte++;
            if (in.ensure_readable() && (*in.cur_byte == 'L')) {
                in.cur_byte++;
            }
        }
    }
}

void readPreprocessorDirective(ViewInStream& in, Preprocessor* pp) {
    bool avail = in.ensure_readable();
    PLY_ASSERT(avail);
    PLY_UNUSED(avail);
    PLY_ASSERT(*in.cur_byte == '#');
    auto savePoint = in.get_save_point();
    in.cur_byte++;

    // Skip whitespace
    while (in.ensure_readable()) {
        char c = *in.cur_byte;
        if (!isWhite(c))
            break;
        in.cur_byte++;
        if (c == '\n')
            return; // null directive
    }

    // Read directive
    StringView directive = in.readView<fmt::Identifier>(fmt::WithDollarSign);
    if (directive.isEmpty()) {
        // invalid characters after #
        pp->error({Preprocessor::Error::InvalidDirective,
                   getLinearLocation(pp, in.cur_byte)});
        in.parse<fmt::Line>();
    } else {
        // Handle directive
        if (directive == "include") {
            in.parse<fmt::Line>();
            if (pp->includeCallback) {
                pp->includeCallback(in.getViewFrom(savePoint));
            }
        } else if (directive == "pragma" || directive == "if" || directive == "else" ||
                   directive == "elif" || directive == "endif" ||
                   directive == "ifdef" || directive == "ifndef" ||
                   directive == "error" || directive == "undef" ||
                   directive == "import") {
            in.parse<fmt::Line>();
        } else if (directive == "define") {
            bool escaping = false;
            while (in.ensure_readable()) {
                char c = *in.cur_byte;
                in.cur_byte++;
                if (c == '\n') {
                    if (!escaping)
                        break;
                } else if (c != '\r') {
                    escaping = (c == '\\');
                }
            }
        } else {
            // Invalid directive
            pp->error({Preprocessor::Error::InvalidDirective,
                       getLinearLocation(pp, directive.bytes)});
            in.parse<fmt::Line>();
        }
    }
    pp->atStartOfLine = true;
}

// Returns true if arguments were encountered, which means the macro can be expanded
// Returns false if arguments were not encountered, which means the macro cannot be
// expanded and the identifier should be returned as a token.
Array<Token> readMacroArguments(Preprocessor* pp, ViewInStream& in) {
    Array<Token> macroArgs;

    // Skip whitespace
    in.parse<fmt::Whitespace>();

    // Look for opening parenthesis
    if (!(in.ensure_readable() && *in.cur_byte == '('))
        return macroArgs;
    LinearLocation openParenLoc = getLinearLocation(pp, in.cur_byte);
    in.cur_byte++;
    const char* argStart = (const char*) in.cur_byte;
    LinearLocation argStartLoc = openParenLoc + 1;

    // Read up to closing parenthesis
    s32 nestLevel = 1;
    for (;;) {
        if (!in.ensure_readable()) {
            // end of file in macro arguments
            pp->error({Preprocessor::Error::EOFInMacro,
                       getLinearLocation(pp, in.cur_byte), openParenLoc});
            return macroArgs;
        }

        char c = *in.cur_byte;
        in.cur_byte++;
        // FIXME: Detect strings here
        switch (c) {
            case '/': {
                if (!in.ensure_readable()) {
                    // end of file in macro arguments
                    pp->error({Preprocessor::Error::EOFInMacro,
                               getLinearLocation(pp, in.cur_byte), openParenLoc});
                    return macroArgs;
                }
                if (*in.cur_byte == '/') {
                    in.cur_byte++;
                    in.parse<fmt::Line>();
                } else if (*in.cur_byte == '*') {
                    in.cur_byte++;
                    if (!fmt::scanUpToAndIncludingSpecial(in, "*/")) {
                        // EOF in comment
                        pp->error({Preprocessor::Error::EOFInComment,
                                   getLinearLocation(pp, in.cur_byte), openParenLoc});
                        return macroArgs;
                    }
                }
                break;
            }

            case '(': {
                nestLevel++;
                break;
            }

            case ')': {
                nestLevel--;
                if (nestLevel <= 0) {
                    macroArgs.append({argStartLoc, Token::MacroArgument,
                                      StringView::fromRange(
                                          argStart, (const char*) in.cur_byte - 1)});
                    return macroArgs;
                }
                break;
            }

            case ',': {
                if (nestLevel == 1) {
                    macroArgs.append({argStartLoc, Token::MacroArgument,
                                      StringView::fromRange(
                                          argStart, (const char*) in.cur_byte - 1)});
                    argStart = (const char*) in.cur_byte;
                    argStartLoc = getLinearLocation(pp, in.cur_byte);
                }
            }

            default:
                break;
        }
    }
}

bool readStringLiteral(ViewInStream& in, Preprocessor* pp, char quotePunc,
                       LinearLocation beginStringLoc) {
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       getLinearLocation(pp, in.cur_byte), beginStringLoc});
            return false;
        }
        char c = *in.cur_byte;
        in.cur_byte++;
        if (c == '\\') {
            if (!in.ensure_readable()) {
                // End of file in string literal
                pp->error({Preprocessor::Error::EOFInStringLiteral,
                           getLinearLocation(pp, in.cur_byte), beginStringLoc});
                return false;
            }
            in.cur_byte++;
        } else if (c == quotePunc) {
            return true;
        }
    }
}

bool readDelimiterAndRawStringLiteral(ViewInStream& in, Preprocessor* pp,
                                      LinearLocation beginStringLoc) {
    PLY_ASSERT(*in.cur_byte == '"');
    in.cur_byte++;

    // read delimiter
    const char* delimiterStart = in.cur_byte;
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file while reading raw string delimiter
            pp->error({Preprocessor::Error::EOFInRawStringDelimiter,
                       getLinearLocation(pp, in.cur_byte), beginStringLoc});
            return false;
        }
        char c = *in.cur_byte;
        if (c == '(')
            break;
        // FIXME: Recognize more whitespace characters
        if (isWhite(c) || c == ')' || c == '\\') {
            // Invalid character in delimiter
            pp->error({Preprocessor::Error::InvalidCharInRawStringDelimiter,
                       getLinearLocation(pp, in.cur_byte), beginStringLoc});
            return false;
        }
        in.cur_byte++;
    }

    // FIXME: Enforce maximum length of delimiter (at most 16 characters)
    const char* delimiterEnd = in.cur_byte;
    in.cur_byte++;

    // Read remainder of string
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       getLinearLocation(pp, in.cur_byte), beginStringLoc});
            return false;
        }
        char c = *in.cur_byte;
        in.cur_byte++;
        if (c == ')') {
            // Try to match delimiter
            const char* d = delimiterStart;
            for (;;) {
                if (d == delimiterEnd) {
                    if (!in.ensure_readable()) {
                        // End of file while matching closing "
                        pp->error({Preprocessor::Error::EOFInStringLiteral,
                                   getLinearLocation(pp, in.cur_byte), beginStringLoc});
                        return false;
                    }
                    c = *in.cur_byte;
                    if (c == '"') {
                        // End of string literal
                        in.cur_byte++;
                        return true;
                    }
                }
                if (!in.ensure_readable()) {
                    // End of file while matching delimiter
                    pp->error({Preprocessor::Error::EOFInStringLiteral,
                               getLinearLocation(pp, in.cur_byte), beginStringLoc});
                    return false;
                }
                c = *in.cur_byte;
                in.cur_byte++;
                if ((u8) c != *d)
                    break; // No match here
                d++;
            }
        }
    }
}

// Copied from TypeParser.cpp:
PLY_INLINE bool match(u8 c, const u32* mask) {
    u32 bitValue = mask[c >> 5] & (1 << (c & 31));
    return (bitValue != 0);
}

Token::Type readIdentifierOrLiteral(ViewInStream& in, Preprocessor* pp,
                                    LinearLocation beginTokenLoc) {
    char c = *in.cur_byte;
    if (c >= '0' && c <= '9') {
        readNumericLiteral(in);
        return Token::NumericLiteral;
    }

    // Copied from fmt::FormatParser<fmt::Identifier>::parse:
    u32 mask[8] = {0,          0,          0x87fffffe, 0x7fffffe,
                   0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    mask[1] |= 0x10; // '$'
    mask[1] |=
        0x3ff0000; // accept digits (we already know the first character is non-digit)

    const char* startByte = in.cur_byte;
    for (;;) {
        if (!in.ensure_readable()) {
            PLY_ASSERT(in.cur_byte != startByte);
            return Token::Identifier;
        }
        c = *in.cur_byte;
        if (!match(c, mask)) {
            if (c == '"') {
                if (in.cur_byte == startByte + 1 && *startByte == 'R') {
                    if (readDelimiterAndRawStringLiteral(in, pp, beginTokenLoc)) {
                        return Token::StringLiteral;
                    }
                } else {
                    // Treat it as a string prefix
                    in.cur_byte++;
                    if (!readStringLiteral(in, pp, c, beginTokenLoc))
                        return Token::Invalid;
                    return Token::StringLiteral;
                }
            } else {
                if (startByte == in.cur_byte) {
                    // Garbage token
                    // FIXME: Should we return 't report error here, but return garbage
                    // token to caller instead, so that parser can decide how to recover
                    // from it
                    in.cur_byte++;
                    pp->error({Preprocessor::Error::GarbageCharacters, beginTokenLoc});
                    return Token::Invalid;
                } else {
                    return Token::Identifier;
                }
            }
        }
        in.cur_byte++;
    }
}

PLY_NO_INLINE Token readToken(Preprocessor* pp) {
    Preprocessor::StackItem* item = nullptr;
    Token token;
    while (pp->stack.numItems() > 0) {
        item = &pp->stack.back();
        auto savePoint = item->in.get_save_point();
        if (!item->in.ensure_readable()) {
            pp->stack.pop();
            if (pp->stack.numItems() > 0) {
                PPVisitedFiles* vf = pp->visitedFiles;
                item = &pp->stack.back();

                // Add new locationMap range
                PPVisitedFiles::LocationMapTraits::Item locMapItem;
                locMapItem.linearLoc = pp->linearLocAtEndOfStackTop;
                locMapItem.includeChainIdx = item->includeChainIdx;
                locMapItem.offset =
                    safeDemote<u32>(item->in.cur_byte - item->in.start_byte);
                vf->locationMap.insert(std::move(locMapItem));

                pp->linearLocAtEndOfStackTop += item->in.num_bytes_readable();
            }
            continue;
        }

        PLY_ASSERT(pp->linearLocAtEndOfStackTop >= 0);
        token.linearLoc = pp->linearLocAtEndOfStackTop - item->in.num_bytes_readable();
        bool wasAtStartOfLine = pp->atStartOfLine;
        pp->atStartOfLine = false;
        switch (*item->in.cur_byte) {
            case '\n':
            case '\r':
            case '\t':
            case ' ': {
                // Skip whitespace while keeping track of start of line
                pp->atStartOfLine = wasAtStartOfLine;
                while (item->in.ensure_readable()) {
                    switch (*item->in.cur_byte) {
                        case '\n':
                            pp->atStartOfLine = true;
                        case '\r':
                        case '\t':
                        case ' ':
                            break;
                        default:
                            goto endOfWhite;
                    }
                    item->in.cur_byte++;
                }
            endOfWhite:
                // FIXME: Optionally return a whitespace token
                break;
            }

            case '#': {
                if (wasAtStartOfLine) {
                    // Preprocessor directive
                    readPreprocessorDirective(item->in, pp);
                    token.type = Token::Directive;
                    goto gotToken;
                } else {
                    // FIXME: Don't report error here. Return garbage token to caller
                    // instead (like below)
                    pp->error({Preprocessor::Error::DirectiveNotAtStartOfLine,
                               getLinearLocation(pp, item->in.cur_byte)});
                    item->in.cur_byte++;
                }
                break;
            }

            case '/': {
                LinearLocation startCommentLoc =
                    getLinearLocation(pp, item->in.cur_byte);
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '/') {
                        item->in.cur_byte++;
                        item->in.parse<fmt::Line>();
                        token.type = Token::LineComment;
                        pp->atStartOfLine = true;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '*') {
                        item->in.cur_byte++;
                        if (fmt::scanUpToAndIncludingSpecial(item->in, "*/")) {
                            token.type = Token::CStyleComment;
                            goto gotToken;
                        } else {
                            // EOF in comment
                            pp->error({Preprocessor::Error::EOFInComment,
                                       getLinearLocation(pp, item->in.cur_byte),
                                       startCommentLoc});
                        }
                        break;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::SlashEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::ForwardSlash;
                goto gotToken;
            }

            case '{': {
                item->in.cur_byte++;
                token.type = Token::OpenCurly;
                goto gotToken;
            }

            case '}': {
                item->in.cur_byte++;
                token.type = Token::CloseCurly;
                goto gotToken;
            }

            case ';': {
                item->in.cur_byte++;
                token.type = Token::Semicolon;
                goto gotToken;
            }

            case '(': {
                item->in.cur_byte++;
                token.type = Token::OpenParen;
                goto gotToken;
            }

            case ')': {
                item->in.cur_byte++;
                token.type = Token::CloseParen;
                goto gotToken;
            }

            case '<': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '<') {
                        item->in.cur_byte++;
                        token.type = Token::LeftShift;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::LessThanOrEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::OpenAngle;
                goto gotToken;
            }

            case '>': {
                item->in.cur_byte++;
                // FIXME: Disable tokenizeCloseAnglesOnly inside parenthesized
                // expressions
                if (!pp->tokenizeCloseAnglesOnly && item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '>') {
                        item->in.cur_byte++;
                        token.type = Token::RightShift;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::GreaterThanOrEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::CloseAngle;
                goto gotToken;
            }

            case '[': {
                item->in.cur_byte++;
                token.type = Token::OpenSquare;
                goto gotToken;
            }

            case ']': {
                item->in.cur_byte++;
                token.type = Token::CloseSquare;
                goto gotToken;
            }

            case ':': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == ':') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleColon;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleColon;
                goto gotToken;
            }

            case ',': {
                item->in.cur_byte++;
                token.type = Token::Comma;
                goto gotToken;
            }

            case '?': {
                item->in.cur_byte++;
                token.type = Token::QuestionMark;
                goto gotToken;
            }

            case '=': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleEqual;
                goto gotToken;
            }

            case '*': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::StarEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::Star;
                goto gotToken;
            }

            case '%': {
                item->in.cur_byte++;
                token.type = Token::Percent;
                goto gotToken;
            }

            case '&': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '&') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleAmpersand;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleAmpersand;
                goto gotToken;
            }

            case '|': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '|') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleVerticalBar;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleVerticalBar;
                goto gotToken;
            }

            case '+': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '+') {
                        item->in.cur_byte++;
                        token.type = Token::DoublePlus;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::PlusEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::SinglePlus;
                goto gotToken;
            }

            case '-': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '-') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleMinus;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::MinusEqual;
                        goto gotToken;
                    } else if (*item->in.cur_byte == '>') {
                        item->in.cur_byte++;
                        token.type = Token::Arrow;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleMinus;
                goto gotToken;
            }

            case '.': {
                item->in.cur_byte++;
                if (item->in.num_bytes_readable() >= 2) {
                    if (item->in.cur_byte[0] == '.' && item->in.cur_byte[1] == '.') {
                        item->in.cur_byte += 2;
                        token.type = Token::Ellipsis;
                        goto gotToken;
                    }
                }
                token.type = Token::Dot;
                goto gotToken;
            }

            case '~': {
                item->in.cur_byte++;
                token.type = Token::Tilde;
                goto gotToken;
            }

            case '^': {
                item->in.cur_byte++;
                token.type = Token::Caret;
                goto gotToken;
            }

            case '!': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::NotEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::Bang;
                goto gotToken;
            }

            case '\'':
            case '"': {
                LinearLocation beginStringLoc = getLinearLocation(pp, item->in.cur_byte);
                char c = *item->in.cur_byte;
                item->in.cur_byte++;
                if (!readStringLiteral(item->in, pp, c, beginStringLoc))
                    break;
                token.type = Token::StringLiteral;
                goto gotToken;
            }

            default: {
                LinearLocation beginTokenLoc = getLinearLocation(pp, item->in.cur_byte);
                token.type = readIdentifierOrLiteral(item->in, pp, beginTokenLoc);
                if (token.type != Token::Identifier) {
                    if (token.type == Token::Invalid)
                        break;
                    goto gotToken;
                }

                token.identifier = item->in.getViewFrom(savePoint);
                PLY_ASSERT(token.identifier);
                if (u32* expansionIdx = pp->macros.find(token.identifier)) {
                    token.type = Token::Macro;
                    token.identifier = item->in.getViewFrom(savePoint);

                    // This is a macro expansion
                    LinearLocation linearLocAtMacro =
                        pp->linearLocAtEndOfStackTop -
                        safeDemote<LinearLocation>(item->in.end_byte -
                                                   token.identifier.bytes);
                    PPVisitedFiles* vf = pp->visitedFiles;

                    const PPVisitedFiles::MacroExpansion& exp =
                        vf->macroExpansions[*expansionIdx];
                    pp->macroArgs.clear();
                    if (exp.takesArgs) {
                        // This macro expects arguments
                        auto savePoint = item->in.get_save_point();
                        pp->macroArgs = readMacroArguments(pp, item->in);
                        if (!pp->macroArgs) {
                            // No arguments were provided, so just return a plain token
                            item->in.rewind(savePoint);
                            token.type = Token::Identifier;
                            goto gotToken;
                        }
                    }

                    // FIXME: Implement special case for empty macros? (Would eliminate
                    // a locationMap entry).

                    // Add a new stack entry for the macro expansion
                    u32 prevChainIdx = item->includeChainIdx;
                    u32 includeChainIdx = vf->includeChains.numItems();
                    PPVisitedFiles::IncludeChain& chain = vf->includeChains.append();
                    chain.isMacroExpansion = true;
                    chain.fileOrExpIdx = *expansionIdx;
                    chain.parentIdx = prevChainIdx;

                    item = &pp->stack.append();
                    item->in = ViewInStream{vf->getContents(includeChainIdx)};
                    item->includeChainIdx = includeChainIdx;
                    pp->linearLocAtEndOfStackTop =
                        linearLocAtMacro + item->in.num_bytes_readable();

                    // Add new locationMap range
                    PPVisitedFiles::LocationMapTraits::Item locMapItem;
                    locMapItem.linearLoc = linearLocAtMacro;
                    locMapItem.includeChainIdx = includeChainIdx;
                    locMapItem.offset = 0;
                    vf->locationMap.insert(std::move(locMapItem));

                    return token;
                }

                token.type = Token::Identifier;
                goto gotToken;
            }
        }
        continue;

    gotToken:
        token.identifier = item->in.getViewFrom(savePoint);
        PLY_ASSERT(token.type != Token::Invalid);
        return token;
    }

    token.type = Token::EndOfFile;
    return token;
}

} // namespace cpp
} // namespace ply

#include "codegen/Preprocessor.inl" //%%
