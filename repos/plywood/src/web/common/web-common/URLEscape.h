/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>
#include <ply-runtime/io/text/StringWriter.h>

namespace ply {
namespace web {
struct URLEscape {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE URLEscape(StringView view, u32 maxPoints = 0) : view{view}, maxPoints{maxPoints} {
    }
};
} // namespace web

template <>
struct fmt::TypePrinter<web::URLEscape> {
    static PLY_NO_INLINE void print(StringWriter* sw, const web::URLEscape& value);
};
} // namespace ply
