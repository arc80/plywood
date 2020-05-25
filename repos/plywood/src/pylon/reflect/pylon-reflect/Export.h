/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon-reflect/Core.h>
#include <pylon/Node.h>
#include <ply-reflect/TypeDescriptor.h>

namespace pylon {

using FilterFunc = HiddenArgFunctor<bool(Node&, TypedPtr)>;
void exportObjTo(Node& aNode, TypedPtr obj, const FilterFunc& filter);
Node exportObj(TypedPtr obj, const FilterFunc& filter = {});

} // namespace pylon
