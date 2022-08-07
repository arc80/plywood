/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/SymbolTitle.h>
#include <ply-web-cook-docs/SemaEntity.h>

namespace ply {
namespace docs {

void writeParseTitleError(OutStream* outs, ParseTitleError err, StringView arg) {
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

Array<TitleSpan> parseTitle(
    StringView srcText,
    const LambdaView<void(ParseTitleError err, StringView arg, const char* loc)>& errorCallback) {
    Array<TitleSpan> result;
    ViewInStream vins{srcText};
    MemOutStream mout;
    TitleSpan::Type inSpanType = TitleSpan::Normal;
    const char* spanStart = srcText.bytes;

    auto flushSpan = [&] {
        String text = mout.moveToString();
        if (text) {
            result.append({inSpanType, std::move(text)});
        }
        mout = {}; // Begin new MemOutStream
    };

    while (vins.numBytesAvailable() > 0) {
        char c = vins.readByte();
        if (c == '\\') {
            if (vins.numBytesAvailable() > 0) {
                mout << (char) vins.readByte();
            }
        } else if (c == '[') {
            if (inSpanType != TitleSpan::Normal) {
                errorCallback(ParseTitleError::UnclosedSpan, {}, spanStart);
            }
            flushSpan();
            spanStart = (const char*) vins.curByte - 1;
            StringView spanType = vins.readView<fmt::Identifier>();
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
            if (vins.numBytesAvailable() == 0 || vins.peekByte() != ' ') {
                errorCallback(ParseTitleError::ExpectedSpaceAfterSpanType, spanType,
                              (const char*) vins.curByte);
            } else {
                vins.advanceByte();
            }
        } else if (c == ']') {
            if (inSpanType == TitleSpan::Normal) {
                errorCallback(ParseTitleError::UnexpectedCloseSquare, {},
                              (const char*) vins.curByte - 1);
            } else {
                flushSpan();
                inSpanType = TitleSpan::Normal;
            }
        } else {
            mout << c;
        }
    }

    if (inSpanType != TitleSpan::Normal) {
        errorCallback(ParseTitleError::UnclosedSpan, {}, spanStart);
    }
    flushSpan();

    return result;
}

void writeAltMemberTitle(OutStream& htmlWriter, ArrayView<const TitleSpan> spans,
                         const LookupContext& lookupCtx,
                         String (*getLinkDestination)(StringView, const LookupContext&)) {
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
                String linkDestination = getLinkDestination(span.text, lookupCtx);
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
