/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/methods/BaseInterpreter.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

StringView MethodTable::unaryOpToString(MethodTable::UnaryOp op) {
    static StringView table[] = {
        "???",
        "-",
        "!",
        "~",
    };
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(table) == (u32) MethodTable::UnaryOp::ContainerStart);
    if ((u32) op < (u32) MethodTable::UnaryOp::ContainerStart) {
        return table[(u32) op];
    }
    return "???";
}

StringView MethodTable::binaryOpToString(MethodTable::BinaryOp op) {
    static StringView table[] = {
        "???", "*", "/", "%", "+", "-", "<", "<=", ">", ">=", "==", "&&", "||",
    };
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(table) == (u32) MethodTable::BinaryOp::Count);
    if ((u32) op < (u32) MethodTable::BinaryOp::Count) {
        return table[(u32) op];
    }
    return "???";
}

MethodResult MethodTable::unsupportedUnaryOp(BaseInterpreter* interp, UnaryOp op,
                                             const AnyObject& obj) {
    interp->returnValue = {};
    interp->error(interp, String::format("'{}' does not support unary operator '{}'",
                                         obj.type->getName(), MethodTable::unaryOpToString(op)));
    return MethodResult::Error;
}

MethodResult MethodTable::unsupportedBinaryOp(BaseInterpreter* interp, MethodTable::BinaryOp op,
                                              const AnyObject& first, const AnyObject& second) {
    interp->returnValue = {};
    interp->error(interp, String::format("'{}' does not support binary operator '{}'",
                                         first.type->getName(), MethodTable::binaryOpToString(op)));
    return MethodResult::Error;
}

MethodResult MethodTable::unsupportedPropertyLookup(BaseInterpreter* interp, const AnyObject& obj,
                                                    StringView propertyName) {
    interp->returnValue = {};
    interp->error(interp,
                  String::format("'{}' does not support property lookup", obj.type->getName()));
    return MethodResult::Error;
}

MethodResult MethodTable::unsupportedSubscript(BaseInterpreter* interp, const AnyObject& obj,
                                               u32 index) {
    interp->returnValue = {};
    interp->error(interp,
                  String::format("'{}' does not support array indexing", obj.type->getName()));
    return MethodResult::Error;
}

MethodResult MethodTable::unsupportedPrint(BaseInterpreter* interp, const AnyObject& obj,
                                           StringView formatSpec) {
    interp->returnValue = {};
    interp->error(interp, String::format("'{}' does not support printing", obj.type->getName()));
    return MethodResult::Error;
}

MethodResult MethodTable::unsupportedCall(BaseInterpreter* interp, const AnyObject& callee,
                                          ArrayView<const AnyObject> args) {
    interp->returnValue = {};
    interp->error(interp,
                  String::format("'{}' cannot be called as a function", callee.type->getName()));
    return MethodResult::Error;
}

PLY_NO_INLINE MethodTable::MethodTable()
    : unaryOp{unsupportedUnaryOp}, binaryOp{unsupportedBinaryOp},
      propertyLookup{unsupportedPropertyLookup}, subscript{unsupportedSubscript},
      print{unsupportedPrint}, call{unsupportedCall} {
}

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
