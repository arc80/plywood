/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

// void write_type_signature(BinaryBuffer& sig, const TypeDescriptor* type_desc);
// disabled for now

void create_conversion_recipe(OutStream& out, const TypeDescriptor_Struct* dst_struct,
                              const ArrayView<TypeDescriptor_Struct*>& src_structs);
void apply_conversion_recipe(const BlockList::Footer* recipe, void* dst_ptr,
                             ArrayView<void*> src_ptrs);

} // namespace ply
