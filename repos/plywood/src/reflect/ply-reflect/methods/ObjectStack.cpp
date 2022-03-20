/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

AnyObject* ObjectStack::appendObject(TypeDescriptor* type) {
    void* data = this->storage.appendBytes(type->fixedSize);
    return &this->items.append(data, type);
}

void ObjectStack::popLastObject() {
    AnyObject* lastObj = &this->items.last();
    lastObj->destruct();
    PLY_ASSERT(PLY_PTR_OFFSET(lastObj->data, lastObj->type->fixedSize) ==
               this->storage.tail->unused());
}

} // namespace ply
