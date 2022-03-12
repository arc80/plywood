/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/TypeKey.h>

namespace ply {

NativeBindings& getNativeBindings_FixedArray() {
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
            TypeDescriptor_FixedArray* arrType = obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            void* item = obj.data;
            for (u32 i : range(arrType->numItems)) {
                PLY_UNUSED(i);
                itemType->bindings.construct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_FixedArray* arrType = obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            void* item = obj.data;
            for (u32 i : range(arrType->numItems)) {
                PLY_UNUSED(i);
                itemType->bindings.destruct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            TypeDescriptor_FixedArray* dstArrType = dst.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* dstItemType = dstArrType->itemType;
            if (src.type->typeKey == &TypeKey_FixedArray) {
                const TypeDescriptor_FixedArray* srcArrType =
                    src.type->cast<TypeDescriptor_FixedArray>();
                TypeDescriptor* srcItemType = srcArrType->itemType;
                // FIXME: Warn about size mismatch
                u32 itemsToCopy = min<u32>(dstArrType->numItems, srcArrType->numItems);
                AnyObject dstItem = {dst.data, dstItemType};
                AnyObject srcItem = {src.data, srcItemType};
                while (itemsToCopy--) {
                    dstItem.copy(srcItem);
                    // FIXME: Support different strides
                    dstItem.data = PLY_PTR_OFFSET(dstItem.data, dstArrType->stride);
                    srcItem.data = PLY_PTR_OFFSET(srcItem.data, srcArrType->stride);
                }
            } else if (src.type->typeKey == &TypeKey_Array) {
                const TypeDescriptor_Array* srcArrType = src.type->cast<TypeDescriptor_Array>();
                const details::BaseArray* baseSrcArray =
                    reinterpret_cast<details::BaseArray*>(src.data);
                TypeDescriptor* srcItemType = srcArrType->itemType;
                // FIXME: Warn about size mismatch
                u32 itemsToCopy = min<u32>(dstArrType->numItems, baseSrcArray->m_numItems);
                AnyObject dstItem = {dst.data, dstItemType};
                AnyObject srcItem = {baseSrcArray->m_items, srcItemType};
                while (itemsToCopy--) {
                    dstItem.copy(srcItem);
                    // FIXME: Support different strides
                    dstItem.data = PLY_PTR_OFFSET(dstItem.data, dstArrType->stride);
                    srcItem.data = PLY_PTR_OFFSET(srcItem.data, srcItemType->fixedSize);
                }
            } else {
                PLY_ASSERT(0); // Not implemented yet
            }
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_Array() {
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
        [](AnyObject obj) { new (obj.data) details::BaseArray; },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
            TypeDescriptor* itemType = arrayType->itemType;
            details::BaseArray* arr = (details::BaseArray*) obj.data;
            void* item = arr->m_items;
            u32 itemSize = itemType->fixedSize;
            // FIXME: Skip this loop if itemType is trivially
            // destructible (Need a way to determine that)
            for (u32 i : range(arr->m_numItems)) {
                PLY_UNUSED(i);
                itemType->bindings.destruct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
            arr->~BaseArray();
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_Owned() {
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
            // Note: This is type punning Owned<T> with void*
            *(void**) obj.data = nullptr;
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Owned* ownedType = obj.type->cast<TypeDescriptor_Owned>();
            // Note: This is type punning Owned<T> with void*
            AnyObject targetObj = AnyObject{*(void**) obj.data, ownedType->targetType};
            targetObj.destroy(); // Note: This assumes operator new() uses PLY_HEAP.alloc
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->typeKey == &TypeKey_Owned);
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
            // Note: This is type punning AppOwned<T> with void*
            if (*(void**) dst.data) {
                // Must destroy existing object
                PLY_FORCE_CRASH(); // Unimplemented
            }
            *(void**) dst.data = *(void**) src.data;
            *(void**) src.data = nullptr;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_Reference() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {
            TypeDescriptor_Reference* referenceType = obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                referenceType->decRef(target);
            }
            PLY_HEAP.free(obj.data);
        },
        // construct
        [](AnyObject obj) {
            // Note: This is type punning Reference<T> with void*
            *(void**) obj.data = nullptr;
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Reference* referenceType = obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                referenceType->decRef(target);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
            TypeDescriptor_Reference* referenceType = dst.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* prevTarget = *(void**) dst.data;
            if (prevTarget) {
                referenceType->decRef(prevTarget);
            }
            *(void**) dst.data = *(void**) src.data;
            *(void**) src.data = nullptr;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_Enum() {
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
        [](AnyObject obj) {},
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->typeKey == &TypeKey_Enum);
            PLY_ASSERT(dst.type->isEquivalentTo(src.type));
            TypeDescriptor_Enum* enumType = dst.type->cast<TypeDescriptor_Enum>();
            if (enumType->fixedSize == 1) {
                *(u8*) dst.data = *(u8*) src.data;
            } else if (enumType->fixedSize == 2) {
                *(u16*) dst.data = *(u16*) src.data;
            } else if (enumType->fixedSize == 4) {
                *(u32*) dst.data = *(u32*) src.data;
            } else {
                PLY_ASSERT(0);
            }
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_RawPtr() {
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
        [](AnyObject obj) { *(void**) obj.data = nullptr; },
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

NativeBindings& getNativeBindings_EnumIndexedArray() {
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
            TypeDescriptor_EnumIndexedArray* arrType =
                obj.type->cast<TypeDescriptor_EnumIndexedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            u32 count = arrType->enumType->identifiers.numItems();
            PLY_ASSERT(itemSize * count == arrType->fixedSize);
            void* item = obj.data;
            for (u32 i = 0; i < count; i++) {
                PLY_UNUSED(i);
                itemType->bindings.construct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_EnumIndexedArray* arrType =
                obj.type->cast<TypeDescriptor_EnumIndexedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            u32 count = arrType->enumType->identifiers.numItems();
            PLY_ASSERT(itemSize * count == arrType->fixedSize);
            void* item = obj.data;
            for (u32 i = 0; i < count; i++) {
                PLY_UNUSED(i);
                itemType->bindings.destruct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

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

PLY_DEFINE_TYPE_DESCRIPTOR(bool) {
    static TypeDescriptor typeDesc{&TypeKey_Bool, sizeof(bool), NativeBindings::make<bool>()};
    return &typeDesc;
};

PLY_DEFINE_TYPE_DESCRIPTOR(s8) {
    static TypeDescriptor typeDesc{&TypeKey_S8, sizeof(s8), NativeBindings::make<s8>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s16) {
    static TypeDescriptor typeDesc{&TypeKey_S16, sizeof(s16), NativeBindings::make<s16>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s32) {
    static TypeDescriptor typeDesc{&TypeKey_S32, sizeof(s32), NativeBindings::make<s32>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s64) {
    static TypeDescriptor typeDesc{&TypeKey_S64, sizeof(s64), NativeBindings::make<s64>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u8) {
    static TypeDescriptor typeDesc{&TypeKey_U8, sizeof(u8), NativeBindings::make<u8>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u16) {
    static TypeDescriptor typeDesc{&TypeKey_U16, sizeof(u16), NativeBindings::make<u16>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u32) {
    static TypeDescriptor typeDesc{&TypeKey_U32, sizeof(u32), NativeBindings::make<u32>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u64) {
    static TypeDescriptor typeDesc{&TypeKey_U64, sizeof(u64), NativeBindings::make<u64>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(float) {
    static TypeDescriptor typeDesc{&TypeKey_Float, sizeof(float), NativeBindings::make<float>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(double) {
    static TypeDescriptor typeDesc{&TypeKey_Double, sizeof(double), NativeBindings::make<double>()};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(String) {
    static TypeDescriptor typeDesc{&TypeKey_String, sizeof(String), NativeBindings::make<String>()};
    return &typeDesc;
}

} // namespace ply
