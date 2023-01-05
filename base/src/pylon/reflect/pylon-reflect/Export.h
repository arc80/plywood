/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon-reflect/Core.h>
#include <pylon/Node.h>
#include <ply-reflect/TypeDescriptor.h>

namespace pylon {

using FilterFunc = Func<Owned<Node>(AnyObject)>;
Owned<Node> exportObj(AnyObject obj, const FilterFunc& filter = {});

} // namespace pylon
