/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

// void writeTypeSignature(BinaryBuffer& sig, const TypeDescriptor* typeDesc); disabled for now

void createConversionRecipe(OutStream& out, const TypeDescriptor_Struct* dstStruct,
                            const ArrayView<TypeDescriptor_Struct*>& srcStructs);
void applyConversionRecipe(const BlockList::Footer* recipe, void* dstPtr, ArrayView<void*> srcPtrs);

} // namespace ply
