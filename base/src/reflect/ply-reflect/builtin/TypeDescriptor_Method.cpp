/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Method.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/AnyObject.h>
#include <ply-reflect/methods/ObjectStack.h>

namespace ply {

NativeBindings& getNativeBindings_Method() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {},
        // construct
        [](AnyObject obj) { *(void**) obj.data = nullptr; },
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
    };
    return bindings;
}

TypeKey TypeKey_Method{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "built-in function";
    },
    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Method* functionType = obj.type->cast<TypeDescriptor_Method>();
        PLY_UNUSED(functionType);
        PLY_FORCE_CRASH(); // FIXME
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Method* functionType = typeDesc->cast<TypeDescriptor_Method>();
        PLY_UNUSED(functionType);
        PLY_FORCE_CRASH(); // FIXME
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        PLY_FORCE_CRASH(); // FIXME
        return true;
    },
};

TypeKey* TypeDescriptor_Method::typeKey = &TypeKey_Method;

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
