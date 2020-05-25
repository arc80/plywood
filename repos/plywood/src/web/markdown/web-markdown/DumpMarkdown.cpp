/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>

namespace ply {
namespace markdown {

void dump(StringWriter* sw, const Node* node, u32 level) {
    String indent = StringView{"  "} * level;
    *sw << indent;
    switch (node->type) {
        case Node::Document: {
            *sw << "document";
            break;
        }
        case Node::List: {
            *sw << "list";
            if (node->isLoose) {
                *sw << " (loose";
            } else {
                *sw << " (tight";
            }
            if (node->isOrderedList()) {
                sw->format(", ordered, start={})", node->listStartNumber);
            } else {
                *sw << ", unordered)";
            }
            break;
        }
        case Node::ListItem: {
            *sw << "item";
            break;
        }
        case Node::BlockQuote: {
            *sw << "block_quote";
            break;
        }
        case Node::Heading: {
            sw->format("heading level={}", node->indentOrLevel);
            break;
        }
        case Node::Paragraph: {
            *sw << "paragraph";
            break;
        }
        case Node::CodeBlock: {
            *sw << "code_block";
            break;
        }
        case Node::Text: {
            sw->format("text \"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::Link: {
            sw->format("link destination=\"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::CodeSpan: {
            sw->format("code \"{}\"", fmt::EscapedString{node->text});
            break;
        }
        case Node::SoftBreak: {
            *sw << "softbreak";
            break;
        }
        case Node::Emphasis: {
            *sw << "emph";
            break;
        }
        case Node::Strong: {
            *sw << "strong";
            break;
        }
        default: {
            PLY_ASSERT(0);
            *sw << "???";
            break;
        }
    }
    *sw << "\n";
    for (StringView text : node->rawLines) {
        sw->format("{}  \"{}\"\n", indent, fmt::EscapedString{text});
    }
    for (const Node* child : node->children) {
        PLY_ASSERT(child->parent == node);
        dump(sw, child, level + 1);
    }
}

} // namespace markdown
} // namespace ply
