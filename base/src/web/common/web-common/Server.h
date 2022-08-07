/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>
#include <web-common/Response.h>

namespace ply {
namespace web {

bool runServer(u16 port, const RequestHandler& reqHandler);

} // namespace web
} // namespace ply
