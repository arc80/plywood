/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon/Core.h>
#include <pylon/Node.h>
#include <ply-runtime/io/OutStream.h>

namespace pylon {

void write(OutStream* outs, const Node* aNode);
String toString(const Node* aNode);

} // namespace pylon
