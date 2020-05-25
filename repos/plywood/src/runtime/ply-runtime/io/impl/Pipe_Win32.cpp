/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/io/impl/Pipe_Win32.h>

namespace ply {

PLY_NO_INLINE void InPipe_Win32_destroy(InPipe* inPipe_) {
    InPipe_Win32* inPipe = static_cast<InPipe_Win32*>(inPipe_);
    if (inPipe->handle != INVALID_HANDLE_VALUE) {
        BOOL rc = CloseHandle(inPipe->handle);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE u32 InPipe_Win32_readSome(InPipe* inPipe_, BufferView buf) {
    InPipe_Win32* inPipe = static_cast<InPipe_Win32*>(inPipe_);
    DWORD readBytes;
    BOOL rc = ReadFile(inPipe->handle, buf.bytes, (DWORD) buf.numBytes, &readBytes, NULL);
    if (!rc) {
        // FIXME: Detect other errors
        PLY_ASSERT(GetLastError() == ERROR_BROKEN_PIPE);
        return 0; // A zero return value means EOF
    }
    return readBytes; // readBytes will be 0 when attempting to read past EOF
}

PLY_NO_INLINE u64 InPipe_Win32_getFileSize(const InPipe* inPipe_) {
    const InPipe_Win32* inPipe = static_cast<const InPipe_Win32*>(inPipe_);
    LARGE_INTEGER fileSize;
    BOOL rc = GetFileSizeEx(inPipe->handle, &fileSize);
    PLY_ASSERT(rc);
    PLY_UNUSED(rc);
    return fileSize.QuadPart;
}

InPipe::Funcs InPipe_Win32::Funcs_ = {
    InPipe_Win32_destroy,
    InPipe_Win32_readSome,
    InPipe_Win32_getFileSize,
};

PLY_NO_INLINE InPipe_Win32::InPipe_Win32(HANDLE handle) : InPipe{&Funcs_}, handle{handle} {
}

PLY_NO_INLINE void OutPipe_Win32_destroy(OutPipe* outPipe_) {
    OutPipe_Win32* outPipe = static_cast<OutPipe_Win32*>(outPipe_);
    if (outPipe->handle != INVALID_HANDLE_VALUE) {
        BOOL rc = CloseHandle(outPipe->handle);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE bool OutPipe_Win32_write(OutPipe* outPipe_, ConstBufferView buf) {
    OutPipe_Win32* outPipe = static_cast<OutPipe_Win32*>(outPipe_);
    while (buf.numBytes > 0) {
        DWORD desiredBytes = min<DWORD>((DWORD) buf.numBytes, UINT32_MAX);
        DWORD writtenBytes;
        BOOL rc = WriteFile(outPipe->handle, buf.bytes, desiredBytes, &writtenBytes, NULL);
        if (!rc) {
            PLY_ASSERT(GetLastError() == ERROR_NO_DATA);
            return false;
        }
        buf.bytes += writtenBytes;
        buf.numBytes -= writtenBytes;
    }
    return true;
}

PLY_NO_INLINE bool OutPipe_Win32_flush(OutPipe* outPipe_, bool toDevice) {
    bool result = true;
    if (toDevice) {
        OutPipe_Win32* outPipe = static_cast<OutPipe_Win32*>(outPipe_);
        BOOL rc = FlushFileBuffers(outPipe->handle);
        if (!rc) {
            DWORD err = GetLastError();
            switch (err) {
                case ERROR_INVALID_HANDLE:
                    break;
                default: {
                    PLY_ASSERT(0); // Need to recognize this error code
                    break;
                }
            }
            result = false;
        }
    }
    return result;
}

PLY_NO_INLINE u64 OutPipe_Win32_seek(OutPipe* outPipe_, s64 pos, SeekDir seekDir) {
    OutPipe_Win32* outPipe = static_cast<OutPipe_Win32*>(outPipe_);
    PLY_ASSERT(outPipe->handle != INVALID_HANDLE_VALUE);
    DWORD moveMethod;
    switch (seekDir) {
        case SeekDir::Set:
        default:
            moveMethod = FILE_BEGIN;
            break;
        case SeekDir::Cur:
            moveMethod = FILE_CURRENT;
            break;
        case SeekDir::End:
            moveMethod = FILE_END;
            break;
    }
    LARGE_INTEGER distance;
    distance.QuadPart = pos;
    LARGE_INTEGER newFilePos;
    newFilePos.QuadPart = 0;
    BOOL rc = SetFilePointerEx(outPipe->handle, distance, &newFilePos, moveMethod);
    if (!rc) {
        PLY_ASSERT(0); // Need to recognize this error code
    }
    return newFilePos.QuadPart;
}

OutPipe::Funcs OutPipe_Win32::Funcs_ = {
    OutPipe_Win32_destroy,
    OutPipe_Win32_write,
    OutPipe_Win32_flush,
    OutPipe_Win32_seek,
};

PLY_NO_INLINE OutPipe_Win32::OutPipe_Win32(HANDLE handle) : OutPipe{&Funcs_}, handle{handle} {
}

} // namespace ply

#endif // PLY_TARGET_WIN32
