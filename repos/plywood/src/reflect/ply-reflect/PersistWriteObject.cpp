/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

void writeObject(TypedPtr obj, WriteObjectContext* writeObjectContext) {
    // Write formatID
    u32 formatID = writeObjectContext->writeFormatContext->addOrGetFormatID(obj.type);
    writeObjectContext->out.write<u32>(formatID);

    // Write the object
    obj.type->typeKey->write(obj, writeObjectContext);
}

void resolveLinksAndWriteLinkTable(MutableStringView view, OutStream* outs,
                                   SavedPtrResolver* ptrResolver) {
    u32 linkIndex = 0;
    for (const SavedPtrResolver::WeakPointerToResolve& weakInfo : ptrResolver->weakPtrsToResolve) {
        PLY_ASSERT(weakInfo.fileOffset + 4 <= view.numBytes);
        auto cursor =
            ptrResolver->addrToSaveInfo.find(weakInfo.ptr.ptr, &ptrResolver->savedOwnedPtrs);
        if (cursor.wasFound()) {
            SavedPtrResolver::SavedOwnedPtr& savedOwnedPtr = ptrResolver->savedOwnedPtrs[*cursor];
            PLY_ASSERT(savedOwnedPtr.ptr == weakInfo.ptr); // Sanity check
            if (savedOwnedPtr.linkIndex < 0) {
                savedOwnedPtr.linkIndex = linkIndex;
                linkIndex++;
            }
            // Note: This performs a potentially unaligned write
            *(u32*) PLY_PTR_OFFSET(view.bytes, weakInfo.fileOffset) = savedOwnedPtr.linkIndex;
        } else {
            PLY_ASSERT(0); // FIXME: Handle unresolved pointers gracefully
        }
    }

    // Write link table
    NativeEndianWriter out{outs};
    out.write(linkIndex); // Number of linkable Owned pointers
    u32 entriesWritten = 0;
    for (const SavedPtrResolver::SavedOwnedPtr& savedOwnedPtr : ptrResolver->savedOwnedPtrs) {
        if (savedOwnedPtr.linkIndex >= 0) {
            PLY_ASSERT(entriesWritten == (u32) savedOwnedPtr.linkIndex);
            out.write(savedOwnedPtr.fileOffset);
            entriesWritten++;
        }
    }
}

} // namespace ply
