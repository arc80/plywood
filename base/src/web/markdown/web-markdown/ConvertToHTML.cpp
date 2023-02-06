/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>
#include <web-common/URLEscape.h>

namespace ply {
namespace markdown {

void convert_to_html(OutStream* outs, const Node* node, const HTMLOptions& options) {
    switch (node->type) {
        case Node::Document: {
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            break;
        }
        case Node::List: {
            if (node->is_ordered_list()) {
                if (node->list_start_number != 1) {
                    outs->format("<ol start=\"{}\">\n", node->list_start_number);
                } else {
                    *outs << "<ol>\n";
                }
            } else {
                *outs << "<ul>\n";
            }
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            if (node->is_ordered_list()) {
                *outs << "</ol>\n";
            } else {
                *outs << "</ul>\n";
            }
            break;
        }
        case Node::ListItem: {
            *outs << "<li>";
            if (!node->parent->is_loose && node->children[0]->type == Node::Paragraph) {
                // Don't output a newline before the paragraph in a tight list.
            } else {
                *outs << "\n";
            }
            for (u32 i = 0; i < node->children.num_items(); i++) {
                convert_to_html(outs, node->children[i], options);
                if (!node->parent->is_loose &&
                    node->children[i]->type == Node::Paragraph &&
                    i + 1 < node->children.num_items()) {
                    // This paragraph had no <p> tag and didn't end in a newline, but
                    // there are more children following it, so add a newline here.
                    *outs << "\n";
                }
            }
            *outs << "</li>\n";
            break;
        }
        case Node::BlockQuote: {
            *outs << "<blockquote>\n";
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            *outs << "</blockquote>\n";
            break;
        }
        case Node::Heading: {
            outs->format("<h{}", node->indent_or_level);
            if (node->id) {
                if (options.child_anchors) {
                    outs->format(" class=\"anchored\"><span class=\"anchor\" "
                                 "id=\"{}\">&nbsp;</span>",
                                 fmt::XMLEscape{node->id});
                } else {
                    outs->format(" id=\"{}\">", fmt::XMLEscape{node->id});
                }
            } else {
                *outs << '>';
            }
            PLY_ASSERT(node->raw_lines.is_empty());
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            outs->format("</h{}>\n", node->indent_or_level);
            break;
        }
        case Node::Paragraph: {
            bool is_inside_tight = (node->parent->type == Node::ListItem &&
                                    !node->parent->parent->is_loose);
            if (!is_inside_tight) {
                *outs << "<p>";
            }
            PLY_ASSERT(node->raw_lines.is_empty());
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            if (!is_inside_tight) {
                *outs << "</p>\n";
            }
            break;
        }
        case Node::CodeBlock: {
            *outs << "<pre><code>";
            PLY_ASSERT(node->children.is_empty());
            for (StringView raw_line : node->raw_lines) {
                *outs << fmt::XMLEscape{raw_line};
            }
            *outs << "</code></pre>\n";
            break;
        }
        case Node::Text: {
            *outs << fmt::XMLEscape{node->text};
            PLY_ASSERT(node->children.is_empty());
            break;
        }
        case Node::Link: {
            outs->format("<a href=\"{}\">", fmt::XMLEscape{node->text});
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            *outs << "</a>";
            break;
        }
        case Node::CodeSpan: {
            *outs << "<code>" << fmt::XMLEscape{node->text} << "</code>";
            PLY_ASSERT(node->children.is_empty());
            break;
        }
        case Node::SoftBreak: {
            *outs << "\n";
            PLY_ASSERT(node->children.is_empty());
            break;
        }
        case Node::Emphasis: {
            *outs << "<em>";
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
            }
            *outs << "</em>";
            break;
        }
        case Node::Strong: {
            *outs << "<strong>";
            for (const Node* child : node->children) {
                convert_to_html(outs, child, options);
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
