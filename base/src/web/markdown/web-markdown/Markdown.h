/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <web-markdown/Core.h>

namespace ply {
namespace markdown {

struct Node {
    enum Type {
        // Container node types (contain other blocks):
        Document = 0,
        List,
        ListItem,
        BlockQuote,
        // Leaf node types (contain only text):
        StartLeafNodeType,
        Heading = StartLeafNodeType,
        Paragraph,
        CodeBlock,
        // Inline node types
        StartInlineNodeType,
        Text = StartInlineNodeType,
        Link,
        CodeSpan,
        SoftBreak,
        Emphasis,
        Strong,
    };

    Type type = Document;
    u32 indent_or_level = 0;            // only for ListItems & Headings
    s32 list_start_number = 0;          // only for Lists. -1 means unordered
    bool is_loose_if_continued = false; // only for Lists
    bool is_loose = false;              // only for Lists
    char list_punc = '-';               // only for Lists
    Array<Owned<Node>> children;
    Node* parent = nullptr;
    Array<String> raw_lines; // only for Leaf nodes (Heading, Paragraph, CodeBlock)
    String text;             // only for Text, CodeSpan or Link (for the destination)
    String id;               // sets the id attribute for Headings

    Node(Node* parent, Type type) : type{type}, parent{parent} {
        if (parent) {
            parent->children.append(this);
        }
    }
    void add_children(ArrayView<Owned<Node>> new_children) {
        for (Node* new_child : new_children) {
            PLY_ASSERT(!new_child->parent);
            new_child->parent = this;
        }
        this->children.move_extend(new_children);
    }
    Array<Owned<Node>> remove_children() {
        for (Node* child : this->children) {
            PLY_ASSERT(child->parent == this);
            child->parent = nullptr;
        }
        return std::move(this->children);
    }
    bool is_container_block() const {
        return this->type < StartLeafNodeType;
    }
    bool is_leaf_block() const {
        return this->type >= StartLeafNodeType && this->type < StartInlineNodeType;
    }
    bool is_inline_element() const {
        return this->type >= StartInlineNodeType;
    }
    bool is_ordered_list() const {
        return this->type == List && this->list_start_number >= 0;
    }
};

Owned<Node> parse(StringView src);
void dump(OutStream* outs, const Node* node, u32 level = 0);

struct HTMLOptions {
    bool child_anchors = false;
};

void convert_to_html(OutStream* outs, const Node* node, const HTMLOptions& options);

} // namespace markdown
} // namespace ply
