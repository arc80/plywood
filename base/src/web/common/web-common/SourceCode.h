/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>
#include <web-common/Response.h>

namespace ply {
namespace web {

struct SourceCode {
    String rootDir;

    PLY_NO_INLINE static void serve(const SourceCode* params, StringView requestPath,
                                    ResponseIface* responseIface);
};

} // namespace web
} // namespace ply
