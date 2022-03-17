/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

AnyObject* ObjectStack::appendObject(TypeDescriptor* type) {
    void* data = this->storage.appendBytes(type->fixedSize);
    return &this->items.append(data, type);
}

} // namespace crowbar
