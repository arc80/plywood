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
    AnyObject* lastObj = &this->items.tail();
    PLY_ASSERT(PLY_PTR_OFFSET(lastObj->data, lastObj->type->fixedSize) ==
               this->storage.tail->unused());
    lastObj->destruct();
    this->storage.popLastBytes(lastObj->type->fixedSize);
    this->items.popTail();
}

} // namespace ply
