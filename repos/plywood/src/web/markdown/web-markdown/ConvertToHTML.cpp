/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>
#include <web-common/URLEscape.h>

namespace ply {
namespace markdown {

void convertToHTML(OutStream* outs, const Node* node, const HTMLOptions& options) {
    switch (node->type) {
        case Node::Document: {
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            break;
        }
        case Node::List: {
            if (node->isOrderedList()) {
                if (node->listStartNumber != 1) {
                    outs->format("<ol start=\"{}\">\n", node->listStartNumber);
                } else {
                    *outs << "<ol>\n";
                }
            } else {
                *outs << "<ul>\n";
            }
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            if (node->isOrderedList()) {
                *outs << "</ol>\n";
            } else {
                *outs << "</ul>\n";
            }
            break;
        }
        case Node::ListItem: {
            *outs << "<li>";
            if (!node->parent->isLoose && node->children[0]->type == Node::Paragraph) {
                // Don't output a newline before the paragraph in a tight list.
            } else {
                *outs << "\n";
            }
            for (u32 i = 0; i < node->children.numItems(); i++) {
                convertToHTML(outs, node->children[i], options);
                if (!node->parent->isLoose && node->children[i]->type == Node::Paragraph &&
                    i + 1 < node->children.numItems()) {
                    // This paragraph had no <p> tag and didn't end in a newline, but there are more
                    // children following it, so add a newline here.
                    *outs << "\n";
                }
            }
            *outs << "</li>\n";
            break;
        }
        case Node::BlockQuote: {
            *outs << "<blockquote>\n";
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            *outs << "</blockquote>\n";
            break;
        }
        case Node::Heading: {
            outs->format("<h{}", node->indentOrLevel);
            if (node->id) {
                if (options.childAnchors) {
                    outs->format(" class=\"anchored\"><span class=\"anchor\" id=\"{}\">&nbsp;</span>",
                               fmt::XMLEscape{node->id});
                } else {
                    outs->format(" id=\"{}\">", fmt::XMLEscape{node->id});
                }
            } else {
                *outs << '>';
            }
            PLY_ASSERT(node->rawLines.isEmpty());
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            outs->format("</h{}>\n", node->indentOrLevel);
            break;
        }
        case Node::Paragraph: {
            bool isInsideTight =
                (node->parent->type == Node::ListItem && !node->parent->parent->isLoose);
            if (!isInsideTight) {
                *outs << "<p>";
            }
            PLY_ASSERT(node->rawLines.isEmpty());
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            if (!isInsideTight) {
                *outs << "</p>\n";
            }
            break;
        }
        case Node::CodeBlock: {
            *outs << "<pre><code>";
            PLY_ASSERT(node->children.isEmpty());
            for (StringView rawLine : node->rawLines) {
                *outs << fmt::XMLEscape{rawLine};
            }
            *outs << "</code></pre>\n";
            break;
        }
        case Node::Text: {
            *outs << fmt::XMLEscape{node->text};
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::Link: {
            outs->format("<a href=\"{}\">", fmt::XMLEscape{node->text});
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            *outs << "</a>";
            break;
        }
        case Node::CodeSpan: {
            *outs << "<code>" << fmt::XMLEscape{node->text} << "</code>";
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::SoftBreak: {
            *outs << "\n";
            PLY_ASSERT(node->children.isEmpty());
            break;
        }
        case Node::Emphasis: {
            *outs << "<em>";
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            *outs << "</em>";
            break;
        }
        case Node::Strong: {
            *outs << "<strong>";
            for (const Node* child : node->children) {
                convertToHTML(outs, child, options);
            }
            *outs << "</strong>";
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
