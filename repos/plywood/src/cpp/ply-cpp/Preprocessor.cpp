/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

PLY_NO_INLINE void addPPDef(Preprocessor* pp, StringView identifier, StringView expansion,
                            bool takesArgs) {
    PPVisitedFiles* vf = pp->visitedFiles;

    u32 expIdx = vf->macroExpansions.numItems();
    PPVisitedFiles::MacroExpansion& exp = vf->macroExpansions.append();
    exp.setString(expansion);
    exp.takesArgs = takesArgs;

    auto cursor = pp->macros.insertOrFind(identifier);
    cursor->identifier = identifier;
    cursor->expansionIdx = expIdx;
}

PLY_INLINE LinearLocation getLinearLocation(const Preprocessor* pp, const char* curByte) {
    const Preprocessor::StackItem& item = pp->stack.back();
    PLY_ASSERT(curByte >= item.vins.getStartByte());
    PLY_ASSERT(curByte <= item.vins.endByte);
    return pp->linearLocAtEndOfStackTop - (item.vins.endByte - curByte);
}

void Preprocessor::Error::writeMessage(OutStream* outs, const PPVisitedFiles* visitedFiles) const {
    outs->format("{}: error: ", expandFileLocation(visitedFiles, this->linearLoc).toString());
    switch (this->type) {
        case Preprocessor::Error::InvalidDirective: {
            *outs << "invalid preprocessing directive\n";
            break;
        }
        case Preprocessor::Error::EOFInMacro: {
            *outs << "unexpected end-of-file in preprocessor macro parameter list\n";
            outs->format("{}: note: parameter list started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::EOFInComment: {
            *outs << "unexpected end-of-file in C-style comment\n";
            outs->format("{}: note: comment started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::EOFInStringLiteral: {
            *outs << "unexpected end-of-file in string literal\n";
            outs->format("{}: note: string literal started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::DirectiveNotAtStartOfLine: {
            *outs << "preprocessing directives must begin at start of line\n";
            break;
        }
        case Preprocessor::Error::EOFInRawStringDelimiter: {
            *outs << "unexpected end-of-file in raw string delimiter\n";
            outs->format("{}: note: delimiter started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::InvalidCharInRawStringDelimiter: {
            *outs << "invalid character in raw string delimiter\n";
            outs->format("{}: note: delimiter started here\n",
                       expandFileLocation(visitedFiles, this->otherLoc).toString());
            break;
        }
        case Preprocessor::Error::InvalidStringLiteralPrefix: {
            *outs << "invalid string literal prefix\n";
            break;
        }
        case Preprocessor::Error::GarbageCharacters: {
            *outs << "garbage characters encountered\n";
            break;
        }
        default: {
            *outs << "error message not implemented!\n";
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

void readNumericLiteral(ViewInStream* vins) {
    if (vins->tryMakeBytesAvailable() && (vins->peekByte() == '0')) {
        vins->advanceByte();
        if (vins->tryMakeBytesAvailable() && (vins->peekByte() == 'x')) {
            vins->advanceByte();
            vins->parse<u64>(fmt::Radix{16});
            goto suffix;
        }
    }
    vins->parse<double>();
suffix:
    if (vins->tryMakeBytesAvailable() && (vins->peekByte() == 'f')) {
        vins->advanceByte();
    } else {
        if (vins->tryMakeBytesAvailable() && (vins->peekByte() == 'U')) {
            vins->advanceByte();
        }
        if (vins->tryMakeBytesAvailable() && (vins->peekByte() == 'L')) {
            vins->advanceByte();
            if (vins->tryMakeBytesAvailable() && (vins->peekByte() == 'L')) {
                vins->advanceByte();
            }
        }
    }
}

void readPreprocessorDirective(ViewInStream* vins, Preprocessor* pp) {
    bool avail = vins->tryMakeBytesAvailable();
    PLY_ASSERT(avail);
    PLY_UNUSED(avail);
    PLY_ASSERT(vins->peekByte() == '#');
    auto savePoint = vins->savePoint();
    vins->advanceByte();

    // Skip whitespace
    while (vins->tryMakeBytesAvailable()) {
        char c = vins->peekByte();
        if (!isWhite(c))
            break;
        vins->advanceByte();
        if (c == '\n')
            return; // null directive
    }

    // Read directive
    StringView directive = vins->readView<fmt::Identifier>(fmt::WithDollarSign);
    if (directive.isEmpty()) {
        // invalid characters after #
        pp->error(
            {Preprocessor::Error::InvalidDirective, getLinearLocation(pp, vins->curByte)});
        vins->parse<fmt::Line>();
    } else {
        // Handle directive
        if (directive == "include") {
            vins->parse<fmt::Line>();
            if (pp->includeCallback) {
                pp->includeCallback(vins->getViewFrom(savePoint));
            }
        } else if (directive == "pragma" || directive == "if" || directive == "else" ||
                   directive == "elif" || directive == "endif" || directive == "ifdef" ||
                   directive == "ifndef" || directive == "error" || directive == "undef" ||
                   directive == "import") {
            vins->parse<fmt::Line>();
        } else if (directive == "define") {
            bool escaping = false;
            while (vins->tryMakeBytesAvailable()) {
                char c = vins->peekByte();
                vins->advanceByte();
                if (c == '\n') {
                    if (!escaping)
                        break;
                } else if (c != '\r') {
                    escaping = (c == '\\');
                }
            }
        } else {
            // Invalid directive
            pp->error(
                {Preprocessor::Error::InvalidDirective, getLinearLocation(pp, directive.bytes)});
            vins->parse<fmt::Line>();
        }
    }
    pp->atStartOfLine = true;
}

// Returns true if arguments were encountered, which means the macro can be expanded
// Returns false if arguments were not encountered, which means the macro cannot be expanded
// and the identifier should be returned as a token.
Array<Token> readMacroArguments(Preprocessor* pp, ViewInStream* vins) {
    Array<Token> macroArgs;

    // Skip whitespace
    vins->parse<fmt::Whitespace>();

    // Look for opening parenthesis
    if (!(vins->tryMakeBytesAvailable() && vins->peekByte() == '('))
        return macroArgs;
    LinearLocation openParenLoc = getLinearLocation(pp, vins->curByte);
    vins->advanceByte();
    const char* argStart = (const char*) vins->curByte;
    LinearLocation argStartLoc = openParenLoc + 1;

    // Read up to closing parenthesis
    s32 nestLevel = 1;
    for (;;) {
        if (!vins->tryMakeBytesAvailable()) {
            // end of file in macro arguments
            pp->error({Preprocessor::Error::EOFInMacro,
                       getLinearLocation(pp, vins->curByte), openParenLoc});
            return macroArgs;
        }

        char c = vins->peekByte();
        vins->advanceByte();
        // FIXME: Detect strings here
        switch (c) {
            case '/': {
                if (!vins->tryMakeBytesAvailable()) {
                    // end of file in macro arguments
                    pp->error({Preprocessor::Error::EOFInMacro,
                               getLinearLocation(pp, vins->curByte), openParenLoc});
                    return macroArgs;
                }
                if (vins->peekByte() == '/') {
                    vins->advanceByte();
                    vins->parse<fmt::Line>();
                } else if (vins->peekByte() == '*') {
                    vins->advanceByte();
                    if (!fmt::scanUpToAndIncludingSpecial(vins, "*/")) {
                        // EOF in comment
                        pp->error({Preprocessor::Error::EOFInComment,
                                   getLinearLocation(pp, vins->curByte), openParenLoc});
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
                                          argStart, (const char*) vins->curByte - 1)});
                    return macroArgs;
                }
                break;
            }

            case ',': {
                if (nestLevel == 1) {
                    macroArgs.append({argStartLoc, Token::MacroArgument,
                                      StringView::fromRange(
                                          argStart, (const char*) vins->curByte - 1)});
                    argStart = (const char*) vins->curByte;
                    argStartLoc = getLinearLocation(pp, vins->curByte);
                }
            }

            default:
                break;
        }
    }
}

bool readStringLiteral(ViewInStream* vins, Preprocessor* pp, char quotePunc,
                       LinearLocation beginStringLoc) {
    for (;;) {
        if (!vins->tryMakeBytesAvailable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       getLinearLocation(pp, vins->curByte), beginStringLoc});
            return false;
        }
        char c = vins->peekByte();
        vins->advanceByte();
        if (c == '\\') {
            if (!vins->tryMakeBytesAvailable()) {
                // End of file in string literal
                pp->error({Preprocessor::Error::EOFInStringLiteral,
                           getLinearLocation(pp, vins->curByte), beginStringLoc});
                return false;
            }
            vins->advanceByte();
        } else if (c == quotePunc) {
            return true;
        }
    }
}

bool readDelimiterAndRawStringLiteral(ViewInStream* vins, Preprocessor* pp,
                                      LinearLocation beginStringLoc) {
    PLY_ASSERT(vins->peekByte() == '"');
    vins->advanceByte();

    // read delimiter
    const char* delimiterStart = vins->curByte;
    for (;;) {
        if (!vins->tryMakeBytesAvailable()) {
            // End of file while reading raw string delimiter
            pp->error({Preprocessor::Error::EOFInRawStringDelimiter,
                       getLinearLocation(pp, vins->curByte), beginStringLoc});
            return false;
        }
        char c = vins->peekByte();
        if (c == '(')
            break;
        // FIXME: Recognize more whitespace characters
        if (isWhite(c) || c == ')' || c == '\\') {
            // Invalid character in delimiter
            pp->error({Preprocessor::Error::InvalidCharInRawStringDelimiter,
                       getLinearLocation(pp, vins->curByte), beginStringLoc});
            return false;
        }
        vins->advanceByte();
    }

    // FIXME: Enforce maximum length of delimiter (at most 16 characters)
    const char* delimiterEnd = vins->curByte;
    vins->advanceByte();

    // Read remainder of string
    for (;;) {
        if (!vins->tryMakeBytesAvailable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       getLinearLocation(pp, vins->curByte), beginStringLoc});
            return false;
        }
        char c = vins->peekByte();
        vins->advanceByte();
        if (c == ')') {
            // Try to match delimiter
            const char* d = delimiterStart;
            for (;;) {
                if (d == delimiterEnd) {
                    if (!vins->tryMakeBytesAvailable()) {
                        // End of file while matching closing "
                        pp->error({Preprocessor::Error::EOFInStringLiteral,
                                   getLinearLocation(pp, vins->curByte), beginStringLoc});
                        return false;
                    }
                    c = vins->peekByte();
                    if (c == '"') {
                        // End of string literal
                        vins->advanceByte();
                        return true;
                    }
                }
                if (!vins->tryMakeBytesAvailable()) {
                    // End of file while matching delimiter
                    pp->error({Preprocessor::Error::EOFInStringLiteral,
                               getLinearLocation(pp, vins->curByte), beginStringLoc});
                    return false;
                }
                c = vins->peekByte();
                vins->advanceByte();
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

Token::Type readIdentifierOrLiteral(ViewInStream* vins, Preprocessor* pp,
                                    LinearLocation beginTokenLoc) {
    char c = vins->peekByte();
    if (c >= '0' && c <= '9') {
        readNumericLiteral(vins);
        return Token::NumericLiteral;
    }

    // Copied from fmt::FormatParser<fmt::Identifier>::parse:
    u32 mask[8] = {0, 0, 0x87fffffe, 0x7fffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    mask[1] |= 0x10;      // '$'
    mask[1] |= 0x3ff0000; // accept digits (we already know the first character is non-digit)

    const char* startByte = vins->curByte;
    for (;;) {
        if (!vins->tryMakeBytesAvailable()) {
            PLY_ASSERT(vins->curByte != startByte);
            return Token::Identifier;
        }
        c = vins->peekByte();
        if (!match(c, mask)) {
            if (c == '"') {
                if (vins->curByte == startByte + 1 && *startByte == 'R') {
                    if (readDelimiterAndRawStringLiteral(vins, pp, beginTokenLoc)) {
                        return Token::StringLiteral;
                    }
                } else {
                    // Treat it as a string prefix
                    vins->advanceByte();
                    if (!readStringLiteral(vins, pp, c, beginTokenLoc))
                        return Token::Invalid;
                    return Token::StringLiteral;
                }
            } else {
                if (startByte == vins->curByte) {
                    // Garbage token
                    // FIXME: Should we return 't report error here, but return garbage token to
                    // caller instead, so that parser can decide how to recover from it
                    vins->advanceByte();
                    pp->error({Preprocessor::Error::GarbageCharacters, beginTokenLoc});
                    return Token::Invalid;
                } else {
                    return Token::Identifier;
                }
            }
        }
        vins->advanceByte();
    }
}

PLY_NO_INLINE Token readToken(Preprocessor* pp) {
    Preprocessor::StackItem* item = nullptr;
    Token token;
    while (pp->stack.numItems() > 0) {
        item = &pp->stack.back();
        auto savePoint = item->vins.savePoint();
        if (!item->vins.tryMakeBytesAvailable()) {
            pp->stack.pop();
            if (pp->stack.numItems() > 0) {
                PPVisitedFiles* vf = pp->visitedFiles;
                item = &pp->stack.back();

                // Add new locationMap range
                PPVisitedFiles::LocationMapTraits::Item locMapItem;
                locMapItem.linearLoc = pp->linearLocAtEndOfStackTop;
                locMapItem.includeChainIdx = item->includeChainIdx;
                locMapItem.offset = safeDemote<u32>(item->vins.curByte -
                                                    item->vins.getStartByte());
                vf->locationMap.insert(std::move(locMapItem));

                pp->linearLocAtEndOfStackTop += item->vins.numBytesAvailable();
            }
            continue;
        }

        PLY_ASSERT(pp->linearLocAtEndOfStackTop >= 0);
        token.linearLoc = pp->linearLocAtEndOfStackTop - item->vins.numBytesAvailable();
        bool wasAtStartOfLine = pp->atStartOfLine;
        pp->atStartOfLine = false;
        switch (item->vins.peekByte()) {
            case '\n':
            case '\r':
            case '\t':
            case ' ': {
                // Skip whitespace while keeping track of start of line
                pp->atStartOfLine = wasAtStartOfLine;
                while (item->vins.tryMakeBytesAvailable()) {
                    switch (item->vins.peekByte()) {
                        case '\n':
                            pp->atStartOfLine = true;
                        case '\r':
                        case '\t':
                        case ' ':
                            break;
                        default:
                            goto endOfWhite;
                    }
                    item->vins.advanceByte();
                }
            endOfWhite:
                // FIXME: Optionally return a whitespace token
                break;
            }

            case '#': {
                if (wasAtStartOfLine) {
                    // Preprocessor directive
                    readPreprocessorDirective(&item->vins, pp);
                    token.type = Token::Directive;
                    goto gotToken;
                } else {
                    // FIXME: Don't report error here. Return garbage token to caller instead
                    // (like below)
                    pp->error({Preprocessor::Error::DirectiveNotAtStartOfLine,
                               getLinearLocation(pp, item->vins.curByte)});
                    item->vins.advanceByte();
                }
                break;
            }

            case '/': {
                LinearLocation startCommentLoc = getLinearLocation(pp, item->vins.curByte);
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '/') {
                        item->vins.advanceByte();
                        item->vins.parse<fmt::Line>();
                        token.type = Token::LineComment;
                        pp->atStartOfLine = true;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '*') {
                        item->vins.advanceByte();
                        if (fmt::scanUpToAndIncludingSpecial(&item->vins, "*/")) {
                            token.type = Token::CStyleComment;
                            goto gotToken;
                        } else {
                            // EOF in comment
                            pp->error({Preprocessor::Error::EOFInComment,
                                       getLinearLocation(pp, item->vins.curByte),
                                       startCommentLoc});
                        }
                        break;
                    } else if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::SlashEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::ForwardSlash;
                goto gotToken;
            }

            case '{': {
                item->vins.advanceByte();
                token.type = Token::OpenCurly;
                goto gotToken;
            }

            case '}': {
                item->vins.advanceByte();
                token.type = Token::CloseCurly;
                goto gotToken;
            }

            case ';': {
                item->vins.advanceByte();
                token.type = Token::Semicolon;
                goto gotToken;
            }

            case '(': {
                item->vins.advanceByte();
                token.type = Token::OpenParen;
                goto gotToken;
            }

            case ')': {
                item->vins.advanceByte();
                token.type = Token::CloseParen;
                goto gotToken;
            }

            case '<': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '<') {
                        item->vins.advanceByte();
                        token.type = Token::LeftShift;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::LessThanOrEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::OpenAngle;
                goto gotToken;
            }

            case '>': {
                item->vins.advanceByte();
                // FIXME: Disable tokenizeCloseAnglesOnly inside parenthesized expressions
                if (!pp->tokenizeCloseAnglesOnly && item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '>') {
                        item->vins.advanceByte();
                        token.type = Token::RightShift;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::GreaterThanOrEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::CloseAngle;
                goto gotToken;
            }

            case '[': {
                item->vins.advanceByte();
                token.type = Token::OpenSquare;
                goto gotToken;
            }

            case ']': {
                item->vins.advanceByte();
                token.type = Token::CloseSquare;
                goto gotToken;
            }

            case ':': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == ':') {
                        item->vins.advanceByte();
                        token.type = Token::DoubleColon;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleColon;
                goto gotToken;
            }

            case ',': {
                item->vins.advanceByte();
                token.type = Token::Comma;
                goto gotToken;
            }

            case '?': {
                item->vins.advanceByte();
                token.type = Token::QuestionMark;
                goto gotToken;
            }

            case '=': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::DoubleEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleEqual;
                goto gotToken;
            }

            case '*': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::StarEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::Star;
                goto gotToken;
            }

            case '%': {
                item->vins.advanceByte();
                token.type = Token::Percent;
                goto gotToken;
            }

            case '&': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '&') {
                        item->vins.advanceByte();
                        token.type = Token::DoubleAmpersand;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleAmpersand;
                goto gotToken;
            }

            case '|': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '|') {
                        item->vins.advanceByte();
                        token.type = Token::DoubleVerticalBar;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleVerticalBar;
                goto gotToken;
            }

            case '+': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '+') {
                        item->vins.advanceByte();
                        token.type = Token::DoublePlus;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::PlusEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::SinglePlus;
                goto gotToken;
            }

            case '-': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '-') {
                        item->vins.advanceByte();
                        token.type = Token::DoubleMinus;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::MinusEqual;
                        goto gotToken;
                    } else if (item->vins.peekByte() == '>') {
                        item->vins.advanceByte();
                        token.type = Token::Arrow;
                        goto gotToken;
                    }
                }
                token.type = Token::SingleMinus;
                goto gotToken;
            }

            case '.': {
                item->vins.advanceByte();
                if (item->vins.numBytesAvailable() >= 2) {
                    if (item->vins.peekByte(0) == '.' &&
                        item->vins.peekByte(1) == '.') {
                        item->vins.advanceByte(2);
                        token.type = Token::Ellipsis;
                        goto gotToken;
                    }
                }
                token.type = Token::Dot;
                goto gotToken;
            }

            case '~': {
                item->vins.advanceByte();
                token.type = Token::Tilde;
                goto gotToken;
            }

            case '^': {
                item->vins.advanceByte();
                token.type = Token::Caret;
                goto gotToken;
            }

            case '!': {
                item->vins.advanceByte();
                if (item->vins.tryMakeBytesAvailable()) {
                    if (item->vins.peekByte() == '=') {
                        item->vins.advanceByte();
                        token.type = Token::NotEqual;
                        goto gotToken;
                    }
                }
                token.type = Token::Bang;
                goto gotToken;
            }

            case '\'':
            case '"': {
                LinearLocation beginStringLoc = getLinearLocation(pp, item->vins.curByte);
                char c = item->vins.peekByte();
                item->vins.advanceByte();
                if (!readStringLiteral(&item->vins, pp, c, beginStringLoc))
                    break;
                token.type = Token::StringLiteral;
                goto gotToken;
            }

            default: {
                LinearLocation beginTokenLoc = getLinearLocation(pp, item->vins.curByte);
                token.type = readIdentifierOrLiteral(&item->vins, pp, beginTokenLoc);
                if (token.type != Token::Identifier) {
                    if (token.type == Token::Invalid)
                        break;
                    goto gotToken;
                }

                token.identifier = item->vins.getViewFrom(savePoint);
                PLY_ASSERT(token.identifier);
                auto cursor = pp->macros.find(token.identifier);
                if (cursor.wasFound()) {
                    token.type = Token::Macro;
                    token.identifier = item->vins.getViewFrom(savePoint);

                    // This is a macro expansion
                    LinearLocation linearLocAtMacro =
                        pp->linearLocAtEndOfStackTop -
                        safeDemote<LinearLocation>(item->vins.endByte -
                                                   token.identifier.bytes);
                    PPVisitedFiles* vf = pp->visitedFiles;

                    const PPVisitedFiles::MacroExpansion& exp =
                        vf->macroExpansions[cursor->expansionIdx];
                    pp->macroArgs.clear();
                    if (exp.takesArgs) {
                        // This macro expects arguments
                        auto savePoint = item->vins.savePoint();
                        pp->macroArgs = readMacroArguments(pp, &item->vins);
                        if (!pp->macroArgs) {
                            // No arguments were provided, so just return a plain token
                            item->vins.restore(savePoint);
                            token.type = Token::Identifier;
                            goto gotToken;
                        }
                    }

                    // FIXME: Implement special case for empty macros? (Would eliminate a
                    // locationMap entry).

                    // Add a new stack entry for the macro expansion
                    u32 prevChainIdx = item->includeChainIdx;
                    u32 includeChainIdx = vf->includeChains.numItems();
                    PPVisitedFiles::IncludeChain& chain = vf->includeChains.append();
                    chain.isMacroExpansion = true;
                    chain.fileOrExpIdx = cursor->expansionIdx;
                    chain.parentIdx = prevChainIdx;

                    item = &pp->stack.append();
                    item->includeChainIdx = includeChainIdx;
                    item->vins = ViewInStream{vf->getContents(includeChainIdx)};
                    pp->linearLocAtEndOfStackTop =
                        linearLocAtMacro + item->vins.numBytesAvailable();

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
        token.identifier = item->vins.getViewFrom(savePoint);
        PLY_ASSERT(token.type != Token::Invalid);
        return token;
    }

    token.type = Token::EndOfFile;
    return token;
}

} // namespace cpp
} // namespace ply

#include "codegen/Preprocessor.inl" //%%
