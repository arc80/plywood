/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
    DWORD read_bytes;
    BOOL rc = ReadFile(this->handle, buf.bytes, buf.num_bytes, &read_bytes, NULL);
    if (!rc) // Handles ERROR_BROKEN_PIPE and other errors.
        return 0;
    return read_bytes; // 0 when attempting to read past EOF.
}

u64 InPipe_Handle::get_file_size() {
    LARGE_INTEGER file_size;
    GetFileSizeEx(this->handle, &file_size);
    return file_size.QuadPart;
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
    while (buf.num_bytes > 0) {
        DWORD desired_bytes = min<DWORD>((DWORD) buf.num_bytes, UINT32_MAX);
        DWORD written_bytes;
        BOOL rc =
            WriteFile(this->handle, buf.bytes, desired_bytes, &written_bytes, NULL);
        if (!rc) // Handles ERROR_NO_DATA and other errors.
            return false;
        buf.bytes += written_bytes;
        buf.num_bytes -= written_bytes;
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
