/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Struct.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

PLY_NO_INLINE MethodTable getMethodTable_Struct() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> FnResult {
        const TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        const TypeDescriptor_Struct::Member* member = structType->findMember(propertyName);
        if (!member) {
            interp->returnValue = {};
            interp->error(String::format("property '{}' not found in type '{}'", propertyName,
                                         obj.type->getName()));
            return Fn_Error;
        }
        interp->returnValue = {PLY_PTR_OFFSET(obj.data, member->offset), member->type};
        return Fn_OK;
    };
    return methods;
}

#endif // PLY_WITH_METHOD_TABLES

NativeBindings& getNativeBindings_SynthesizedStruct() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor* typeDesc) -> AnyObject {
            void* data = Heap.alloc(typeDesc->fixedSize);
            AnyObject obj{data, typeDesc};
            obj.construct();
            return obj;
        },
        // destroy
        [](AnyObject obj) {
            obj.destruct();
            Heap.free(obj.data);
        },
        // construct
        [](AnyObject obj) {
            TypeDescriptor_Struct* structType = (TypeDescriptor_Struct*) obj.type;
            // Zero-initialize the struct before calling the constructors of any members.
            // I'm not sure this is always what we want, but it will help set the padding to
            // zero when exporting synthesized vertex attributes:
            memset(obj.data, 0, structType->fixedSize);
            for (TypeDescriptor_Struct::Member& member : structType->members) {
                member.type->bindings.construct(
                    {PLY_PTR_OFFSET(obj.data, member.offset), member.type});
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Struct* structType = (TypeDescriptor_Struct*) obj.type;
            for (TypeDescriptor_Struct::Member& member : structType->members) {
                member.type->bindings.destruct(
                    {PLY_PTR_OFFSET(obj.data, member.offset), member.type});
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            TypeDescriptor_Struct* structType = (TypeDescriptor_Struct*) dst.type;
            for (TypeDescriptor_Struct::Member& member : structType->members) {
                member.type->bindings.copy({PLY_PTR_OFFSET(dst.data, member.offset), member.type},
                                           {PLY_PTR_OFFSET(src.data, member.offset), member.type});
            }
        },
    };
    return bindings;
}

} // namespace ply
