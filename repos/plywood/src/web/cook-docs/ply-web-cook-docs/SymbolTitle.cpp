/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/SymbolTitle.h>
#include <ply-web-cook-docs/SemaEntity.h>

namespace ply {
namespace docs {

void writeParseTitleError(StringWriter* sw, ParseTitleError err, StringView arg) {
    switch (err) {
        case ParseTitleError::ExpectedSpanTypeAfterOpenSquare: {
            *sw << "expected span type immediately following '['\n";
            break;
        }
        case ParseTitleError::UnclosedSpan: {
            *sw << "unclosed '['\n";
            break;
        }
        case ParseTitleError::UnrecognizedSpanType: {
            sw->format("unrecognized span type '{}'\n", arg);
            break;
        }
        case ParseTitleError::ExpectedSpaceAfterSpanType: {
            sw->format("expected a single space immedately following '{}'", arg);
            break;
        }
        case ParseTitleError::UnexpectedCloseSquare: {
            *sw << "unexpected ']'\n";
            break;
        }
        default: {
            *sw << "error message not implemented!\n";
            break;
        }
    }
}

Array<TitleSpan> parseTitle(
    StringView srcText,
    const LambdaView<void(ParseTitleError err, StringView arg, const char* loc)>& errorCallback) {
    Array<TitleSpan> result;
    StringViewReader svr{srcText};
    StringWriter sw;
    TitleSpan::Type inSpanType = TitleSpan::Normal;
    const char* spanStart = srcText.bytes;

    auto flushSpan = [&] {
        String text = sw.moveToString();
        if (text) {
            result.append({inSpanType, std::move(text)});
        }
        sw = {}; // Begin new StringWriter
    };

    while (svr.numBytesAvailable() > 0) {
        char c = svr.readByte();
        if (c == '\\') {
            if (svr.numBytesAvailable() > 0) {
                sw << (char) svr.readByte();
            }
        } else if (c == '[') {
            if (inSpanType != TitleSpan::Normal) {
                errorCallback(ParseTitleError::UnclosedSpan, {}, spanStart);
            }
            flushSpan();
            spanStart = (const char*) svr.curByte - 1;
            StringView spanType = svr.readView<fmt::Identifier>();
            if (!spanType) {
                errorCallback(ParseTitleError::ExpectedSpanTypeAfterOpenSquare, {}, spanStart);
            } else if (spanType == "strong") {
                inSpanType = TitleSpan::Strong;
            } else if (spanType == "em") {
                inSpanType = TitleSpan::Em;
            } else if (spanType == "qid") {
                inSpanType = TitleSpan::QID;
            } else {
                errorCallback(ParseTitleError::UnrecognizedSpanType, spanType, spanType.bytes);
                inSpanType = TitleSpan::Strong;
            }
            if (svr.numBytesAvailable() == 0 || svr.peekByte() != ' ') {
                errorCallback(ParseTitleError::ExpectedSpaceAfterSpanType, spanType,
                              (const char*) svr.curByte);
            } else {
                svr.advanceByte();
            }
        } else if (c == ']') {
            if (inSpanType == TitleSpan::Normal) {
                errorCallback(ParseTitleError::UnexpectedCloseSquare, {},
                              (const char*) svr.curByte - 1);
            } else {
                flushSpan();
                inSpanType = TitleSpan::Normal;
            }
        } else {
            sw << c;
        }
    }

    if (inSpanType != TitleSpan::Normal) {
        errorCallback(ParseTitleError::UnclosedSpan, {}, spanStart);
    }
    flushSpan();

    return result;
}

void writeAltMemberTitle(StringWriter& htmlWriter, ArrayView<const TitleSpan> spans,
                         SemaEntity* classEnt,
                         String (*getLinkDestination)(StringView, SemaEntity*)) {
    for (const TitleSpan& span : spans) {
        switch (span.type) {
            case TitleSpan::Normal: {
                htmlWriter << fmt::XMLEscape{span.text};
                break;
            }
            case TitleSpan::Strong: {
                htmlWriter.format("<strong>{}</strong>", fmt::XMLEscape{span.text});
                break;
            }
            case TitleSpan::Em: {
                htmlWriter.format("<em>{}</em>", fmt::XMLEscape{span.text});
                break;
            }
            case TitleSpan::QID: {
                String linkDestination = getLinkDestination(span.text, classEnt);
                if (linkDestination) {
                    htmlWriter.format("<a href=\"{}\">", fmt::XMLEscape{linkDestination});
                }
                htmlWriter << fmt::XMLEscape{span.text};
                if (linkDestination) {
                    htmlWriter << "</a>";
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
