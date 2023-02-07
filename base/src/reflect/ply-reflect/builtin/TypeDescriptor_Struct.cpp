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

MethodTable get_method_table_struct() {
    MethodTable methods;
    methods.property_lookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                 StringView property_name) -> FnResult {
        const TypeDescriptor_Struct* struct_type =
            obj.type->cast<TypeDescriptor_Struct>();
        const TypeDescriptor_Struct::Member* member =
            struct_type->find_member(property_name);
        if (!member) {
            interp->return_value = {};
            interp->error(String::format("property '{}' not found in type '{}'",
                                         property_name, obj.type->get_name()));
            return Fn_Error;
        }
        interp->return_value = {PLY_PTR_OFFSET(obj.data, member->offset), member->type};
        return Fn_OK;
    };
    return methods;
}

#endif // PLY_WITH_METHOD_TABLES

NativeBindings& get_native_bindings_synthesized_struct() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor* type_desc) -> AnyObject {
            void* data = Heap.alloc(type_desc->fixed_size);
            AnyObject obj{data, type_desc};
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
            TypeDescriptor_Struct* struct_type = (TypeDescriptor_Struct*) obj.type;
            // Zero-initialize the struct before calling the constructors of any
            // members. I'm not sure this is always what we want, but it will help set
            // the padding to zero when exporting synthesized vertex attributes:
            memset(obj.data, 0, struct_type->fixed_size);
            for (TypeDescriptor_Struct::Member& member : struct_type->members) {
                member.type->bindings.construct(
                    {PLY_PTR_OFFSET(obj.data, member.offset), member.type});
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Struct* struct_type = (TypeDescriptor_Struct*) obj.type;
            for (TypeDescriptor_Struct::Member& member : struct_type->members) {
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
            TypeDescriptor_Struct* struct_type = (TypeDescriptor_Struct*) dst.type;
            for (TypeDescriptor_Struct::Member& member : struct_type->members) {
                member.type->bindings.copy(
                    {PLY_PTR_OFFSET(dst.data, member.offset), member.type},
                    {PLY_PTR_OFFSET(src.data, member.offset), member.type});
            }
        },
    };
    return bindings;
}

} // namespace ply
