/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypedArray.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/TypeSynthesizer.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

PLY_NO_INLINE void TypedArrayView::constructSlow(TypedArrayView view) {
    TypedPtr typedItem{view.items, view.itemTypeOwner->getRootType()};
    u32 stride = typedItem.type->fixedSize;
    auto ctor = typedItem.type->bindings.construct;
    for (u32 numItems = view.numItems; numItems > 0; numItems--) {
        ctor(typedItem);
        typedItem.ptr = PLY_PTR_OFFSET(typedItem.ptr, stride);
    }
}

PLY_NO_INLINE void TypedArrayView::destructSlow(TypedArrayView view) {
    TypedPtr typedItem{view.items, view.itemTypeOwner->getRootType()};
    u32 stride = typedItem.type->fixedSize;
    auto dtor = typedItem.type->bindings.destruct;
    for (u32 numItems = view.numItems; numItems > 0; numItems--) {
        dtor(typedItem);
        typedItem.ptr = PLY_PTR_OFFSET(typedItem.ptr, stride);
    }
}

PLY_NO_INLINE void TypedArray::operator=(TypedArray&& other) {
    if (m_array.m_numItems > 0) {
        view().destruct();
    }
    PLY_HEAP.free(m_array.m_items);
    m_array.m_items = other.m_array.m_items;
    m_array.m_numItems = other.m_array.m_numItems;
    m_array.m_allocated = other.m_array.m_allocated;
    other.m_array.m_items = nullptr;
    other.m_array.m_numItems = 0;
    other.m_array.m_allocated = 0;
    m_typeOwner = std::move(other.m_typeOwner);
}

PLY_NO_INLINE void TypedArray::destroy() {
    if (m_array.m_numItems > 0) {
        view().destruct();
    }
    m_array.free();
    m_typeOwner = nullptr;
}

PLY_NO_INLINE void TypedArray::create(TypeDescriptorOwner* typeOwner, u32 numItems) {
    if (m_array.m_numItems > 0) {
        view().destruct();
    }
    m_array.realloc(numItems, typeOwner->getRootType()->fixedSize);
    m_typeOwner = typeOwner;
    view().construct();
}

PLY_NO_INLINE void TypedArray::assign(TypeDescriptorOwner* typeOwner, void* items, u32 numItems) {
    if (m_array.m_numItems > 0) {
        view().destruct();
    }
    m_typeOwner = typeOwner;
    m_array.m_items = items;
    m_array.m_numItems = numItems;
    m_array.m_allocated = numItems;
}

PLY_NO_INLINE TypedPtr TypedArray::append() {
    u32 i = m_array.m_numItems++;
    m_array.reserve(m_array.m_numItems, m_typeOwner->getRootType()->fixedSize);
    TypedPtr result = getItem(i);
    result.construct();
    return result;
}

TypeKey TypeKey_TypedArray{
    // write
    [](TypedPtr obj, WriteObjectContext* context) {
        TypedArray* arr = (TypedArray*) obj.ptr;
        TypeDescriptor* itemType = arr->getItemType();
        PLY_ASSERT(itemType);

        // Make sure the items' TypeDescriptor is written as part of the schema.
        // Assign and write its formatID.
        u32 itemFormatID = context->writeFormatContext->addOrGetFormatID(itemType);
        context->out.write<u32>(itemFormatID);

        // Write all array items.
        u32 itemSize = itemType->fixedSize;
        void* item = arr->m_array.m_items;
        PLY_ASSERT(arr->m_array.m_numItems <= UINT32_MAX);
        context->out.write<u32>((u32) arr->m_array.m_numItems);
        for (u32 i : range(arr->m_array.m_numItems)) {
            PLY_UNUSED(i);
            itemType->typeKey->write(TypedPtr{item, itemType}, context);
            item = PLY_PTR_OFFSET(item, itemSize);
        }
    },

    // writeFormat
    [](TypeDescriptor*, WriteFormatContext* context) {
        context->writePrimitive(FormatKey::TypedArray);
    },

    // read
    [](TypedPtr obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::TypedArray) {
            // FIXME: More detailed message
            // FIXME: Could probably accept Arrays, TypedArrays
            SLOG(Load, "Can't load TypedArray");
            skip(context, formatDesc);
            return;
        }

        // Read formatID and look up the corresponding FormatDescriptor of the array items.
        u32 itemFormatID = context->in.read<u32>();
        FormatDescriptor* itemFormat = context->schema->getFormatDesc(itemFormatID);

        // Synthesize TypeDescriptor with all its child types, and give the whole group a
        // TypeDescriptorOwner.
        Reference<TypeDescriptorOwner> itemTypeOwner = synthesizeType(itemFormat);
        TypeDescriptor* itemType = itemTypeOwner->getRootType();

        // Read all array items
        TypedArray* arr = (TypedArray*) obj.ptr;
        u32 itemSize = itemType->fixedSize;
        u32 arrSize = context->in.read<u32>();
        arr->create(itemTypeOwner, arrSize);
        TypedPtr typedItem{arr->m_array.m_items, itemType};
        for (u32 i : range((u32) arrSize)) {
            PLY_UNUSED(i);
            itemType->typeKey->read(typedItem, context, itemFormat);
            typedItem.ptr = PLY_PTR_OFFSET(typedItem.ptr, itemSize);
        }
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<TypedArray>::get() {
    static TypeDescriptor typeDesc{&TypeKey_TypedArray, sizeof(TypedArray),
                                   NativeBindings::make<TypedArray>()};
    return &typeDesc;
}

} // namespace ply
