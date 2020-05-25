/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-documentation/Core.h>

namespace ply {
namespace web {

struct Contents {
    PLY_REFLECT()
    String title;
    String linkDestination;
    Array<Contents> children;
    // ply reflect off
};

} // namespace web
} // namespace ply
