/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeConverter.h>
#include <ply-reflect/TypeKey.h>

// FIXME: Remove this declaration when textures are removed from the uniform objects:
namespace asset_bank {
extern ply::TypeKey TypeKey_AssetRef;
};

namespace ply {

#if 0 // disabled for now
// Note: If TypeDescriptors were flat, with enum-based TypeKeys, this function might not be needed:
void write_type_signature(BinaryBuffer& sig, const TypeDescriptor* type_desc) {
    enum class Token : u8 {
        End = 0,
        Float,
        FixedArray,
        Array,
        Struct,
    };

    if (type_desc->type_key == &TypeKey_Float) {
        sig.append(Token::Float);
    } else if (type_desc->type_key == &TypeKey_FixedArray) {
        const TypeDescriptor_FixedArray* fixed_arr_type = type_desc->cast<TypeDescriptor_FixedArray>();
        sig.append(Token::FixedArray);
        sig.append(check_cast<u16>(fixed_arr_type->num_items));
        sig.append(check_cast<u16>(fixed_arr_type->stride));
        write_type_signature(sig, fixed_arr_type->item_type);
    } else if (type_desc->type_key == &TypeKey_Array) {
        const TypeDescriptor_Array* arr_type = type_desc->cast<TypeDescriptor_Array>();
        sig.append(Token::Array);
        write_type_signature(sig, arr_type->item_type);
    } else if (type_desc->type_key == &TypeKey_Struct) {
        const TypeDescriptor_Struct* struct_type = type_desc->cast<TypeDescriptor_Struct>();
        sig.append(Token::Struct);
        sig.append(check_cast<u8>(struct_type->members.num_items()));
        for (const auto& member : struct_type->members) {
            sig.append(check_cast<u8>(member.name.num_bytes()));
            void* dst = sig.begin_enqueue(member.name.num_bytes());
            memcpy(dst, member.name.bytes(), member.name.num_bytes());
            sig.append(check_cast<u16>(member.offset));
            write_type_signature(sig, member.type);
        }
    } else if (type_desc->type_key == &TypeKey_RawPtr || type_desc->type_key == &asset_bank::TypeKey_AssetRef) {
        // FIXME: Need to remove textures from the uniform objects
    } else {
        PLY_FORCE_CRASH();     // unsupported destination type_desc
    }
}
#endif

struct TypeConverter {
    struct WriteContext {
        u32 dst_offset;
        u32 src_offset;

        WriteContext(u32 dst_offset, u32 src_offset)
            : dst_offset{dst_offset}, src_offset{src_offset} {
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
        u16 source_index;
    };

    struct IterateArrayToFixedArray {
        Cmd cmd;
        u16 dst_offset;
        u16 src_offset;
        u16 dst_stride;
        u16 src_stride;
        u16 dst_size;
    };

    struct BaseCmd {
        Cmd cmd;
        u16 dst_offset;
        u16 src_offset;
    };

    struct Copy32Range {
        Cmd cmd;
        u16 dst_offset;
        u16 src_offset;
        u16 count;
    };
};

void make_conversion_recipe(OutStream& out,
                            const TypeConverter::WriteContext& write_ctx,
                            const TypeDescriptor* type_desc,
                            const TypeDescriptor* src_type_desc) {
    if (type_desc->type_key == &TypeKey_Float) {
        PLY_ASSERT(src_type_desc->type_key == &TypeKey_Float);
        NativeEndianWriter{out}.write(TypeConverter::BaseCmd{
            TypeConverter::Cmd::Copy32, check_cast<u16>(write_ctx.dst_offset),
            check_cast<u16>(write_ctx.src_offset)});
    } else if (type_desc->type_key == &TypeKey_FixedArray) {
        const TypeDescriptor_FixedArray* dst_fixed_array_type =
            type_desc->cast<TypeDescriptor_FixedArray>();
        if (src_type_desc->type_key == &TypeKey_FixedArray) {
            const TypeDescriptor_FixedArray* src_fixed_array_type =
                src_type_desc->cast<TypeDescriptor_FixedArray>();
            // FIXME: Warn on size mismatch
            u32 items_to_copy = min<u32>(dst_fixed_array_type->num_items,
                                         src_fixed_array_type->num_items);
            if (dst_fixed_array_type->item_type->type_key == &TypeKey_Float &&
                src_fixed_array_type->item_type->type_key == &TypeKey_Float &&
                dst_fixed_array_type->stride == sizeof(float) &&
                src_fixed_array_type->stride == sizeof(float)) {
                // Fast path: FixedArray of float with default stride
                NativeEndianWriter{out}.write(
                    TypeConverter::Copy32Range{TypeConverter::Cmd::Copy32Range,
                                               check_cast<u16>(write_ctx.dst_offset),
                                               check_cast<u16>(write_ctx.src_offset),
                                               check_cast<u16>(items_to_copy)});
            } else {
                // Slow path
                TypeConverter::WriteContext child_write_ctx = write_ctx;
                for (u32 i = 0; i < items_to_copy; i++) {
                    make_conversion_recipe(out, child_write_ctx,
                                           dst_fixed_array_type->item_type,
                                           src_fixed_array_type->item_type);
                    child_write_ctx.dst_offset += dst_fixed_array_type->stride;
                    child_write_ctx.src_offset += src_fixed_array_type->stride;
                }
            }
        } else if (src_type_desc->type_key == &TypeKey_Array) {
            const TypeDescriptor_Array* src_arr_type =
                src_type_desc->cast<TypeDescriptor_Array>();
            NativeEndianWriter{out}.write(TypeConverter::IterateArrayToFixedArray{
                TypeConverter::Cmd::IterateArrayToFixedArray,
                check_cast<u16>(write_ctx.dst_offset),
                check_cast<u16>(write_ctx.src_offset),
                check_cast<u16>(dst_fixed_array_type->stride),
                check_cast<u16>(src_arr_type->item_type->fixed_size),
                check_cast<u16>(dst_fixed_array_type->num_items),
            });
            make_conversion_recipe(out, {0, 0}, dst_fixed_array_type->item_type,
                                   src_arr_type->item_type);
            NativeEndianWriter{out}.write(TypeConverter::Cmd::EndScope);
        } else {
            PLY_FORCE_CRASH(); // unsupported src_type_desc
        }
    } else {
        PLY_FORCE_CRASH(); // unsupported destination type_desc
    }
}

void create_conversion_recipe(OutStream& out, const TypeDescriptor_Struct* dst_struct,
                              const ArrayView<TypeDescriptor_Struct*>& src_structs) {
    for (const auto& dst_member : dst_struct->members) {
        for (u32 s = 0; s < src_structs.num_items; s++) {
            TypeDescriptor_Struct* src_struct = src_structs[s];
            for (const auto& src_member : src_struct->members) {
                if (dst_member.name == src_member.name) {
                    NativeEndianWriter{out}.write(TypeConverter::SetRootSourceIndex{
                        TypeConverter::Cmd::SetRootSourceIndex, check_cast<u16>(s)});
                    TypeConverter::WriteContext write_ctx = {dst_member.offset,
                                                             src_member.offset};
                    make_conversion_recipe(out, write_ctx, dst_member.type,
                                           src_member.type);
                    goto found;
                }
            }
        }
        PLY_ASSERT(0); // No match for dst_member
    found:;
    }
    NativeEndianWriter{out}.write(TypeConverter::Cmd::EndScope);
}

template <class T>
const T* safe_cast(StringView view) {
    PLY_ASSERT(sizeof(T) <= view.num_bytes);
    return (const T*) view.bytes;
}

void convert(BlockList::WeakRef cursor, void* dst_ptr, ArrayView<void*> src_ptrs,
             void* src_ptr = nullptr) {
    // FIXME: Implement MessageSequence and use that instead.

    BlockList::Footer* view_block = nullptr;
    StringView view;
    for (;;) {
        if (view.is_empty()) {
            view_block = cursor.block;
            if (!view_block)
                break;
            PLY_ASSERT(view_block->view_used_bytes().contains(cursor.byte));
            view = {cursor.byte, check_cast<u32>(view_block->end() - cursor.byte)};
            PLY_ASSERT(!view.is_empty());
            cursor = cursor.block->weak_ref_to_next();
        }

        switch (*safe_cast<TypeConverter::Cmd>(view)) {
            case TypeConverter::Cmd::SetRootSourceIndex: {
                // Note: Could add an assert here to ensure that we are at the "root"
                // convert(), but that would require passing an additional depth
                // parameter
                auto* cmd = safe_cast<TypeConverter::SetRootSourceIndex>(view);
                src_ptr = src_ptrs[cmd->source_index];
                view.offset_head(sizeof(TypeConverter::SetRootSourceIndex));
                break;
            }

            case TypeConverter::Cmd::IterateArrayToFixedArray: {
                auto cmd = *safe_cast<TypeConverter::IterateArrayToFixedArray>(view);
                view.offset_head(sizeof(TypeConverter::IterateArrayToFixedArray));

                BaseArray* base_arr =
                    (BaseArray*) PLY_PTR_OFFSET(src_ptr, cmd.src_offset);
                // FIXME: Warn if base_arr has too many source elements
                u32 items_to_copy = min<u32>(cmd.dst_size, base_arr->num_items);
                void* child_dst_ptr = PLY_PTR_OFFSET(dst_ptr, cmd.dst_offset);
                void* child_src_ptr = base_arr->items;
                while (items_to_copy) {
                    BlockList::WeakRef child_cursor = cursor; // Copy child cursor
                    convert(child_cursor, child_dst_ptr, src_ptrs, child_src_ptr);
                    child_dst_ptr = PLY_PTR_OFFSET(child_dst_ptr, cmd.dst_stride);
                    child_src_ptr = PLY_PTR_OFFSET(child_src_ptr, cmd.src_stride);
                    if (--items_to_copy == 0) {
                        cursor = child_cursor;
                    }
                }
                break;
            }

            case TypeConverter::Cmd::Copy32: {
                auto* cmd = safe_cast<TypeConverter::BaseCmd>(view);
                *(u32*) PLY_PTR_OFFSET(dst_ptr, cmd->dst_offset) =
                    *(u32*) PLY_PTR_OFFSET(src_ptr, cmd->src_offset);
                view.offset_head(sizeof(TypeConverter::BaseCmd));
                break;
            }

            case TypeConverter::Cmd::Copy32Range: {
                auto* cmd = safe_cast<TypeConverter::Copy32Range>(view);
                u32* dst = (u32*) PLY_PTR_OFFSET(dst_ptr, cmd->dst_offset);
                u32* dst_end = dst + cmd->count;
                u32* src = (u32*) PLY_PTR_OFFSET(src_ptr, cmd->src_offset);
                while (dst < dst_end) {
                    *dst++ = *src++;
                }
                view.offset_head(sizeof(TypeConverter::Copy32Range));
                break;
            }

            case TypeConverter::Cmd::EndScope: {
                view.offset_head(sizeof(TypeConverter::Cmd));
                return;
            }

            default: {
                PLY_FORCE_CRASH(); // Unsupported
            }
        }
    }
}

void apply_conversion_recipe(const BlockList::Footer* recipe, void* dst_ptr,
                             ArrayView<void*> src_ptrs) {
    convert({const_cast<BlockList::Footer*>(recipe), recipe->start()}, dst_ptr,
            src_ptrs);
}

} // namespace ply
