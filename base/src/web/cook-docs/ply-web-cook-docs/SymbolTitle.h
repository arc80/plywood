﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-web-cook-docs/Core.h>

namespace ply {
namespace docs {

struct SemaEntity;
struct LookupContext;

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
    const Functor<void(ParseTitleError err, StringView arg, const char* loc)>& errorCallback);
void writeParseTitleError(OutStream* outs, ParseTitleError err, StringView arg);
void writeAltMemberTitle(OutStream& htmlWriter, ArrayView<const TitleSpan> spans,
                         const LookupContext& lookupCtx,
                         String (*getLinkDestination)(StringView, const LookupContext&));

} // namespace docs
} // namespace ply
