/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>
#include <web-common/URLEscape.h>

namespace ply {
namespace markdown {

void convertToHTML(StringWriter* sw, const Node* node) {
    switch (node->type) {
        case Node::Document: {
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            break;
        }
        case Node::List: {
            if (node->isOrderedList()) {
                if (node->listStartNumber != 1) {
                    sw->format("<ol start=\"{}\">\n", node->listStartNumber);
                } else {
                    *sw << "<ol>\n";
                }
            } else {
                *sw << "<ul>\n";
            }
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            if (node->isOrderedList()) {
                *sw << "</ol>\n";
            } else {
                *sw << "</ul>\n";
            }
            break;
        }
        case Node::ListItem: {
            if (!node->parent->isLoose && node->children.numItems() == 1 &&
                node->children[0]->type == Node::Paragraph) {
                // It's a paragraph in a tight list. Omit the <p> tags:
                Node* para = node->children[0];
                *sw << "<li>";
                PLY_ASSERT(para->rawLines.isEmpty());
                for (const Node* child : para->children) {
                    convertToHTML(sw, child);
                }
                *sw << "</li>\n";
            } else {
                *sw << "<li>\n";
                for (const Node* child : node->children) {
                    convertToHTML(sw, child);
                }
                *sw << "</li>\n";
            }
            break;
        }
        case Node::BlockQuote: {
            *sw << "<blockquote>\n";
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            *sw << "</blockquote>\n";
            break;
        }
        case Node::Heading: {
            sw->format("<h{}>", node->indentOrLevel);
            PLY_ASSERT(node->rawLines.isEmpty());
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            sw->format("</h{}>\n", node->indentOrLevel);
            break;
        }
        case Node::Paragraph: {
            *sw << "<p>";
            PLY_ASSERT(node->rawLines.isEmpty());
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            *sw << "</p>\n";
            break;
        }
        case Node::CodeBlock: {
            *sw << "<pre><code>";
            PLY_ASSERT(node->children.isEmpty());
            for (StringView rawLine : node->rawLines) {
                *sw << fmt::XMLEscape{rawLine};
            }
            *sw << "</code></pre>\n";
            break;
        }
        case Node::Text: {
            *sw << fmt::XMLEscape{node->text};
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::Link: {
            sw->format("<a href=\"{}\">", fmt::XMLEscape{node->text});
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            *sw << "</a>";
            break;
        }
        case Node::CodeSpan: {
            *sw << "<code>" << fmt::XMLEscape{node->text} << "</code>";
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::SoftBreak: {
            *sw << "\n";
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::Emphasis: {
            *sw << "<em>";
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            *sw << "</em>";
            break;
        }
        case Node::Strong: {
            *sw << "<strong>";
            for (const Node* child : node->children) {
                convertToHTML(sw, child);
            }
            *sw << "</strong>";
            break;
        }
        default: {
            PLY_ASSERT(0);
            break;
        }
    }
}

} // namespace markdown
} // namespace ply
