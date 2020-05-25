/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-cook-docs/Core.h>

namespace ply {
namespace docs {

struct SemaEntity;

struct TitleSpan {
    enum Type {
        Normal,
        Strong,
        Em,
        QID,
    };

    Type type = Normal;
    String text;
};

enum class ParseTitleError {
    ExpectedSpanTypeAfterOpenSquare = 0,
    UnclosedSpan,
    UnrecognizedSpanType,
    ExpectedSpaceAfterSpanType,
    UnexpectedCloseSquare,
    NumErrors,
};

Array<TitleSpan> parseTitle(
    StringView srcText,
    const LambdaView<void(ParseTitleError err, StringView arg, const char* loc)>& errorCallback);
void writeParseTitleError(StringWriter* sw, ParseTitleError err, StringView arg);
void writeAltMemberTitle(StringWriter& htmlWriter, ArrayView<const TitleSpan> spans,
                         SemaEntity* classEnt,
                         String (*getLinkDestination)(StringView, SemaEntity*));

} // namespace docs
} // namespace ply
