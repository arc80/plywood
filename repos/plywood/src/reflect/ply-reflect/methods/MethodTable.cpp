/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

AnyObject defaultUnaryOp(ObjectStack* stack, MethodTable::UnaryOp op, const AnyObject& obj) {
    PLY_ASSERT(0);
    return {};
}

AnyObject defaultBinaryOp(ObjectStack* stack, MethodTable::BinaryOp op, const AnyObject& first,
                     const AnyObject& second) {
    PLY_ASSERT(0);
    return {};
}

AnyObject defaultPropertyLookup(ObjectStack* stack, const AnyObject& obj, StringView propertyName) {
    PLY_ASSERT(0);
    return {};
}

AnyObject defaultSubscript(ObjectStack* stack, const AnyObject& obj, u32 index) {
    PLY_ASSERT(0);
    return {};
}

void defaultPrint(ObjectStack* stack, OutStream* outs, StringView formatSpec,
                  const AnyObject& obj) {
    PLY_ASSERT(0);
}

AnyObject defaultCall(ObjectStack* stack, const AnyObject& obj) {
    PLY_ASSERT(0);
    return {};
}

PLY_NO_INLINE MethodTable::MethodTable()
    : unaryOp{defaultUnaryOp}, binaryOp{defaultBinaryOp}, propertyLookup{defaultPropertyLookup},
      subscript{defaultSubscript}, print{defaultPrint}, call{defaultCall} {
}

} // namespace ply
