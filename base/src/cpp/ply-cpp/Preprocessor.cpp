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

PLY_NO_INLINE void add_ppdef(Preprocessor* pp, StringView identifier,
                             StringView expansion, bool takes_args) {
    PPVisitedFiles* vf = pp->visited_files;

    u32 exp_idx = vf->macro_expansions.num_items();
    PPVisitedFiles::MacroExpansion& exp = vf->macro_expansions.append();
    exp.set_string(expansion);
    exp.takes_args = takes_args;

    pp->macros.assign(identifier, exp_idx);
}

PLY_INLINE LinearLocation get_linear_location(const Preprocessor* pp,
                                              const char* cur_byte) {
    const Preprocessor::StackItem& item = pp->stack.back();
    PLY_ASSERT(cur_byte >= item.in.start_byte);
    PLY_ASSERT(cur_byte <= item.in.end_byte);
    return pp->linear_loc_at_end_of_stack_top - (item.in.end_byte - cur_byte);
}

void Preprocessor::Error::write_message(OutStream& out,
                                        const PPVisitedFiles* visited_files) const {
    out.format("{}: error: ",
               expand_file_location(visited_files, this->linear_loc).to_string());
    switch (this->type) {
        case Preprocessor::Error::InvalidDirective: {
            out << "invalid preprocessing directive\n";
            break;
        }
        case Preprocessor::Error::EOFInMacro: {
            out << "unexpected end-of-file in preprocessor macro parameter list\n";
            out.format(
                "{}: note: parameter list started here\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case Preprocessor::Error::EOFInComment: {
            out << "unexpected end-of-file in C-style comment\n";
            out.format(
                "{}: note: comment started here\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case Preprocessor::Error::EOFInStringLiteral: {
            out << "unexpected end-of-file in string literal\n";
            out.format(
                "{}: note: string literal started here\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case Preprocessor::Error::DirectiveNotAtStartOfLine: {
            out << "preprocessing directives must begin at start of line\n";
            break;
        }
        case Preprocessor::Error::EOFInRawStringDelimiter: {
            out << "unexpected end-of-file in raw string delimiter\n";
            out.format(
                "{}: note: delimiter started here\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case Preprocessor::Error::InvalidCharInRawStringDelimiter: {
            out << "invalid character in raw string delimiter\n";
            out.format(
                "{}: note: delimiter started here\n",
                expand_file_location(visited_files, this->other_loc).to_string());
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
    if (this->from_file) {
        subst::destruct_by_member(&this->range);
    } else {
        subst::destruct_by_member(&this->str);
    }
}

PLY_NO_INLINE StringView PPVisitedFiles::get_contents(u32 include_chain_index) const {
    const IncludeChain& chain = this->include_chains[include_chain_index];
    if (chain.is_macro_expansion) {
        const MacroExpansion& exp = this->macro_expansions[chain.file_or_exp_idx];
        if (exp.from_file) {
            const SourceFile& file = this->source_files[exp.from_file];
            return file.contents.view().sub_str(
                exp.range.start_ofs, exp.range.end_ofs - exp.range.start_ofs);
        } else {
            return exp.str;
        }
    } else {
        const SourceFile& file = this->source_files[chain.file_or_exp_idx];
        return file.contents;
    }
}

void read_numeric_literal(ViewInStream& in) {
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

void read_preprocessor_directive(ViewInStream& in, Preprocessor* pp) {
    bool avail = in.ensure_readable();
    PLY_ASSERT(avail);
    PLY_UNUSED(avail);
    PLY_ASSERT(*in.cur_byte == '#');
    auto save_point = in.get_save_point();
    in.cur_byte++;

    // Skip whitespace
    while (in.ensure_readable()) {
        char c = *in.cur_byte;
        if (!is_white(c))
            break;
        in.cur_byte++;
        if (c == '\n')
            return; // null directive
    }

    // Read directive
    StringView directive = in.read_view<fmt::Identifier>(fmt::WithDollarSign);
    if (directive.is_empty()) {
        // invalid characters after #
        pp->error({Preprocessor::Error::InvalidDirective,
                   get_linear_location(pp, in.cur_byte)});
        in.parse<fmt::Line>();
    } else {
        // Handle directive
        if (directive == "include") {
            in.parse<fmt::Line>();
            if (pp->include_callback) {
                pp->include_callback(in.get_view_from(save_point));
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
                       get_linear_location(pp, directive.bytes)});
            in.parse<fmt::Line>();
        }
    }
    pp->at_start_of_line = true;
}

// Returns true if arguments were encountered, which means the macro can be expanded
// Returns false if arguments were not encountered, which means the macro cannot be
// expanded and the identifier should be returned as a token.
Array<Token> read_macro_arguments(Preprocessor* pp, ViewInStream& in) {
    Array<Token> macro_args;

    // Skip whitespace
    in.parse<fmt::Whitespace>();

    // Look for opening parenthesis
    if (!(in.ensure_readable() && *in.cur_byte == '('))
        return macro_args;
    LinearLocation open_paren_loc = get_linear_location(pp, in.cur_byte);
    in.cur_byte++;
    const char* arg_start = (const char*) in.cur_byte;
    LinearLocation arg_start_loc = open_paren_loc + 1;

    // Read up to closing parenthesis
    s32 nest_level = 1;
    for (;;) {
        if (!in.ensure_readable()) {
            // end of file in macro arguments
            pp->error({Preprocessor::Error::EOFInMacro,
                       get_linear_location(pp, in.cur_byte), open_paren_loc});
            return macro_args;
        }

        char c = *in.cur_byte;
        in.cur_byte++;
        // FIXME: Detect strings here
        switch (c) {
            case '/': {
                if (!in.ensure_readable()) {
                    // end of file in macro arguments
                    pp->error({Preprocessor::Error::EOFInMacro,
                               get_linear_location(pp, in.cur_byte), open_paren_loc});
                    return macro_args;
                }
                if (*in.cur_byte == '/') {
                    in.cur_byte++;
                    in.parse<fmt::Line>();
                } else if (*in.cur_byte == '*') {
                    in.cur_byte++;
                    if (!fmt::scan_up_to_and_including_special(in, "*/")) {
                        // EOF in comment
                        pp->error({Preprocessor::Error::EOFInComment,
                                   get_linear_location(pp, in.cur_byte),
                                   open_paren_loc});
                        return macro_args;
                    }
                }
                break;
            }

            case '(': {
                nest_level++;
                break;
            }

            case ')': {
                nest_level--;
                if (nest_level <= 0) {
                    macro_args.append({arg_start_loc, Token::MacroArgument,
                                       StringView::from_range(
                                           arg_start, (const char*) in.cur_byte - 1)});
                    return macro_args;
                }
                break;
            }

            case ',': {
                if (nest_level == 1) {
                    macro_args.append({arg_start_loc, Token::MacroArgument,
                                       StringView::from_range(
                                           arg_start, (const char*) in.cur_byte - 1)});
                    arg_start = (const char*) in.cur_byte;
                    arg_start_loc = get_linear_location(pp, in.cur_byte);
                }
            }

            default:
                break;
        }
    }
}

bool read_string_literal(ViewInStream& in, Preprocessor* pp, char quote_punc,
                         LinearLocation begin_string_loc) {
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       get_linear_location(pp, in.cur_byte), begin_string_loc});
            return false;
        }
        char c = *in.cur_byte;
        in.cur_byte++;
        if (c == '\\') {
            if (!in.ensure_readable()) {
                // End of file in string literal
                pp->error({Preprocessor::Error::EOFInStringLiteral,
                           get_linear_location(pp, in.cur_byte), begin_string_loc});
                return false;
            }
            in.cur_byte++;
        } else if (c == quote_punc) {
            return true;
        }
    }
}

bool read_delimiter_and_raw_string_literal(ViewInStream& in, Preprocessor* pp,
                                           LinearLocation begin_string_loc) {
    PLY_ASSERT(*in.cur_byte == '"');
    in.cur_byte++;

    // read delimiter
    const char* delimiter_start = in.cur_byte;
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file while reading raw string delimiter
            pp->error({Preprocessor::Error::EOFInRawStringDelimiter,
                       get_linear_location(pp, in.cur_byte), begin_string_loc});
            return false;
        }
        char c = *in.cur_byte;
        if (c == '(')
            break;
        // FIXME: Recognize more whitespace characters
        if (is_white(c) || c == ')' || c == '\\') {
            // Invalid character in delimiter
            pp->error({Preprocessor::Error::InvalidCharInRawStringDelimiter,
                       get_linear_location(pp, in.cur_byte), begin_string_loc});
            return false;
        }
        in.cur_byte++;
    }

    // FIXME: Enforce maximum length of delimiter (at most 16 characters)
    const char* delimiter_end = in.cur_byte;
    in.cur_byte++;

    // Read remainder of string
    for (;;) {
        if (!in.ensure_readable()) {
            // End of file in string literal
            pp->error({Preprocessor::Error::EOFInStringLiteral,
                       get_linear_location(pp, in.cur_byte), begin_string_loc});
            return false;
        }
        char c = *in.cur_byte;
        in.cur_byte++;
        if (c == ')') {
            // Try to match delimiter
            const char* d = delimiter_start;
            for (;;) {
                if (d == delimiter_end) {
                    if (!in.ensure_readable()) {
                        // End of file while matching closing "
                        pp->error({Preprocessor::Error::EOFInStringLiteral,
                                   get_linear_location(pp, in.cur_byte),
                                   begin_string_loc});
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
                               get_linear_location(pp, in.cur_byte), begin_string_loc});
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
    u32 bit_value = mask[c >> 5] & (1 << (c & 31));
    return (bit_value != 0);
}

Token::Type read_identifier_or_literal(ViewInStream& in, Preprocessor* pp,
                                       LinearLocation begin_token_loc) {
    char c = *in.cur_byte;
    if (c >= '0' && c <= '9') {
        read_numeric_literal(in);
        return Token::NumericLiteral;
    }

    // Copied from fmt::FormatParser<fmt::Identifier>::parse:
    u32 mask[8] = {0,          0,          0x87fffffe, 0x7fffffe,
                   0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    mask[1] |= 0x10; // '$'
    mask[1] |=
        0x3ff0000; // accept digits (we already know the first character is non-digit)

    const char* start_byte = in.cur_byte;
    for (;;) {
        if (!in.ensure_readable()) {
            PLY_ASSERT(in.cur_byte != start_byte);
            return Token::Identifier;
        }
        c = *in.cur_byte;
        if (!match(c, mask)) {
            if (c == '"') {
                if (in.cur_byte == start_byte + 1 && *start_byte == 'R') {
                    if (read_delimiter_and_raw_string_literal(in, pp,
                                                              begin_token_loc)) {
                        return Token::StringLiteral;
                    }
                } else {
                    // Treat it as a string prefix
                    in.cur_byte++;
                    if (!read_string_literal(in, pp, c, begin_token_loc))
                        return Token::Invalid;
                    return Token::StringLiteral;
                }
            } else {
                if (start_byte == in.cur_byte) {
                    // Garbage token
                    // FIXME: Should we return 't report error here, but return garbage
                    // token to caller instead, so that parser can decide how to recover
                    // from it
                    in.cur_byte++;
                    pp->error(
                        {Preprocessor::Error::GarbageCharacters, begin_token_loc});
                    return Token::Invalid;
                } else {
                    return Token::Identifier;
                }
            }
        }
        in.cur_byte++;
    }
}

PLY_NO_INLINE Token read_token(Preprocessor* pp) {
    Preprocessor::StackItem* item = nullptr;
    Token token;
    while (pp->stack.num_items() > 0) {
        item = &pp->stack.back();
        auto save_point = item->in.get_save_point();
        if (!item->in.ensure_readable()) {
            pp->stack.pop();
            if (pp->stack.num_items() > 0) {
                PPVisitedFiles* vf = pp->visited_files;
                item = &pp->stack.back();

                // Add new location_map range
                PPVisitedFiles::LocationMapTraits::Item loc_map_item;
                loc_map_item.linear_loc = pp->linear_loc_at_end_of_stack_top;
                loc_map_item.include_chain_idx = item->include_chain_idx;
                loc_map_item.offset =
                    check_cast<u32>(item->in.cur_byte - item->in.start_byte);
                vf->location_map.insert(std::move(loc_map_item));

                pp->linear_loc_at_end_of_stack_top += item->in.num_bytes_readable();
            }
            continue;
        }

        PLY_ASSERT(pp->linear_loc_at_end_of_stack_top >= 0);
        token.linear_loc =
            pp->linear_loc_at_end_of_stack_top - item->in.num_bytes_readable();
        bool was_at_start_of_line = pp->at_start_of_line;
        pp->at_start_of_line = false;
        switch (*item->in.cur_byte) {
            case '\n':
            case '\r':
            case '\t':
            case ' ': {
                // Skip whitespace while keeping track of start of line
                pp->at_start_of_line = was_at_start_of_line;
                while (item->in.ensure_readable()) {
                    switch (*item->in.cur_byte) {
                        case '\n':
                            pp->at_start_of_line = true;
                        case '\r':
                        case '\t':
                        case ' ':
                            break;
                        default:
                            goto end_of_white;
                    }
                    item->in.cur_byte++;
                }
            end_of_white:
                // FIXME: Optionally return a whitespace token
                break;
            }

            case '#': {
                if (was_at_start_of_line) {
                    // Preprocessor directive
                    read_preprocessor_directive(item->in, pp);
                    token.type = Token::Directive;
                    goto got_token;
                } else {
                    // FIXME: Don't report error here. Return garbage token to caller
                    // instead (like below)
                    pp->error({Preprocessor::Error::DirectiveNotAtStartOfLine,
                               get_linear_location(pp, item->in.cur_byte)});
                    item->in.cur_byte++;
                }
                break;
            }

            case '/': {
                LinearLocation start_comment_loc =
                    get_linear_location(pp, item->in.cur_byte);
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '/') {
                        item->in.cur_byte++;
                        item->in.parse<fmt::Line>();
                        token.type = Token::LineComment;
                        pp->at_start_of_line = true;
                        goto got_token;
                    } else if (*item->in.cur_byte == '*') {
                        item->in.cur_byte++;
                        if (fmt::scan_up_to_and_including_special(item->in, "*/")) {
                            token.type = Token::CStyleComment;
                            goto got_token;
                        } else {
                            // EOF in comment
                            pp->error({Preprocessor::Error::EOFInComment,
                                       get_linear_location(pp, item->in.cur_byte),
                                       start_comment_loc});
                        }
                        break;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::SlashEqual;
                        goto got_token;
                    }
                }
                token.type = Token::ForwardSlash;
                goto got_token;
            }

            case '{': {
                item->in.cur_byte++;
                token.type = Token::OpenCurly;
                goto got_token;
            }

            case '}': {
                item->in.cur_byte++;
                token.type = Token::CloseCurly;
                goto got_token;
            }

            case ';': {
                item->in.cur_byte++;
                token.type = Token::Semicolon;
                goto got_token;
            }

            case '(': {
                item->in.cur_byte++;
                token.type = Token::OpenParen;
                goto got_token;
            }

            case ')': {
                item->in.cur_byte++;
                token.type = Token::CloseParen;
                goto got_token;
            }

            case '<': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '<') {
                        item->in.cur_byte++;
                        token.type = Token::LeftShift;
                        goto got_token;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::LessThanOrEqual;
                        goto got_token;
                    }
                }
                token.type = Token::OpenAngle;
                goto got_token;
            }

            case '>': {
                item->in.cur_byte++;
                // FIXME: Disable tokenize_close_angles_only inside parenthesized
                // expressions
                if (!pp->tokenize_close_angles_only && item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '>') {
                        item->in.cur_byte++;
                        token.type = Token::RightShift;
                        goto got_token;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::GreaterThanOrEqual;
                        goto got_token;
                    }
                }
                token.type = Token::CloseAngle;
                goto got_token;
            }

            case '[': {
                item->in.cur_byte++;
                token.type = Token::OpenSquare;
                goto got_token;
            }

            case ']': {
                item->in.cur_byte++;
                token.type = Token::CloseSquare;
                goto got_token;
            }

            case ':': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == ':') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleColon;
                        goto got_token;
                    }
                }
                token.type = Token::SingleColon;
                goto got_token;
            }

            case ',': {
                item->in.cur_byte++;
                token.type = Token::Comma;
                goto got_token;
            }

            case '?': {
                item->in.cur_byte++;
                token.type = Token::QuestionMark;
                goto got_token;
            }

            case '=': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleEqual;
                        goto got_token;
                    }
                }
                token.type = Token::SingleEqual;
                goto got_token;
            }

            case '*': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::StarEqual;
                        goto got_token;
                    }
                }
                token.type = Token::Star;
                goto got_token;
            }

            case '%': {
                item->in.cur_byte++;
                token.type = Token::Percent;
                goto got_token;
            }

            case '&': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '&') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleAmpersand;
                        goto got_token;
                    }
                }
                token.type = Token::SingleAmpersand;
                goto got_token;
            }

            case '|': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '|') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleVerticalBar;
                        goto got_token;
                    }
                }
                token.type = Token::SingleVerticalBar;
                goto got_token;
            }

            case '+': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '+') {
                        item->in.cur_byte++;
                        token.type = Token::DoublePlus;
                        goto got_token;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::PlusEqual;
                        goto got_token;
                    }
                }
                token.type = Token::SinglePlus;
                goto got_token;
            }

            case '-': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '-') {
                        item->in.cur_byte++;
                        token.type = Token::DoubleMinus;
                        goto got_token;
                    } else if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::MinusEqual;
                        goto got_token;
                    } else if (*item->in.cur_byte == '>') {
                        item->in.cur_byte++;
                        token.type = Token::Arrow;
                        goto got_token;
                    }
                }
                token.type = Token::SingleMinus;
                goto got_token;
            }

            case '.': {
                item->in.cur_byte++;
                if (item->in.num_bytes_readable() >= 2) {
                    if (item->in.cur_byte[0] == '.' && item->in.cur_byte[1] == '.') {
                        item->in.cur_byte += 2;
                        token.type = Token::Ellipsis;
                        goto got_token;
                    }
                }
                token.type = Token::Dot;
                goto got_token;
            }

            case '~': {
                item->in.cur_byte++;
                token.type = Token::Tilde;
                goto got_token;
            }

            case '^': {
                item->in.cur_byte++;
                token.type = Token::Caret;
                goto got_token;
            }

            case '!': {
                item->in.cur_byte++;
                if (item->in.ensure_readable()) {
                    if (*item->in.cur_byte == '=') {
                        item->in.cur_byte++;
                        token.type = Token::NotEqual;
                        goto got_token;
                    }
                }
                token.type = Token::Bang;
                goto got_token;
            }

            case '\'':
            case '"': {
                LinearLocation begin_string_loc =
                    get_linear_location(pp, item->in.cur_byte);
                char c = *item->in.cur_byte;
                item->in.cur_byte++;
                if (!read_string_literal(item->in, pp, c, begin_string_loc))
                    break;
                token.type = Token::StringLiteral;
                goto got_token;
            }

            default: {
                LinearLocation begin_token_loc =
                    get_linear_location(pp, item->in.cur_byte);
                token.type = read_identifier_or_literal(item->in, pp, begin_token_loc);
                if (token.type != Token::Identifier) {
                    if (token.type == Token::Invalid)
                        break;
                    goto got_token;
                }

                token.identifier = item->in.get_view_from(save_point);
                PLY_ASSERT(token.identifier);
                if (u32* expansion_idx = pp->macros.find(token.identifier)) {
                    token.type = Token::Macro;
                    token.identifier = item->in.get_view_from(save_point);

                    // This is a macro expansion
                    LinearLocation linear_loc_at_macro =
                        pp->linear_loc_at_end_of_stack_top -
                        check_cast<LinearLocation>(item->in.end_byte -
                                                    token.identifier.bytes);
                    PPVisitedFiles* vf = pp->visited_files;

                    const PPVisitedFiles::MacroExpansion& exp =
                        vf->macro_expansions[*expansion_idx];
                    pp->macro_args.clear();
                    if (exp.takes_args) {
                        // This macro expects arguments
                        auto save_point = item->in.get_save_point();
                        pp->macro_args = read_macro_arguments(pp, item->in);
                        if (!pp->macro_args) {
                            // No arguments were provided, so just return a plain token
                            item->in.rewind(save_point);
                            token.type = Token::Identifier;
                            goto got_token;
                        }
                    }

                    // FIXME: Implement special case for empty macros? (Would eliminate
                    // a location_map entry).

                    // Add a new stack entry for the macro expansion
                    u32 prev_chain_idx = item->include_chain_idx;
                    u32 include_chain_idx = vf->include_chains.num_items();
                    PPVisitedFiles::IncludeChain& chain = vf->include_chains.append();
                    chain.is_macro_expansion = true;
                    chain.file_or_exp_idx = *expansion_idx;
                    chain.parent_idx = prev_chain_idx;

                    item = &pp->stack.append();
                    item->in = ViewInStream{vf->get_contents(include_chain_idx)};
                    item->include_chain_idx = include_chain_idx;
                    pp->linear_loc_at_end_of_stack_top =
                        linear_loc_at_macro + item->in.num_bytes_readable();

                    // Add new location_map range
                    PPVisitedFiles::LocationMapTraits::Item loc_map_item;
                    loc_map_item.linear_loc = linear_loc_at_macro;
                    loc_map_item.include_chain_idx = include_chain_idx;
                    loc_map_item.offset = 0;
                    vf->location_map.insert(std::move(loc_map_item));

                    return token;
                }

                token.type = Token::Identifier;
                goto got_token;
            }
        }
        continue;

    got_token:
        token.identifier = item->in.get_view_from(save_point);
        PLY_ASSERT(token.type != Token::Invalid);
        return token;
    }

    token.type = Token::EndOfFile;
    return token;
}

} // namespace cpp
} // namespace ply

#include "codegen/Preprocessor.inl" //%%
