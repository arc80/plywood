/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct BoolMethodTable {
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                              const AnyObject& first,
                              const AnyObject& second) -> MethodResult {
            switch (op) {
                case MethodTable::BinaryOp::DoubleEqual: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() =
                        (*first.cast<bool>() == *second.cast<bool>());
                    return MethodResult::OK;
                }
                case MethodTable::BinaryOp::LogicalAnd: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() =
                        (*first.cast<bool>() && *second.cast<bool>());
                    return MethodResult::OK;
                }
                case MethodTable::BinaryOp::LogicalOr: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() =
                        (*first.cast<bool>() || *second.cast<bool>());
                    return MethodResult::OK;
                }
                default: {
                    return MethodTable::unsupportedBinaryOp(interp, op, first, second);
                }
            }
        };
        return methods;
    }
};

#endif // PLY_WITH_METHOD_TABLES

PLY_DEFINE_TYPE_DESCRIPTOR(bool) {
    static TypeDescriptor typeDesc{&TypeKey_Bool, (bool*) nullptr,
                                   NativeBindings::make<bool>()
                                       PLY_METHOD_TABLES_ONLY(, BoolMethodTable::make())};
    return &typeDesc;
};

} // namespace ply
