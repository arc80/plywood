/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
            if (node->isLoose) {
                *outs << " (loose";
            } else {
                *outs << " (tight";
            }
            if (node->isOrderedList()) {
                outs->format(", ordered, start={})", node->listStartNumber);
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
            outs->format("heading level={}", node->indentOrLevel);
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
    for (StringView text : node->rawLines) {
        outs->format("{}  \"{}\"\n", indent, fmt::EscapedString{text});
    }
    for (const Node* child : node->children) {
        PLY_ASSERT(child->parent == node);
        dump(outs, child, level + 1);
    }
}

} // namespace markdown
} // namespace ply
