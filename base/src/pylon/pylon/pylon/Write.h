/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon/Core.h>
#include <pylon/Node.h>

namespace pylon {

void write(OutStream& out, const Node* aNode);
String toString(const Node* aNode);

} // namespace pylon
