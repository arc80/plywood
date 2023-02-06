/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>

namespace ply {
namespace markdown {

void dump(OutStream* outs, const Node* node, u32 level) {
    String indent = StringView{"  "} * level;
    *outs << indent;
    switch (node->type) {
        case Node::Document: {
            *outs << "document";
            break;
        }
        case Node::List: {
            *outs << "list";
            if (node->is_loose) {
                *outs << " (loose";
            } else {
                *outs << " (tight";
            }
            if (node->is_ordered_list()) {
                outs->format(", ordered, start={})", node->list_start_number);
            } else {
                *outs << ", unordered)";
            }
            break;
        }
        case Node::ListItem: {
            *outs << "item";
            break;
        }
        case Node::BlockQuote: {
            *outs << "block_quote";
            break;
        }
        case Node::Heading: {
            outs->format("heading level={}", node->indent_or_level);
            break;
        }
        case Node::Paragraph: {
            *outs << "paragraph";
            break;
        }
        case Node::CodeBlock: {
            *outs << "code_block";
            break;
        }
        case Node::Text: {
            outs->format("text \"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::Link: {
            outs->format("link destination=\"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::CodeSpan: {
            outs->format("code \"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::SoftBreak: {
            *outs << "softbreak";
            break;
        }
        case Node::Emphasis: {
            *outs << "emph";
            break;
        }
        case Node::Strong: {
            *outs << "strong";
            break;
        }
        default: {
            PLY_ASSERT(0);
            *outs << "???";
            break;
        }
    }
    *outs << "\n";
    for (StringView text : node->raw_lines) {
        outs->format("{}  \"{}\"\n", indent, fmt::EscapedString{text});
    }
    for (const Node* child : node->children) {
        PLY_ASSERT(child->parent == node);
        dump(outs, child, level + 1);
    }
}

} // namespace markdown
} // namespace ply
