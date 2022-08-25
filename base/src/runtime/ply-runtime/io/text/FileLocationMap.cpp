/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/FileLocationMap.h>
#include <ply-runtime/string/TextEncoding.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

FileLocationMap FileLocationMap::fromView(StringView path, StringView src) {
    FileLocationMap result;
    result.path = path;
    result.view = src;
    u32 lineNumber = 1;
    u32 columnNumber = 1;
    u32 lineStartOfs = 0;

    u32 ofs = 0;
    u32 nextChunkOfs = 256;
    result.table.append({1, 0, 1, 0});
    for (;;) {
        if (src.numBytes == 0)
            break;

        DecodeResult decoded = UTF8::decodePoint(src);
        u32 nextOfs = ofs + decoded.numBytes;
        if (nextOfs > nextChunkOfs) {
            result.table.append({lineNumber, nextChunkOfs - lineStartOfs, columnNumber, ofs - 256});
            nextChunkOfs += 256;
        }
        ofs = nextOfs;
        src.offsetHead(decoded.numBytes);

        if (decoded.point == '\n') {
            lineNumber++;
            columnNumber = 1;
            lineStartOfs = ofs;
        } else if (decoded.point == '\t') {
            u32 tabSize = 4;
            columnNumber += tabSize - (columnNumber % tabSize);
        } else if (decoded.point == '\r') {
        } else {
            columnNumber++;
        }
    }
    return result;
}

FileLocation FileLocationMap::getFileLocation(u32 offset) const {
    PLY_ASSERT(offset <= this->view.numBytes);
    const FileLocation& fileLoc = this->table[offset >> 8];
    u32 chunkOfs = offset & ~0xff;
    const char* lineStart = this->view.bytes + (chunkOfs - fileLoc.numBytesIntoLine);
    StringView src = this->view;
    src.offsetHead(chunkOfs - fileLoc.numBytesIntoColumn);
    const char* target = this->view.bytes + offset;
    u32 lineNumber = fileLoc.lineNumber;
    u32 columnNumber = fileLoc.columnNumber;

    for (;;) {
        if (src.bytes >= target) {
            u32 nb = safeDemote<u32>(target - src.bytes);
            return {lineNumber, safeDemote<u32>(target - lineStart), columnNumber, nb};
        }

        // FIXME: Unify with similar code in previous function
        DecodeResult decoded = UTF8::decodePoint(src);
        src.offsetHead(decoded.numBytes);

        if (decoded.point == '\n') {
            lineNumber++;
            columnNumber = 1;
            lineStart = src.bytes;
        } else if (decoded.point == '\t') {
            u32 tabSize = 4; // FIXME: Make configurable somewhere
            columnNumber += tabSize - (columnNumber % tabSize);
        } else if (decoded.point == '\r') {
        } else {
            columnNumber++;
        }
    }
}

PLY_NO_INLINE String FileLocationMap::formatFileLocation(u32 offset, bool withPath) const {
    FileLocation fileLoc = this->getFileLocation(offset);
    return String::format("{}({}, {})", (withPath ? this->path : ""), fileLoc.lineNumber,
                          fileLoc.columnNumber);
}

} // namespace ply
