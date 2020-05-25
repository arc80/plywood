/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-serve-docs/Core.h>
#include <web-common/Response.h>
#include <web-documentation/Contents.h>

namespace ply {
namespace web {

struct DocServer {
    String dataRoot;
    Array<Contents> contents;

    void init(StringView dataRoot);
    void serve(StringView requestPath, ResponseIface* responseIface) const;
};

} // namespace web
} // namespace ply
