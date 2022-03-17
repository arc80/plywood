/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>

namespace ply {

void defaultUnaryOp(ObjectStack* stack, MethodTable::UnaryOp op, const AnyObject& obj) {
    PLY_ASSERT(0);
}

void defaultBinaryOp(ObjectStack* stack, MethodTable::BinaryOp op, const AnyObject& first,
                     const AnyObject& second) {
    PLY_ASSERT(0);
}

void defaultPropertyLookup(ObjectStack* stack, const AnyObject& obj, StringView propertyName) {
    PLY_ASSERT(0);
}

void defaultSubscript(ObjectStack* stack, const AnyObject& obj, u32 index) {
    PLY_ASSERT(0);
}

void defaultPrint(ObjectStack* stack, OutStream* outs, StringView formatSpec,
                  const AnyObject& obj) {
    PLY_ASSERT(0);
}

void defaultCall(ObjectStack* stack, const AnyObject& obj) {
    PLY_ASSERT(0);
}

PLY_NO_INLINE MethodTable::MethodTable()
    : unaryOp{defaultUnaryOp}, binaryOp{defaultBinaryOp}, propertyLookup{defaultPropertyLookup},
      subscript{defaultSubscript}, print{defaultPrint}, call{defaultCall} {
}

} // namespace ply
