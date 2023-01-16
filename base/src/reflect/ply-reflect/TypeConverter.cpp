/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeConverter.h>
#include <ply-reflect/TypeKey.h>

// FIXME: Remove this declaration when textures are removed from the uniform objects:
namespace assetBank {
extern ply::TypeKey TypeKey_AssetRef;
};

namespace ply {

#if 0 // disabled for now
// Note: If TypeDescriptors were flat, with enum-based TypeKeys, this function might not be needed:
void writeTypeSignature(BinaryBuffer& sig, const TypeDescriptor* typeDesc) {
    enum class Token : u8 {
        End = 0,
        Float,
        FixedArray,
        Array,
        Struct,
    };

    if (typeDesc->typeKey == &TypeKey_Float) {
        sig.append(Token::Float);
    } else if (typeDesc->typeKey == &TypeKey_FixedArray) {
        const TypeDescriptor_FixedArray* fixedArrType = typeDesc->cast<TypeDescriptor_FixedArray>();
        sig.append(Token::FixedArray);
        sig.append(safeDemote<u16>(fixedArrType->numItems));
        sig.append(safeDemote<u16>(fixedArrType->stride));
        writeTypeSignature(sig, fixedArrType->itemType);
    } else if (typeDesc->typeKey == &TypeKey_Array) {
        const TypeDescriptor_Array* arrType = typeDesc->cast<TypeDescriptor_Array>();
        sig.append(Token::Array);
        writeTypeSignature(sig, arrType->itemType);
    } else if (typeDesc->typeKey == &TypeKey_Struct) {
        const TypeDescriptor_Struct* structType = typeDesc->cast<TypeDescriptor_Struct>();
        sig.append(Token::Struct);
        sig.append(safeDemote<u8>(structType->members.numItems()));
        for (const auto& member : structType->members) {
            sig.append(safeDemote<u8>(member.name.numBytes()));
            void* dst = sig.beginEnqueue(member.name.numBytes());
            memcpy(dst, member.name.bytes(), member.name.numBytes());
            sig.append(safeDemote<u16>(member.offset));
            writeTypeSignature(sig, member.type);
        }
    } else if (typeDesc->typeKey == &TypeKey_RawPtr || typeDesc->typeKey == &assetBank::TypeKey_AssetRef) {
        // FIXME: Need to remove textures from the uniform objects
    } else {
        PLY_FORCE_CRASH();     // unsupported destination typeDesc
    }
}
#endif

struct TypeConverter {
    struct WriteContext {
        u32 dstOffset;
        u32 srcOffset;

        WriteContext(u32 dstOffset, u32 srcOffset) : dstOffset{dstOffset}, srcOffset{srcOffset} {
        }
    };

    enum class Cmd : u16 {
        SetRootSourceIndex,
        IterateArrayToFixedArray,
        Copy32,
        Copy32Range,
        EndScope
    };

    struct SetRootSourceIndex {
        Cmd cmd;
        u16 sourceIndex;
    };

    struct IterateArrayToFixedArray {
        Cmd cmd;
        u16 dstOffset;
        u16 srcOffset;
        u16 dstStride;
        u16 srcStride;
        u16 dstSize;
    };

    struct BaseCmd {
        Cmd cmd;
        u16 dstOffset;
        u16 srcOffset;
    };

    struct Copy32Range {
        Cmd cmd;
        u16 dstOffset;
        u16 srcOffset;
        u16 count;
    };
};

void makeConversionRecipe(OutStream& out, const TypeConverter::WriteContext& writeCtx,
                          const TypeDescriptor* typeDesc, const TypeDescriptor* srcTypeDesc) {
    if (typeDesc->typeKey == &TypeKey_Float) {
        PLY_ASSERT(srcTypeDesc->typeKey == &TypeKey_Float);
        NativeEndianWriter{out}.write(TypeConverter::BaseCmd{TypeConverter::Cmd::Copy32,
                                                              safeDemote<u16>(writeCtx.dstOffset),
                                                              safeDemote<u16>(writeCtx.srcOffset)});
    } else if (typeDesc->typeKey == &TypeKey_FixedArray) {
        const TypeDescriptor_FixedArray* dstFixedArrayType =
            typeDesc->cast<TypeDescriptor_FixedArray>();
        if (srcTypeDesc->typeKey == &TypeKey_FixedArray) {
            const TypeDescriptor_FixedArray* srcFixedArrayType =
                srcTypeDesc->cast<TypeDescriptor_FixedArray>();
            // FIXME: Warn on size mismatch
            u32 itemsToCopy = min<u32>(dstFixedArrayType->numItems, srcFixedArrayType->numItems);
            if (dstFixedArrayType->itemType->typeKey == &TypeKey_Float &&
                srcFixedArrayType->itemType->typeKey == &TypeKey_Float &&
                dstFixedArrayType->stride == sizeof(float) &&
                srcFixedArrayType->stride == sizeof(float)) {
                // Fast path: FixedArray of float with default stride
                NativeEndianWriter{out}.write(TypeConverter::Copy32Range{
                    TypeConverter::Cmd::Copy32Range, safeDemote<u16>(writeCtx.dstOffset),
                    safeDemote<u16>(writeCtx.srcOffset), safeDemote<u16>(itemsToCopy)});
            } else {
                // Slow path
                TypeConverter::WriteContext childWriteCtx = writeCtx;
                for (u32 i = 0; i < itemsToCopy; i++) {
                    makeConversionRecipe(out, childWriteCtx, dstFixedArrayType->itemType,
                                         srcFixedArrayType->itemType);
                    childWriteCtx.dstOffset += dstFixedArrayType->stride;
                    childWriteCtx.srcOffset += srcFixedArrayType->stride;
                }
            }
        } else if (srcTypeDesc->typeKey == &TypeKey_Array) {
            const TypeDescriptor_Array* srcArrType = srcTypeDesc->cast<TypeDescriptor_Array>();
            NativeEndianWriter{out}.write(TypeConverter::IterateArrayToFixedArray{
                TypeConverter::Cmd::IterateArrayToFixedArray,
                safeDemote<u16>(writeCtx.dstOffset),
                safeDemote<u16>(writeCtx.srcOffset),
                safeDemote<u16>(dstFixedArrayType->stride),
                safeDemote<u16>(srcArrType->itemType->fixedSize),
                safeDemote<u16>(dstFixedArrayType->numItems),
            });
            makeConversionRecipe(out, {0, 0}, dstFixedArrayType->itemType, srcArrType->itemType);
            NativeEndianWriter{out}.write(TypeConverter::Cmd::EndScope);
        } else {
            PLY_FORCE_CRASH(); // unsupported srcTypeDesc
        }
    } else {
        PLY_FORCE_CRASH(); // unsupported destination typeDesc
    }
}

void createConversionRecipe(OutStream& out, const TypeDescriptor_Struct* dstStruct,
                            const ArrayView<TypeDescriptor_Struct*>& srcStructs) {
    for (const auto& dstMember : dstStruct->members) {
        for (u32 s = 0; s < srcStructs.numItems; s++) {
            TypeDescriptor_Struct* srcStruct = srcStructs[s];
            for (const auto& srcMember : srcStruct->members) {
                if (dstMember.name == srcMember.name) {
                    NativeEndianWriter{out}.write(TypeConverter::SetRootSourceIndex{
                        TypeConverter::Cmd::SetRootSourceIndex, safeDemote<u16>(s)});
                    TypeConverter::WriteContext writeCtx = {dstMember.offset, srcMember.offset};
                    makeConversionRecipe(out, writeCtx, dstMember.type, srcMember.type);
                    goto found;
                }
            }
        }
        PLY_ASSERT(0); // No match for dstMember
    found:;
    }
    NativeEndianWriter{out}.write(TypeConverter::Cmd::EndScope);
}

template <class T>
const T* safeCast(StringView view) {
    PLY_ASSERT(sizeof(T) <= view.numBytes);
    return (const T*) view.bytes;
}

void convert(BlockList::WeakRef cursor, void* dstPtr, ArrayView<void*> srcPtrs,
             void* srcPtr = nullptr) {
    // FIXME: Implement MessageSequence and use that instead.

    BlockList::Footer* viewBlock = nullptr;
    StringView view;
    for (;;) {
        if (view.isEmpty()) {
            viewBlock = cursor.block;
            if (!viewBlock)
                break;
            PLY_ASSERT(viewBlock->viewUsedBytes().contains(cursor.byte));
            view = {cursor.byte, safeDemote<u32>(viewBlock->end() - cursor.byte)};
            PLY_ASSERT(!view.isEmpty());
            cursor = cursor.block->weakRefToNext();
        }

        switch (*safeCast<TypeConverter::Cmd>(view)) {
            case TypeConverter::Cmd::SetRootSourceIndex: {
                // Note: Could add an assert here to ensure that we are at the "root"
                // convert(), but that would require passing an additional depth parameter
                auto* cmd = safeCast<TypeConverter::SetRootSourceIndex>(view);
                srcPtr = srcPtrs[cmd->sourceIndex];
                view.offsetHead(sizeof(TypeConverter::SetRootSourceIndex));
                break;
            }

            case TypeConverter::Cmd::IterateArrayToFixedArray: {
                auto cmd = *safeCast<TypeConverter::IterateArrayToFixedArray>(view);
                view.offsetHead(sizeof(TypeConverter::IterateArrayToFixedArray));

                impl::BaseArray* baseArr = (impl::BaseArray*) PLY_PTR_OFFSET(srcPtr, cmd.srcOffset);
                // FIXME: Warn if baseArr has too many source elements
                u32 itemsToCopy = min<u32>(cmd.dstSize, baseArr->m_numItems);
                void* childDstPtr = PLY_PTR_OFFSET(dstPtr, cmd.dstOffset);
                void* childSrcPtr = baseArr->m_items;
                while (itemsToCopy) {
                    BlockList::WeakRef childCursor = cursor; // Copy child cursor
                    convert(childCursor, childDstPtr, srcPtrs, childSrcPtr);
                    childDstPtr = PLY_PTR_OFFSET(childDstPtr, cmd.dstStride);
                    childSrcPtr = PLY_PTR_OFFSET(childSrcPtr, cmd.srcStride);
                    if (--itemsToCopy == 0) {
                        cursor = childCursor;
                    }
                }
                break;
            }

            case TypeConverter::Cmd::Copy32: {
                auto* cmd = safeCast<TypeConverter::BaseCmd>(view);
                *(u32*) PLY_PTR_OFFSET(dstPtr, cmd->dstOffset) =
                    *(u32*) PLY_PTR_OFFSET(srcPtr, cmd->srcOffset);
                view.offsetHead(sizeof(TypeConverter::BaseCmd));
                break;
            }

            case TypeConverter::Cmd::Copy32Range: {
                auto* cmd = safeCast<TypeConverter::Copy32Range>(view);
                u32* dst = (u32*) PLY_PTR_OFFSET(dstPtr, cmd->dstOffset);
                u32* dstEnd = dst + cmd->count;
                u32* src = (u32*) PLY_PTR_OFFSET(srcPtr, cmd->srcOffset);
                while (dst < dstEnd) {
                    *dst++ = *src++;
                }
                view.offsetHead(sizeof(TypeConverter::Copy32Range));
                break;
            }

            case TypeConverter::Cmd::EndScope: {
                view.offsetHead(sizeof(TypeConverter::Cmd));
                return;
            }

            default: {
                PLY_FORCE_CRASH(); // Unsupported
            }
        }
    }
}

void applyConversionRecipe(const BlockList::Footer* recipe, void* dstPtr,
                           ArrayView<void*> srcPtrs) {
    convert({const_cast<BlockList::Footer*>(recipe), recipe->start()}, dstPtr, srcPtrs);
}

} // namespace ply
