/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Struct.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& getNativeBindings_SynthesizedStruct() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor* typeDesc) -> AnyObject {
            void* data = PLY_HEAP.alloc(typeDesc->fixedSize);
            AnyObject obj{data, typeDesc};
            obj.construct();
            return obj;
        },
        // destroy
        [](AnyObject obj) {
            obj.destruct();
            PLY_HEAP.free(obj.data);
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
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
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