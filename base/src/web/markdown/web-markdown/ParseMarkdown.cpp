/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>
#include <ply-runtime/string/TextEncoding.h>

namespace ply {
namespace markdown {

// A helper class that keeps track of the indentation level while consuming input.
struct LineConsumer {
    ViewInStream vins;
    u32 outer_indent = 0;
    u32 indent = 0;

    PLY_NO_INLINE bool consume_space_or_tab() {
        if (this->vins.num_bytes_available() == 0)
            return false;
        char c = *this->vins.cur_byte;
        if (c == ' ') {
            this->indent++;
        } else if (c == '\t') {
            u32 tab_size = 4;
            this->indent += tab_size - (this->indent % tab_size);
        } else {
            return false;
        }
        this->vins.advance_byte();
        return true;
    }

    PLY_INLINE u32 inner_indent() const {
        return this->indent - this->outer_indent;
    }

    PLY_INLINE StringView trimmed_remainder() const {
        return this->vins.view_available().trim(is_white);
    }

    PLY_INLINE LineConsumer(StringView line) : vins{line} {
    }
};

//-----------------------------------------------------------------
// Code to parse block elements (first pass)
//-----------------------------------------------------------------
String extract_code_line(StringView line, u32 from_indent) {
    u32 indent = 0;
    for (u32 i = 0; i < line.num_bytes; i++) {
        if (indent == from_indent) {
            return line.sub_str(i);
        }
        u8 c = line[i];
        PLY_ASSERT(c < 128);              // No high code points
        PLY_ASSERT(c >= 32 || c == '\t'); // No control characters
        if (c == '\t') {
            u32 tab_size = 4;
            u32 new_indent = indent + tab_size - (indent % tab_size);
            if (new_indent > from_indent) {
                return StringView{" "} * (new_indent - from_indent) +
                       line.sub_str(i + 1);
            }
            indent = new_indent;
        } else {
            indent++;
        }
    }
    PLY_ASSERT(0);
    return {};
}

struct Parser {
    // block_nodes and leaf_node, together, represent the stack of elements for the
    // current line. block_nodes[0] holds the root Node::Document. block_nodes[1 and
    // greater] can only hold a Node::BlockQuote or Node::ListItem. leaf_node represents
    // the top of the stack where text goes. It can either hold a Node::Paragraph or
    // Node::CodeBlock.
    Array<Node*> block_nodes;
    Node* leaf_node = nullptr;

    // Only used if leaf_node is CodeBlock:
    u32 num_blank_lines_in_code_block = 0;

    // This flag indicates that some Lists on the stack have their is_loose_if_continued
    // flag set: (Alternatively, we *could* store the number of such Lists on the stack,
    // and eliminate the is_loose_if_continued flag completely, but it would complicate
    // match_existing_indentation a little bit. Sticking with this approach for now.)
    bool check_list_continuations = false;

    // This is called at the start of each line. It figures out which of the existing
    // elements we are still inside by consuming indentation and blockquote '>' markers
    // that match the current element stack, then truncates any unmatched BlockQuote
    // nodes from the stack. (Unmatched ListItem nodes are not truncated here because
    // list items are allowed to contain blank lines.) Returns true if there is more
    // text on the current line.
    PLY_NO_INLINE bool match_existing_indentation(StringView line, LineConsumer& lc) {
        u32 keep_stack_depth = 1;
        for (;;) {
            while (lc.consume_space_or_tab()) {
            }
            if (keep_stack_depth >= this->block_nodes.num_items())
                break;
            Node* node = this->block_nodes[keep_stack_depth];
            if (node->type == Node::BlockQuote) {
                if (lc.vins.num_bytes_available() > 0 && *lc.vins.cur_byte == '>' &&
                    lc.inner_indent() <= 3) {
                    // Continue the current blockquote
                    lc.vins.advance_byte();
                    lc.indent++;
                    lc.outer_indent = lc.indent;
                    keep_stack_depth++;
                    // Consume optional space and include it in outer_indent:
                    if (lc.consume_space_or_tab()) {
                        lc.outer_indent++;
                    }
                    continue;
                }
            } else if (node->type == Node::ListItem) {
                if (lc.inner_indent() >= node->indent_or_level) {
                    // Continue the current list item
                    keep_stack_depth++;
                    lc.outer_indent += node->indent_or_level;
                    continue;
                }
            } else {
                // block_nodes indices >= 1 should only hold BlockQuote and ListItem
                PLY_ASSERT(0);
            }
            break;
        }

        // Is remainder of line blank?
        if (lc.trimmed_remainder().is_empty()) {
            // Yes. Terminate paragraph if any
            if (this->leaf_node && this->leaf_node->type == Node::Paragraph) {
                this->leaf_node = nullptr;
                PLY_ASSERT(this->num_blank_lines_in_code_block == 0);
            }
            // Truncate non-continued blockquotes
            while (keep_stack_depth < this->block_nodes.num_items() &&
                   this->block_nodes[keep_stack_depth]->type == Node::ListItem) {
                keep_stack_depth++;
            }
            PLY_ASSERT(keep_stack_depth >= this->block_nodes.num_items() ||
                       this->block_nodes[keep_stack_depth]->type == Node::BlockQuote);
            if (keep_stack_depth < this->block_nodes.num_items()) {
                this->block_nodes.resize(keep_stack_depth);
                this->leaf_node = nullptr;
                this->num_blank_lines_in_code_block = 0;
            }
            if (this->leaf_node) {
                // At this point, the only possible leaf node is a CodeBlock, because
                // Paragraphs are terminated above, and Headings don't persist across
                // lines.
                PLY_ASSERT(this->leaf_node->type == Node::CodeBlock);
                // Count blank lines in CodeBlocks
                if (lc.indent - lc.outer_indent > 4) {
                    // Add intermediate blank lines
                    // FIXME: Could this be unified with the code below? (Code
                    // simplification)
                    for (u32 i = 0; i < this->num_blank_lines_in_code_block; i++) {
                        this->leaf_node->raw_lines.append("\n");
                    }
                    this->num_blank_lines_in_code_block = 0;
                    String code_line = extract_code_line(line, lc.outer_indent + 4);
                    this->leaf_node->raw_lines.append(std::move(code_line));
                } else {
                    this->num_blank_lines_in_code_block++;
                }
            } else {
                // There's no leaf node and the remainder of the line is blank.
                // Walk the stack and set the "isLooseIfContinued" flag on all Lists.
                for (Node* node : this->block_nodes) {
                    if (node->type == Node::ListItem) {
                        PLY_ASSERT(node->parent->type == Node::List);
                        if (!node->parent->is_loose) {
                            node->parent->is_loose_if_continued = true;
                            this->check_list_continuations = true;
                        }
                    }
                }
            }
            return false;
        }

        // No. There's more text on the current line
        if (keep_stack_depth < this->block_nodes.num_items()) {
            this->block_nodes.resize(keep_stack_depth);
            this->leaf_node = nullptr;
            this->num_blank_lines_in_code_block = 0;
        }
        return true;
    }

    // This function consumes new blockquote '>' markers and list item markers such as
    // '*' that *don't* match existing block elements on the current stack. It creates
    // new block elements for each marker encountered.
    void parse_new_markers(LineConsumer& lc) {
        PLY_ASSERT(!lc.trimmed_remainder()
                        .is_empty()); // Not called if remainder of line is blank

        // Attempt to parse new Node markers
        while (lc.vins.num_bytes_available() > 0) {
            if (lc.inner_indent() >= 4)
                break;

            auto save_point = lc.vins.save_point();
            u32 saved_indent = lc.indent;

            // This code block will handle any list markers encountered:
            auto got_list_marker = [&](s32 marker_number, char punc) {
                bool is_ordered = (marker_number >= 0);
                this->leaf_node = nullptr;
                this->num_blank_lines_in_code_block = 0;
                Node* list_node = nullptr;
                Node* parent_ctr = this->block_nodes.back();
                PLY_ASSERT(parent_ctr->is_container_block());
                if (!parent_ctr->children.is_empty()) {
                    Node* potential_parent = parent_ctr->children.back();
                    if (potential_parent->type == Node::List &&
                        potential_parent->is_ordered_list() == is_ordered &&
                        potential_parent->list_punc == punc) {
                        // Add item to existing list
                        list_node = potential_parent;
                    }
                } else if (parent_ctr->type == Node::ListItem) {
                    // Begin new list as a sublist of existing list
                    parent_ctr = parent_ctr->parent;
                    PLY_ASSERT(parent_ctr->type == Node::List);
                }
                if (!list_node) {
                    // Begin new list
                    list_node = new Node{parent_ctr, Node::List};
                    list_node->list_start_number = marker_number;
                    list_node->list_punc = punc;
                }
                Node* list_item = new Node{list_node, Node::ListItem};
                list_item->indent_or_level = lc.outer_indent;
                this->block_nodes.append(list_item);
            };

            char c = *lc.vins.cur_byte;
            PLY_ASSERT(!is_white(c));
            if (c == '>') {
                // Begin a new blockquote
                this->block_nodes.append(
                    new Node{this->block_nodes.back(), Node::BlockQuote});
                // Consume optional space after '>'
                lc.vins.advance_byte();
                lc.indent++;
                lc.outer_indent = lc.indent;
                if (lc.consume_space_or_tab()) {
                    lc.outer_indent++;
                }
            } else if (c == '*' || c == '-' || c == '+') {
                lc.vins.advance_byte();
                lc.indent++;
                u32 indent_after_star = lc.indent;
                if (!lc.consume_space_or_tab())
                    goto not_marker;
                if (this->leaf_node && lc.trimmed_remainder().is_empty()) {
                    // If the list item interrupts a paragraph, it must not begin with a
                    // blank line.
                    goto not_marker;
                }

                // It's an unordered list item.
                lc.outer_indent = indent_after_star + 1;
                got_list_marker(-1, c);
            } else if (is_decimal_digit(c)) {
                u64 num = lc.vins.parse<u64>();
                if (this->leaf_node && num != 1) {
                    // If list item interrupts a paragraph, the start number must be 1.
                    goto not_marker;
                }
                uptr marker_length = (lc.vins.cur_byte - save_point.start_byte);
                if (marker_length > 9)
                    goto not_marker; // marker too long
                lc.indent += check_cast<u32>(marker_length);
                if (lc.vins.num_bytes_available() < 2)
                    goto not_marker;
                char punc = *lc.vins.cur_byte;
                // FIXME: support alternate punctuator ')'.
                // If the punctuator doesn't match, it should start a new list.
                if (punc != '.' && punc != ')')
                    goto not_marker;
                lc.vins.advance_byte();
                lc.indent++;
                u32 indent_after_marker = lc.indent;
                if (!lc.consume_space_or_tab())
                    goto not_marker;
                if (this->leaf_node && lc.trimmed_remainder().is_empty()) {
                    // If the list item interrupts a paragraph, it must not begin with a
                    // blank line.
                    goto not_marker;
                }

                // It's an ordered list item.
                // 32-bit demotion is safe because we know the marker is 9 digits or
                // less:
                lc.outer_indent = indent_after_marker + 1;
                got_list_marker(check_cast<s32>(num), punc);
            } else {
                goto not_marker;
            }

            // Consume whitespace
            while (lc.consume_space_or_tab()) {
            }
            continue;

        not_marker:
            lc.vins.restore(save_point);
            lc.indent = saved_indent;
            break;
        }
    }

    PLY_NO_INLINE void parse_paragraph_text(StringView line, LineConsumer& lc) {
        StringView remaining_text = lc.trimmed_remainder();
        bool has_para = this->leaf_node && this->leaf_node->type == Node::Paragraph;
        if (!has_para && lc.inner_indent() >= 4) {
            // Potentially begin or append to code Node
            if (remaining_text && !this->leaf_node) {
                this->leaf_node = new Node{this->block_nodes.back(), Node::CodeBlock};
                PLY_ASSERT(this->num_blank_lines_in_code_block == 0);
            }
            if (this->leaf_node) {
                PLY_ASSERT(this->leaf_node->type == Node::CodeBlock);
                // Add intermediate blank lines
                for (u32 i = 0; i < this->num_blank_lines_in_code_block; i++) {
                    this->leaf_node->raw_lines.append("\n");
                }
                this->num_blank_lines_in_code_block = 0;
                String code_line = extract_code_line(line, lc.outer_indent + 4);
                this->leaf_node->raw_lines.append(std::move(code_line));
            }
        } else {
            if (remaining_text) {
                // We're going to create or extend a leaf node.
                // First, check if any Lists should be marked loose:
                if (this->check_list_continuations) {
                    // Yes, we should mark some (possibly zero) lists loose. It's
                    // impossible for a leaf node to exist at this point:
                    PLY_ASSERT(!this->leaf_node);
                    for (Node* node : this->block_nodes) {
                        if (node->type == Node::ListItem) {
                            PLY_ASSERT(node->parent->type == Node::List);
                            if (node->parent->is_loose_if_continued) {
                                node->parent->is_loose = true;
                                node->parent->is_loose_if_continued = false;
                            }
                        }
                    }
                    this->check_list_continuations = false;
                }

                if (*lc.vins.cur_byte == '#' && lc.inner_indent() <= 3) {
                    // Attempt to parse a heading
                    auto save_point = lc.vins.save_point();
                    StringView pound_seq = lc.vins.read_view(
                        fmt::Callback{[](char c) { return c == '#'; }});
                    StringView space = lc.vins.read_view<fmt::Whitespace>();
                    if (pound_seq.num_bytes <= 6 &&
                        (!space.is_empty() || lc.vins.num_bytes_available() == 0)) {
                        // Got a heading
                        Node* heading_node =
                            new Node{this->block_nodes.back(), Node::Heading};
                        heading_node->indent_or_level = pound_seq.num_bytes;
                        if (StringView remaining_text = lc.trimmed_remainder()) {
                            heading_node->raw_lines.append(remaining_text);
                        }
                        this->leaf_node = nullptr;
                        return;
                    }
                    lc.vins.restore(save_point);
                }
                // If this->leaf_node already exists, it's a lazy paragraph continuation
                if (!has_para) {
                    // Begin new paragraph
                    this->leaf_node =
                        new Node{this->block_nodes.back(), Node::Paragraph};
                    this->num_blank_lines_in_code_block = 0;
                }
                this->leaf_node->raw_lines.append(remaining_text);
            } else {
                PLY_ASSERT(!this->leaf_node); // Should already be cleared by this point
            }
        }
    }

    PLY_NO_INLINE Owned<Node> parse(StringView src) {
        ViewInStream vins{src};
        Owned<Node> document = new Node{nullptr, Node::Document};

        // Initialize stack
        this->block_nodes = {document.get()};
        this->leaf_node = nullptr;
        this->num_blank_lines_in_code_block = 0;

        while (StringView line = vins.read_view<fmt::Line>()) {
            LineConsumer lc{line};
            if (!match_existing_indentation(line, lc))
                continue; // Remainder of line is blank

            parse_new_markers(lc);
            parse_paragraph_text(line, lc);
        }

        return document;
    }
};

//-----------------------------------------------------------------
// Code to parse inline elements (second pass)
//-----------------------------------------------------------------
struct InlineConsumer {
    ArrayView<const String> raw_lines;
    StringView raw_line;
    u32 line_index = 0;
    u32 i = 0;

    InlineConsumer(ArrayView<const String> raw_lines) : raw_lines{raw_lines} {
        PLY_ASSERT(raw_lines.num_items > 0);
        raw_line = raw_lines[0];
        PLY_ASSERT(raw_line);
    }

    enum ValidIndexResult { SameLine, NextLine, End };

    ValidIndexResult valid_index() {
        if (this->i >= this->raw_line.num_bytes) {
            if (this->line_index >= this->raw_lines.num_items) {
                return End;
            }
            this->i = 0;
            this->line_index++;
            if (this->line_index >= this->raw_lines.num_items) {
                this->raw_line = {};
                return End;
            }
            this->raw_line = this->raw_lines[this->line_index];
            PLY_ASSERT(this->raw_line);
            return NextLine;
        }
        return SameLine;
    }
};

String get_code_span(InlineConsumer& ic, u32 end_tick_count) {
    MemOutStream mout;
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.valid_index();
        if (res == InlineConsumer::End)
            return {};
        if (res == InlineConsumer::NextLine) {
            mout << ' ';
        }
        char c = ic.raw_line[ic.i];
        ic.i++;
        if (c == '`') {
            u32 tick_count = 1;
            for (; ic.i < ic.raw_line.num_bytes && ic.raw_line[ic.i] == '`'; ic.i++) {
                tick_count++;
            }
            if (tick_count == end_tick_count) {
                String result = mout.move_to_string();
                PLY_ASSERT(result);
                if (result[0] == ' ' && result.back() == ' ' &&
                    result.find_byte([](char c) { return c != ' '; }) >= 0) {
                    result = result.sub_str(1, result.num_bytes - 2);
                }
                return result;
            }
            mout << ic.raw_line.sub_str(ic.i - tick_count, tick_count);
        } else {
            mout << c;
        }
    }
}

// FIXME: Recognize all Unicode punctuation
PLY_INLINE bool is_asc_punc(char c) {
    return (c >= 0x21 && c <= 0x2f) || (c >= 0x3a && c <= 0x40) ||
           (c >= 0x5b && c <= 0x60) || (c >= 0x7b && c <= 0x7e);
}

struct Delimiter {
    enum Type {
        RawText,
        Stars,
        Underscores,
        OpenLink,
        InlineElem,
    };

    Type type = RawText;
    bool left_flanking = false;  // Stars & Underscores only
    bool right_flanking = false; // Stars & Underscores only
    bool active = true;          // OpenLink only
    HybridString text;
    Owned<Node> element; // InlineElem only, and it'll be an inline node type

    PLY_INLINE Delimiter() = default;
    PLY_INLINE Delimiter(Type type, HybridString&& text)
        : type{type}, text{std::move(text)} {
    }
    PLY_INLINE Delimiter(Owned<Node>&& elem)
        : type{InlineElem}, element{std::move(elem)} {
    }
    static PLY_NO_INLINE Delimiter make_run(Type type, StringView raw_line, u32 start,
                                            u32 num_bytes) {
        bool preceded_by_white = (start == 0) || is_white(raw_line[start - 1]);
        bool followed_by_white = (start + num_bytes >= raw_line.num_bytes) ||
                                 is_white(raw_line[start + num_bytes]);
        bool preceded_by_punc = (start > 0) && is_asc_punc(raw_line[start - 1]);
        bool followed_by_punc = (start + num_bytes < raw_line.num_bytes) &&
                                is_asc_punc(raw_line[start + num_bytes]);

        Delimiter result{type, raw_line.sub_str(start, num_bytes)};
        result.left_flanking =
            !followed_by_white &&
            (!followed_by_punc ||
             (followed_by_punc && (preceded_by_white || preceded_by_punc)));
        result.right_flanking =
            !preceded_by_white &&
            (!preceded_by_punc ||
             (preceded_by_punc && (followed_by_white || followed_by_punc)));
        return result;
    }
};

Tuple<bool, String> parse_link_destination(InlineConsumer& ic) {
    // FIXME: Support < > destinations
    // FIXME: Support link titles

    // Skip initial whitespace
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.valid_index();
        if (res == InlineConsumer::End) {
            return {false, String{}};
        }
        if (!is_white(ic.raw_line[ic.i]))
            break;
        ic.i++;
    }

    MemOutStream mout;
    u32 paren_nest_level = 0;
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.valid_index();
        if (res != InlineConsumer::SameLine)
            break;

        char c = ic.raw_line[ic.i];
        if (c == '\\') {
            ic.i++;
            if (ic.valid_index() != InlineConsumer::SameLine) {
                mout << '\\';
                break;
            }
            c = ic.raw_line[ic.i];
            if (!is_asc_punc(c)) {
                mout << '\\';
            }
            mout << c;
        } else if (c == '(') {
            ic.i++;
            mout << c;
            paren_nest_level++;
        } else if (c == ')') {
            if (paren_nest_level > 0) {
                ic.i++;
                mout << c;
                paren_nest_level--;
            } else {
                break;
            }
        } else if (c >= 0 && c <= 32) {
            break;
        } else {
            ic.i++;
            mout << c;
        }
    }

    if (paren_nest_level != 0) {
        return {false, String{}};
    }

    // Skip trailing whitespace
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.valid_index();
        if (res == InlineConsumer::End) {
            return {false, String{}};
        }
        char c = ic.raw_line[ic.i];
        if (c == ')') {
            ic.i++;
            return {true, mout.move_to_string()};
        } else if (!is_white(c)) {
            return {false, String{}};
        }
        ic.i++;
    }
}

Array<Owned<Node>> convert_to_inline_elems(ArrayView<Delimiter> delimiters) {
    Array<Owned<Node>> elements;
    for (Delimiter& delimiter : delimiters) {
        if (delimiter.type == Delimiter::InlineElem) {
            elements.append(std::move(delimiter.element));
        } else {
            if (!(elements.num_items() > 0 && elements.back()->type == Node::Text)) {
                elements.append(new Node{nullptr, Node::Text});
            }
            elements.back()->text += delimiter.text;
        }
    }
    return elements;
}

Array<Owned<Node>> process_emphasis(Array<Delimiter>& delimiters, u32 bottom_pos) {
    u32 star_opener = bottom_pos;
    u32 underscore_opener = bottom_pos;
    for (u32 pos = bottom_pos; pos < delimiters.num_items(); pos++) {
        auto handle_closer = [&](Delimiter::Type type, u32& opener_pos) {
            for (u32 j = pos; j > opener_pos;) {
                --j;
                if (delimiters[j].type == type && delimiters[j].left_flanking) {
                    u32 span_length = min(delimiters[j].text.num_bytes,
                                          delimiters[pos].text.num_bytes);
                    PLY_ASSERT(span_length > 0);
                    Owned<Node> elem = new Node{
                        nullptr, span_length >= 2 ? Node::Strong : Node::Emphasis};
                    elem->add_children(convert_to_inline_elems(
                        delimiters.sub_view(j + 1, pos - j - 1)));
                    u32 delims_to_subtract = min(span_length, 2u);
                    delimiters[j].text.num_bytes -= delims_to_subtract;
                    delimiters[pos].text.num_bytes -= delims_to_subtract;
                    // We're going to delete from j to pos inclusive, so leave remaining
                    // delimiters if any
                    if (!delimiters[j].text.is_empty()) {
                        j++;
                    }
                    if (!delimiters[pos].text.is_empty()) {
                        pos--;
                    }
                    delimiters.erase(j, pos + 1 - j);
                    delimiters.insert(j) = std::move(elem);
                    pos = j;
                    star_opener = min(star_opener, pos + 1);
                    underscore_opener = min(star_opener, pos + 1);
                    return;
                }
            }
            // None found
            opener_pos = pos + 1;
        };
        if (delimiters[pos].type == Delimiter::Stars &&
            delimiters[pos].right_flanking) {
            handle_closer(Delimiter::Stars, star_opener);
        } else if (delimiters[pos].type == Delimiter::Underscores &&
                   delimiters[pos].right_flanking) {
            handle_closer(Delimiter::Underscores, underscore_opener);
        }
    }
    Array<Owned<Node>> result =
        convert_to_inline_elems(delimiters.sub_view(bottom_pos));
    delimiters.resize(bottom_pos);
    return result;
}

Array<Owned<Node>> expand_inline_elements(ArrayView<const String> raw_lines) {
    Array<Delimiter> delimiters;
    InlineConsumer ic{raw_lines};
    u32 flushed_index = 0;
    auto flush_text = [&] {
        if (ic.i > flushed_index) {
            delimiters.append(
                {Delimiter::RawText,
                 ic.raw_line.sub_str(flushed_index, ic.i - flushed_index)});
            flushed_index = ic.i;
        }
    };
    for (;;) {
        if (ic.i >= ic.raw_line.num_bytes) {
            flush_text();
            ic.i = 0;
            flushed_index = 0;
            ic.line_index++;
            if (ic.line_index >= ic.raw_lines.num_items)
                break;
            ic.raw_line = ic.raw_lines[ic.line_index];
            delimiters.append(Owned<Node>::create(nullptr, Node::SoftBreak));
        }

        char c = ic.raw_line[ic.i];
        if (c == '`') {
            flush_text();
            u32 tick_count = 1;
            for (ic.i++; ic.i < ic.raw_line.num_bytes && ic.raw_line[ic.i] == '`';
                 ic.i++) {
                tick_count++;
            }
            // Try consuming code span
            InlineConsumer backup = ic;
            String code_str = get_code_span(ic, tick_count);
            if (code_str) {
                Owned<Node> code_span = new Node{nullptr, Node::CodeSpan};
                code_span->text = std::move(code_str);
                delimiters.append(std::move(code_span));
                flushed_index = ic.i;
            } else {
                ic = backup;
                flush_text();
            }
        } else if (c == '*') {
            flush_text();
            u32 run_length = 1;
            for (ic.i++; ic.i < ic.raw_line.num_bytes && ic.raw_line[ic.i] == '*';
                 ic.i++) {
                run_length++;
            }
            delimiters.append(Delimiter::make_run(Delimiter::Stars, ic.raw_line,
                                                  ic.i - run_length, run_length));
            flushed_index = ic.i;
        } else if (c == '_') {
            flush_text();
            u32 run_length = 1;
            for (ic.i++; ic.i < ic.raw_line.num_bytes && ic.raw_line[ic.i] == '_';
                 ic.i++) {
                run_length++;
            }
            delimiters.append(Delimiter::make_run(Delimiter::Underscores, ic.raw_line,
                                                  ic.i - run_length, run_length));
            flushed_index = ic.i;
        } else if (c == '[') {
            flush_text();
            delimiters.append({Delimiter::OpenLink, ic.raw_line.sub_str(ic.i, 1)});
            ic.i++;
            flushed_index = ic.i;
        } else if (c == ']') {
            // Try to parse an inline link
            flush_text();
            ic.i++;
            if (!(ic.i < ic.raw_line.num_bytes && ic.raw_line[ic.i] == '('))
                continue; // No parenthesis

            // Got opening parenthesis
            ic.i++;

            // Look for preceding OpenLink delimiter
            s32 open_link = rfind(delimiters, [](const Delimiter& delim) {
                return delim.type == Delimiter::OpenLink;
            });
            if (open_link < 0)
                continue; // No preceding OpenLink delimiter

            // Found a preceding OpenLink delimiter
            // Try to parse link destination
            InlineConsumer backup = ic;
            Tuple<bool, String> link_dest = parse_link_destination(ic);
            if (!link_dest.first) {
                // Couldn't parse link destination
                ic = backup;
                continue;
            }

            // Successfully parsed link destination
            Owned<Node> elem = new Node{nullptr, Node::Link};
            elem->text = std::move(link_dest.second);
            elem->add_children(process_emphasis(delimiters, open_link + 1));
            delimiters.resize(open_link);
            delimiters.append(std::move(elem));
            flushed_index = ic.i;
        } else {
            ic.i++;
        }
    }

    return process_emphasis(delimiters, 0);
}

static PLY_NO_INLINE void do_inlines(Node* node) {
    if (node->is_container_block()) {
        PLY_ASSERT(node->raw_lines.is_empty());
        for (Node* child : node->children) {
            do_inlines(child);
        }
    } else {
        PLY_ASSERT(node->is_leaf_block());
        if (node->type != Node::CodeBlock) {
            node->add_children(expand_inline_elements(node->raw_lines));
            node->raw_lines.clear();
        }
    }
}

//-----------------------------------------------------------------
// Main parse function
//-----------------------------------------------------------------
Owned<Node> parse(StringView src) {
    // First pass: Block elements
    Owned<Node> document = Parser{}.parse(src);

    // Second pass: Inline elements
    do_inlines(document);

    return document;
}

} // namespace markdown
} // namespace ply
