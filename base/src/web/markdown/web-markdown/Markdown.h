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
    u32 indentOrLevel = 0;           // only for ListItems & Headings
    s32 listStartNumber = 0;         // only for Lists. -1 means unordered
    bool isLooseIfContinued = false; // only for Lists
    bool isLoose = false;            // only for Lists
    char listPunc = '-';             // only for Lists
    Array<Owned<Node>> children;
    Node* parent = nullptr;
    Array<String> rawLines; // only for Leaf nodes (Heading, Paragraph, CodeBlock)
    String text;            // only for Text, CodeSpan or Link (for the destination)
    String id;              // sets the id attribute for Headings

    PLY_INLINE Node(Node* parent, Type type) : type{type}, parent{parent} {
        if (parent) {
            parent->children.append(this);
        }
    }
    PLY_INLINE void addChildren(ArrayView<Owned<Node>> newChildren) {
        for (Node* newChild : newChildren) {
            PLY_ASSERT(!newChild->parent);
            newChild->parent = this;
        }
        this->children.moveExtend(newChildren);
    }
    PLY_INLINE Array<Owned<Node>> removeChildren() {
        for (Node* child : this->children) {
            PLY_ASSERT(child->parent == this);
            child->parent = nullptr;
        }
        return std::move(this->children);
    }
    PLY_INLINE bool isContainerBlock() const {
        return this->type < StartLeafNodeType;
    }
    PLY_INLINE bool isLeafBlock() const {
        return this->type >= StartLeafNodeType && this->type < StartInlineNodeType;
    }
    PLY_INLINE bool isInlineElement() const {
        return this->type >= StartInlineNodeType;
    }
    PLY_INLINE bool isOrderedList() const {
        return this->type == List && this->listStartNumber >= 0;
    }
};

Owned<Node> parse(StringView src);
void dump(OutStream* outs, const Node* node, u32 level = 0);

struct HTMLOptions {
    bool childAnchors = false;
};

void convertToHTML(OutStream* outs, const Node* node, const HTMLOptions& options);

} // namespace markdown
} // namespace ply
