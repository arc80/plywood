/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/SymbolTitle.h>
#include <ply-web-cook-docs/SemaEntity.h>

namespace ply {
namespace docs {

void write_parse_title_error(OutStream* outs, ParseTitleError err, StringView arg) {
    switch (err) {
        case ParseTitleError::ExpectedSpanTypeAfterOpenSquare: {
            *outs << "expected span type immediately following '['\n";
            break;
        }
        case ParseTitleError::UnclosedSpan: {
            *outs << "unclosed '['\n";
            break;
        }
        case ParseTitleError::UnrecognizedSpanType: {
            outs->format("unrecognized span type '{}'\n", arg);
            break;
        }
        case ParseTitleError::ExpectedSpaceAfterSpanType: {
            outs->format("expected a single space immedately following '{}'", arg);
            break;
        }
        case ParseTitleError::UnexpectedCloseSquare: {
            *outs << "unexpected ']'\n";
            break;
        }
        default: {
            *outs << "error message not implemented!\n";
            break;
        }
    }
}

Array<TitleSpan> parse_title(StringView src_text,
                             const Functor<void(ParseTitleError err, StringView arg,
                                                const char* loc)>& error_callback) {
    Array<TitleSpan> result;
    ViewInStream vins{src_text};
    MemOutStream mout;
    TitleSpan::Type in_span_type = TitleSpan::Normal;
    const char* span_start = src_text.bytes;

    auto flush_span = [&] {
        String text = mout.move_to_string();
        if (text) {
            result.append({in_span_type, std::move(text)});
        }
        mout = {}; // Begin new MemOutStream
    };

    while (vins.num_bytes_available() > 0) {
        char c = vins.read_byte();
        if (c == '\\') {
            if (vins.num_bytes_available() > 0) {
                mout << (char) vins.read_byte();
            }
        } else if (c == '[') {
            if (in_span_type != TitleSpan::Normal) {
                error_callback(ParseTitleError::UnclosedSpan, {}, span_start);
            }
            flush_span();
            span_start = (const char*) vins.cur_byte - 1;
            StringView span_type = vins.read_view<fmt::Identifier>();
            if (!span_type) {
                error_callback(ParseTitleError::ExpectedSpanTypeAfterOpenSquare, {},
                               span_start);
            } else if (span_type == "strong") {
                in_span_type = TitleSpan::Strong;
            } else if (span_type == "em") {
                in_span_type = TitleSpan::Em;
            } else if (span_type == "qid") {
                in_span_type = TitleSpan::QID;
            } else {
                error_callback(ParseTitleError::UnrecognizedSpanType, span_type,
                               span_type.bytes);
                in_span_type = TitleSpan::Strong;
            }
            if (vins.num_bytes_available() == 0 || vins.peek_byte() != ' ') {
                error_callback(ParseTitleError::ExpectedSpaceAfterSpanType, span_type,
                               (const char*) vins.cur_byte);
            } else {
                vins.advance_byte();
            }
        } else if (c == ']') {
            if (in_span_type == TitleSpan::Normal) {
                error_callback(ParseTitleError::UnexpectedCloseSquare, {},
                               (const char*) vins.cur_byte - 1);
            } else {
                flush_span();
                in_span_type = TitleSpan::Normal;
            }
        } else {
            mout << c;
        }
    }

    if (in_span_type != TitleSpan::Normal) {
        error_callback(ParseTitleError::UnclosedSpan, {}, span_start);
    }
    flush_span();

    return result;
}

void write_alt_member_title(OutStream& html_writer, ArrayView<const TitleSpan> spans,
                            const LookupContext& lookup_ctx,
                            String (*get_link_destination)(StringView,
                                                           const LookupContext&)) {
    for (const TitleSpan& span : spans) {
        switch (span.type) {
            case TitleSpan::Normal: {
                html_writer << fmt::XMLEscape{span.text};
                break;
            }
            case TitleSpan::Strong: {
                html_writer.format("<strong>{}</strong>", fmt::XMLEscape{span.text});
                break;
            }
            case TitleSpan::Em: {
                html_writer.format("<em>{}</em>", fmt::XMLEscape{span.text});
                break;
            }
            case TitleSpan::QID: {
                String link_destination = get_link_destination(span.text, lookup_ctx);
                if (link_destination) {
                    html_writer.format("<a href=\"{}\">",
                                       fmt::XMLEscape{link_destination});
                }
                html_writer << fmt::XMLEscape{span.text};
                if (link_destination) {
                    html_writer << "</a>";
                }
                break;
            }
            default: {
                PLY_ASSERT(0);
                break;
            }
        }
    }
}

} // namespace docs
} // namespace ply
