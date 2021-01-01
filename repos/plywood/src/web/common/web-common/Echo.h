/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>
#include <web-common/Response.h>

namespace ply {
namespace web {

PLY_NO_INLINE void echo_serve(const void*, const StringView requestPath, ResponseIface* responseIface);

} // namespace web
} // namespace ply
