/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Function.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/ObjectStack.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

NativeBindings& getNativeBindings_Function() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {
            PLY_ASSERT(0); // Not supported
        },
        // construct
        [](AnyObject obj) {
            *(void**) obj.data = nullptr;
        },
        // destruct
        [](AnyObject obj) {
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
            *(void**) dst.data = *(void**) src.data;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
            *(void**) dst.data = *(void**) src.data;
        },
    };
    return bindings;
}

TypeKey TypeKey_Function {
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "built-in function";
    },
    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Function* functionType = obj.type->cast<TypeDescriptor_Function>();
        PLY_UNUSED(functionType);
        PLY_FORCE_CRASH(); // FIXME
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Function* functionType = typeDesc->cast<TypeDescriptor_Function>();
        PLY_UNUSED(functionType);
        PLY_FORCE_CRASH(); // FIXME
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* functionType = typeDesc->cast<const TypeDescriptor_Function>();
        for (const TypeDescriptor* paramType : functionType->paramTypes) {
            hasher << paramType;
        }
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* functionType0 = type0->cast<const TypeDescriptor_Function>();
        const auto* functionType1 = type1->cast<const TypeDescriptor_Function>();
        if (functionType0->paramTypes.numItems() != functionType1->paramTypes.numItems())
            return false;
        for (u32 i = 0; i < functionType0->paramTypes.numItems(); i++) {
            if (!functionType0->paramTypes[i]->isEquivalentTo(functionType1->paramTypes[i]))
                return false;
        }
        return true;
    },
};

TypeKey* TypeDescriptor_Function::typeKey = &TypeKey_Function;

} // namespace ply
