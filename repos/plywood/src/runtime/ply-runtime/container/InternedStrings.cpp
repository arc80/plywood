/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Core.h>
#include <ply-runtime/container/InternedStrings.h>

namespace ply {

PLY_NO_INLINE InternedStrings::InternedStrings() {
    this->bigPool.append('\0');
}

PLY_NO_INLINE u32 InternedStrings::findOrInsertKey(StringView view) {
    auto cursor = this->strToIndex.insertOrFind(view, &this->bigPool);
    if (cursor.wasFound())
        return *cursor;

    u32 numEntryBytes = details::InternedStringEncoder::getEncLen(view.numBytes) + view.numBytes;
    u32 resultID = safeDemote<u32>(this->bigPool.numItems());
    *cursor = resultID;

    char* ptr = this->bigPool.alloc(numEntryBytes);
    details::InternedStringEncoder::encodeValue(ptr, view.numBytes);
    memcpy(ptr, view.bytes, view.numBytes);
    PLY_ASSERT(ptr + view.numBytes == this->bigPool.end());
    return resultID;
}

PLY_NO_INLINE StringView InternedStrings::view(u32 key) const {
    const char* ptr = this->bigPool.get(key);
    u32 numBytes = details::InternedStringEncoder::decodeValue(ptr);
    return {ptr, numBytes};
}

} // namespace ply
