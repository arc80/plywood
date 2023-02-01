/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

namespace ply {

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_Handle  ┃
// ┗━━━━━━━━━━━━━━━━━┛
InPipe_Handle::~InPipe_Handle() {
    if (this->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(this->handle);
    }
}

u32 InPipe_Handle::read(MutStringView buf) {
    DWORD readBytes;
    BOOL rc = ReadFile(this->handle, buf.bytes, buf.numBytes, &readBytes, NULL);
    if (!rc) // Handles ERROR_BROKEN_PIPE and other errors.
        return 0;
    return readBytes; // 0 when attempting to read past EOF.
}

u64 InPipe_Handle::get_file_size() {
    LARGE_INTEGER fileSize;
    GetFileSizeEx(this->handle, &fileSize);
    return fileSize.QuadPart;
}

void InPipe_Handle::seek(s64 offset, SeekDirection dir) {
    DWORD move_method = FILE_BEGIN;
    if (dir == Seek_Relative) {
        move_method = FILE_CURRENT;
    } else if (dir == Seek_End) {
        move_method = FILE_END;
    }
    LARGE_INTEGER distance;
    distance.QuadPart = offset;
    SetFilePointerEx(this->handle, distance, NULL, move_method);
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_Handle  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
OutPipe_Handle::~OutPipe_Handle() {
    if (this->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(this->handle);
    }
}

bool OutPipe_Handle::write(StringView buf) {
    while (buf.numBytes > 0) {
        DWORD desiredBytes = min<DWORD>((DWORD) buf.numBytes, UINT32_MAX);
        DWORD writtenBytes;
        BOOL rc = WriteFile(this->handle, buf.bytes, desiredBytes, &writtenBytes, NULL);
        if (!rc) // Handles ERROR_NO_DATA and other errors.
            return false;
        buf.bytes += writtenBytes;
        buf.numBytes -= writtenBytes;
    }
    return true;
}

void OutPipe_Handle::flush(bool hard) {
    if (hard) {
        FlushFileBuffers(this->handle);
    }
}

void OutPipe_Handle::seek(s64 offset, SeekDirection dir) {
    DWORD move_method = FILE_BEGIN;
    if (dir == Seek_Relative) {
        move_method = FILE_CURRENT;
    } else if (dir == Seek_End) {
        move_method = FILE_END;
    }
    LARGE_INTEGER distance;
    distance.QuadPart = offset;
    SetFilePointerEx(this->handle, distance, NULL, move_method);
}

} // namespace ply

#endif // PLY_TARGET_WIN32
