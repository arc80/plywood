/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
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

Array<TitleSpan> parse_title(StringView src_text,
                             const Functor<void(ParseTitleError err, StringView arg,
                                                const char* loc)>& error_callback);
void write_parse_title_error(OutStream* outs, ParseTitleError err, StringView arg);
void write_alt_member_title(OutStream& html_writer, ArrayView<const TitleSpan> spans,
                            const LookupContext& lookup_ctx,
                            String (*get_link_destination)(StringView,
                                                           const LookupContext&));

} // namespace docs
} // namespace ply
