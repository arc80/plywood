/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/Core.h>
#include <ply-runtime/string/Label.h>

namespace ply {

LabelStorage* g_labelStorage = nullptr;

PLY_NO_INLINE void LabelStorage::initialize() {
    PLY_ASSERT(!g_labelStorage);
    g_labelStorage = new LabelStorage;
    g_labelStorage->bigPool.append('\0');
}

PLY_NO_INLINE Label LabelStorage::insert(StringView view) {
    PLY_RACE_DETECT_GUARD(this->raceDetector);

    auto cursor = this->strToIndex.insertOrFind(view, &this->bigPool);
    if (cursor.wasFound())
        return Label{*cursor};

    u32 numEntryBytes = details::LabelEncoder::getEncLen(view.numBytes) + view.numBytes;
    u32 resultID = safeDemote<u32>(this->bigPool.numItems());
    *cursor = resultID;

    char* ptr = this->bigPool.alloc(numEntryBytes);
    details::LabelEncoder::encodeValue(ptr, view.numBytes);
    memcpy(ptr, view.bytes, view.numBytes);
    PLY_ASSERT(ptr + view.numBytes == this->bigPool.end());
    return Label{resultID};
}

PLY_NO_INLINE Label LabelStorage::find(StringView view) const {
    PLY_RACE_DETECT_GUARD(this->raceDetector);

    auto cursor = this->strToIndex.find(view, &this->bigPool);
    if (cursor.wasFound())
        return Label{*cursor};
    return {};
}

PLY_NO_INLINE StringView LabelStorage::view(Label label) const {
    PLY_RACE_DETECT_GUARD(this->raceDetector);

    const char* ptr = this->bigPool.get(label.idx);
    u32 numBytes = details::LabelEncoder::decodeValue(ptr);
    return {ptr, numBytes};
}

} // namespace ply
